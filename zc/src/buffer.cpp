#include "opencv2/core/ocl.hpp"
#include "opencv2/oclrenderpano.hpp"


#if 0
#define LOGD printf
#else
#define LOGD(...)
#endif

namespace cv {
namespace ocl {
namespace imvt {

using namespace std;

#define APPEND(a) append(buffers, a);

inline void  append(vector<UMat>& c, const UMat& e) {
	c.insert(c.end(), e);
}

inline void  append(vector<UMat>& c, const vector<UMat>& a) {
	c.insert(c.end(), a.begin(), a.end());
}

inline size_t allocGranularity(size_t size) {
	if (size < 1024 * 1024)
		return 4096;
	else if (size < 16 * 1024 * 1024)
		return 64 * 1024;
	else
		return 1024 * 1024;
}

size_t estimate(vector<UMat>& buffers) {
	size_t s = 0;
	for (int i = 0; i < buffers.size(); ++i) {
		size_t size = buffers[i].rows * buffers[i].cols * buffers[i].elemSize();
		s += alignSize(size, (int)allocGranularity(size));
	}
	return s;
}

vector<UMat> buildPyramid(const UMat& src) {
	vector<UMat> pyramid = { src };
	while (pyramid.size() < 1000) {
		Size newSize(pyramid.back().cols * 0.9f + 0.5f, pyramid.back().rows * 0.9f + 0.5f);
		if (newSize.height <= 24 || newSize.width <= 24) {
			break;
		}
		UMat scaledImage(newSize, src.type());
		pyramid.push_back(scaledImage);
	}
	return pyramid;
}

/**
* @brief allocate buffers for optical flow
*/
void allocForOpticalFlow(vector<UMat>& buffers, Size imgSize) {

	Size downscaleSize(imgSize.width *  0.5f, imgSize.height * 0.5f);
	
	// for finalFlow, finalFlowTmp
	UMat finalFlow(imgSize, CV_32FC2);
	UMat finalFlowTmp(imgSize, CV_32FC2);
	APPEND(finalFlow);
	APPEND(finalFlowTmp);

	// for rgba0byteDownscaled, rgba1byteDownscaled, prevI0BGRADownscaled, prevI1BGRADownscaled
	UMat rgba0byteDownscaled(downscaleSize, CV_8UC4);
	UMat rgba1byteDownscaled(downscaleSize, CV_8UC4); 
	UMat prevI0BGRADownscaled(downscaleSize, CV_8UC4);
	UMat prevI1BGRADownscaled(downscaleSize, CV_8UC4);
	APPEND(rgba0byteDownscaled);
	APPEND(rgba1byteDownscaled);
	APPEND(prevI0BGRADownscaled);
	APPEND(prevI1BGRADownscaled);

	// for I0Grey, I1Grey, 
	UMat I0Grey(downscaleSize, CV_8UC1); 
	UMat I1Grey(downscaleSize, CV_8UC1);
	APPEND(I0Grey);
	APPEND(I1Grey);

	// for I0Tmp,(Gaussian blur)
	UMat I0Tmp(downscaleSize, CV_32F);
	APPEND(I0Tmp);

	// for channels0, channels1
	vector<UMat> channels0;
	vector<UMat> channels1;
	split(rgba0byteDownscaled, channels0);
	split(rgba1byteDownscaled, channels1);
	APPEND(channels0);
	APPEND(channels1);

	// for I0, I1, alpha0, alpha1, prevFlowDownscaled, motion
	UMat I0(downscaleSize, CV_32F);
	UMat I1(downscaleSize, CV_32F);
	UMat alpha0(downscaleSize, CV_32F);
	UMat alpha1(downscaleSize, CV_32F);
	UMat prevFlowDownscaled(downscaleSize, CV_32FC2);
	UMat motion(downscaleSize, CV_32F);
	vector<UMat> pyramidI0 = buildPyramid(I0);
	vector<UMat> pyramidI1 = buildPyramid(I1);
	vector<UMat> pyramidAlpha0 = buildPyramid(alpha0);
	vector<UMat> pyramidAlpha1 = buildPyramid(alpha1);
	vector<UMat> prevFlowPyramid = buildPyramid(prevFlowDownscaled);
	vector<UMat> motionPyramid = buildPyramid(motion);
	APPEND(pyramidI0);
	APPEND(pyramidI1);
	APPEND(pyramidAlpha0);
	APPEND(pyramidAlpha1);
	APPEND(prevFlowPyramid);
	APPEND(motionPyramid);

	// for flow, flowTmp blurredFlow, I0x, I0y, I1x, I1y
	UMat flow(downscaleSize, CV_32FC2);
	UMat flowTmp(downscaleSize, CV_32FC2);
	UMat blurredFlow(downscaleSize, CV_32FC2);
	UMat I0x(downscaleSize, CV_32F);
	UMat I0y(downscaleSize, CV_32F);
	UMat I1x(downscaleSize, CV_32F);
	UMat I1y(downscaleSize, CV_32F);
	vector<UMat> pyramidFlow = buildPyramid(flow);
	vector<UMat> pyramidFlowTmp = buildPyramid(flowTmp);
	vector<UMat> pyramidBlurredFlow = buildPyramid(blurredFlow);
	vector<UMat> pyramidI0x = buildPyramid(I0x);
	vector<UMat> pyramidI0y = buildPyramid(I0y);
	vector<UMat> pyramidI1x = buildPyramid(I1x);
	vector<UMat> pyramidI1y = buildPyramid(I1y);
	APPEND(pyramidFlow);
	APPEND(pyramidFlowTmp);
	APPEND(pyramidBlurredFlow);
	APPEND(pyramidI0x);
	APPEND(pyramidI0y);
	APPEND(pyramidI1x);
	APPEND(pyramidI1y);
}


void allocForNovelView(vector<UMat>& buffers, Size nvSize) {
	// for render Lazy Novel View
	UMat warpOpticalFlow(nvSize, CV_32FC2);
	UMat remappedFlow(nvSize, CV_32FC2);
	APPEND(warpOpticalFlow);
	APPEND(remappedFlow);

	// for novelView, novelViewFlowMag
	UMat novelView(nvSize, CV_8UC4);
	UMat novelViewFlowMag(nvSize, CV_32F);
	for (int i = 0; i < 4; ++i) {
		APPEND(novelView);
		APPEND(novelViewFlowMag);
	}
}

void allocForRenderChunks(vector<UMat>& buffers, int nCams, Size optSize, Size nvSize) {
	vector<UMat> flowLtoRs;
	vector<UMat> flowRtoLs;
	vector<UMat> chunkLs;
	vector<UMat> chunkRs;
	for (int i = 0; i < nCams; ++i) {
		flowLtoRs.push_back(UMat(optSize, CV_32FC2));
		flowRtoLs.push_back(UMat(optSize, CV_32FC2));
		chunkLs.push_back(UMat(nvSize, CV_32FC2));
		chunkRs.push_back(UMat(nvSize, CV_32FC2));

		allocForOpticalFlow(buffers, optSize);
		allocForNovelView(buffers, nvSize);
	}
	APPEND(flowLtoRs);
	APPEND(flowRtoLs);
	APPEND(chunkLs);
	APPEND(chunkRs);
}

void allocForGammaLUT(vector<UMat>& buffers) {
	UMat lutGamma(1, 65536, CV_16U);
	UMat lutAntiGamma(1, 65536, CV_16U);
	APPEND(lutGamma);
	APPEND(lutAntiGamma);
}

void allocForWarps(vector<UMat>& buffers, Size nvSize) {
	vector<UMat> warpLs;
	vector<UMat> warpRs;
	for (int i = 0; i < 1; ++i) {
		warpLs.push_back(UMat(nvSize, CV_32FC3));
		warpRs.push_back(UMat(nvSize, CV_32FC3));
	}
	APPEND(warpLs);
	APPEND(warpRs);
}

void allocForPrevious(vector<UMat>& buffers, int nCams, Size optSize) {
	vector<UMat> imgLs;
	vector<UMat> imgRs;
	vector<UMat> flowLtoRs;
	vector<UMat> flowRtoLs;
	for (int i = 0; i < nCams; ++i) {
		imgLs.push_back(UMat(optSize, CV_8UC4));
		imgRs.push_back(UMat(optSize, CV_8UC4));
		flowLtoRs.push_back(UMat(optSize, CV_32FC2));
		flowRtoLs.push_back(UMat(optSize, CV_32FC2));
	}
	APPEND(imgLs);
	APPEND(imgRs);
	APPEND(flowLtoRs);
	APPEND(flowRtoLs);
}


size_t getReservedBufferSize() {
	MatAllocator* allocator = ocl::getOpenCLAllocator();
	BufferPoolController* controller = allocator->getBufferPoolController("OCL");
	size_t s = 0;
	if (controller) {
		s += controller->getReservedSize();
	}
	controller = allocator->getBufferPoolController("HOST_ALLOC");
	if (controller) {
		s += controller->getReservedSize();
	}
	controller = allocator->getBufferPoolController("SVM");
	if (controller) {
		s += controller->getReservedSize();
	}
	return s;
}

CV_EXPORTS_W bool oclInitBuffers(int nCams, Size optSize, Size nvSize, int& numThreads) {
	
	try {
	LOGD("before init, reserved buffer size: %llu\n", getReservedBufferSize());

	// hold buffers for render chunks
	vector<UMat> common;
	allocForGammaLUT(common);
	allocForWarps(common, nvSize);
	allocForPrevious(common, nCams, optSize);
	size_t commonSize = estimate(common);
	LOGD("common buffer size: %llu\n", commonSize);
	LOGD("after common alloc, reserved buffer size: %llu\n", getReservedBufferSize());

	// run all functions except render chunks
	{
	vector<UMat> images(nCams);
	vector<UMat> chunkLs(nCams);
	vector<UMat> chunkRs(nCams);
	for (int i = 0; i < nCams; ++i) {
		images[i] = UMat(Size(optSize.width + nvSize.width, optSize.height), CV_8UC4);
		chunkLs[i] = UMat(nvSize, CV_8UC4);
		chunkRs[i] = UMat(nvSize, CV_8UC4);
	}
	oclPreColorAdjustByGamma(images, -1, 180, 0.01, false, false, true);

	oclRemoveChunkLines(chunkLs);
	oclRemoveChunkLines(chunkRs);

	UMat panoL;
	UMat panoR;
	oclStackHorizontal(chunkLs, panoL);
	oclStackHorizontal(chunkRs, panoR);

	oclSharpImage(panoL, 0.5);
	oclSharpImage(panoR, 0.5);

	UMat prePanoL(panoL.size(), panoL.type());
	UMat prePanoR(panoR.size(), panoR.type());
	oclSmoothImage(panoL, prePanoL, 0.05f, true);
	oclSmoothImage(panoR, prePanoR, 0.05f, true);

	oclOffsetHorizontalWrap(panoL, 150);
	oclOffsetHorizontalWrap(panoR, -150);

	UMat pano;
	vector<UMat> panos = { panoL, panoR };
	oclStackHorizontal(panos, pano);
	}
	ocl::finish();
	size_t otherSize = getReservedBufferSize();
	LOGD("after warm up, reserved buffer size: %llu\n", getReservedBufferSize());

	vector<UMat> chunks;
	allocForRenderChunks(chunks, 1, optSize, nvSize);
	ocl::finish();
	size_t chunkSize = estimate(chunks);
	LOGD("chunk buffer size: %llu\n", chunkSize);
	LOGD("after one chunk alloc, reserved buffer size: %llu\n", getReservedBufferSize());

	size_t globalSize = ocl::Device::getDefault().globalMemSize();
	if (commonSize + chunkSize + otherSize > globalSize) {
		return false;
	}
	numThreads = (globalSize - commonSize - otherSize) / chunkSize;
	LOGD("max thread num: %d\n", numThreads);
	numThreads = nCams > numThreads ? numThreads/2 : numThreads;
	numThreads = numThreads >= 2 ? (numThreads >= 4 ? 4 : 2) : 1;
	LOGD("suggest thread num: %d\n", numThreads);

	allocForRenderChunks(chunks, numThreads - 1, optSize, nvSize);
	ocl::finish();
	LOGD("after other chunks alloc, reserved buffer size: %llu\n", getReservedBufferSize());

	chunks.clear();
	ocl::finish();
	LOGD("after chunks release, reserved buffer size: %llu\n", getReservedBufferSize());

	common.clear();
	ocl::finish();
	LOGD("after init, reserved buffer size: %llu\n", getReservedBufferSize());
	
	} catch (...) {
		LOGD("init buffer failed!");
		return false;
	}
	return true;
}







}	// namespace imvt
}	// namespace ocl
} 	// namespace cv