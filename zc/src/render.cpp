/**
* Copyright (c) 2016-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE_render file in the root directory of this subproject. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#include <stdio.h>

#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>

#include "precomp.hpp"
#include "opencv2/core/opencl/runtime/opencl_core.hpp"
#include "opencv2/core/opencl/runtime/opencl_core_wrappers.hpp"

#include "opencv2/oclrenderpano/ocl_optflow.hpp"
#include "opencv2/oclrenderpano/ocl_novelview.hpp"
#include "opencv2/oclrenderpano/ocl_coloradjust.hpp"
#include "opencv2/oclrenderpano/ocl_buffer.hpp"
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

template <class T>
class SafeQueue {
public:
    SafeQueue() : q(), m(), c() {}
    ~SafeQueue() {}

void enqueue(T t) {
    std::lock_guard<std::mutex> lock(m);
    q.push(t);
    c.notify_one();
}

T dequeue() {
    std::unique_lock<std::mutex> lock(m);
    while (q.empty()) {
        c.wait(lock);
    }
    T val = q.front();
    q.pop();
    return val;
}

private:
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;
};


struct RenderTask {
    int index;
    const UMat* imageL;
    const UMat* imageR;
    UMat* chunkL;
	UMat* chunkR;
    float motionThreshold;
};


// @deprecated This is only a "rough estimate", should be refined after experiments.  
static size_t calcThreadCount(int numCams, Size imgSize) {
	if (numCams > 4) {
		numCams = 4;
	}
	size_t bufferPoolSize = ocl::Device::getDefault().globalMemSize() >> 20;
	size_t frameSize = (numCams* imgSize.width * imgSize.height * 4) >> 20;
	size_t ration = bufferPoolSize / frameSize;
	if (ration > 128) {
		return 4;
	}
	if (ration > 64) {
		return 2;
	}
	return 1;
}

struct RenderContext {

	// render tasks/threads
	SafeQueue<RenderTask> inQueue;
	SafeQueue<RenderTask> outQueue;
    vector<thread> threads;
    
	// previous images/flows
	vector<UMat> preImageLs;
	vector<UMat> preImageRs;
	vector<UMat> preFlowLtoRs;
	vector<UMat> preFlowRtoLs;

	// for both stereo and mono
	// NOTES: 
	// opticalFlowWidth == overlapWidth
	// overlapWidth + numNovelViews == camImageWidth
	int numCams = 0;
	int camImageWidth = 0;
	int camImageHeight = 0;
	int numNovelViews = 0;
	float vergeAtInfinitySlabDisplacement = 0.0f;

	// true for stereo, false for mono
	bool isStereo = false;

	// for stereo
	vector<UMat> warpLs;
	vector<UMat> warpRs;

	// for mono
	vector<UMat> warps;

	static RenderContext& instance() {
		static RenderContext context;
		return context;
	}

	bool isInit() {
		return numCams > 0;
	}

	void initMonoWarps(
		int numCams,
		int camImageWidth,
		int camImageHeight,
		int numNovelViews) {

		Mat warp(Size(numNovelViews, camImageHeight), CV_32FC3);
		for (int x = 0; x < numNovelViews; ++x) {
			const float shift = float(x) / float(numNovelViews);
			const float slabShift = float(camImageWidth) * 0.5f - float(numNovelViews - x);
			for (int y = 0; y < camImageHeight; ++y) {
				warp.at<Point3f>(y, x) = Point3f(slabShift, y, shift);
			};
		}
		warps.assign(numCams, UMat());
		for (int i = 0; i < numCams; ++i) {
			warp.copyTo(warps[i]);
		}
	}

	void initStereoWarps(
		int numCams,
		int camImageWidth,
		int camImageHeight, 
		int numNovelViews, 
		float vergeAtInfinitySlabDisplacement) {

		Mat warpL(Size(numNovelViews, camImageHeight), CV_32FC3);
		Mat warpR(Size(numNovelViews, camImageHeight), CV_32FC3);
		for (int x = 0; x < numNovelViews; ++x) {
			const float shift = float(x) / float(numNovelViews);
			const float slabShift = float(camImageWidth) * 0.5f - float(numNovelViews - x);
			for (int y = 0; y < camImageHeight; ++y) {
				warpL.at<Point3f>(y, x) = Point3f(slabShift + vergeAtInfinitySlabDisplacement, y, shift);
				warpR.at<Point3f>(y, x) = Point3f(slabShift - vergeAtInfinitySlabDisplacement, y, shift);
			};
		}
		UMat uwarpL;
		UMat uwarpR;
		warpL.copyTo(uwarpL);
		warpR.copyTo(uwarpR);
		warpLs.assign(numCams, uwarpL);
		warpRs.assign(numCams, uwarpR);

		// @deprecated
		//warpLs.assign(numCams, UMat());
		//warpRs.assign(numCams, UMat());
		//for (int i = 0; i < numCams; ++i) {
		//	warpL.copyTo(warpLs[i]);
		//	warpR.copyTo(warpRs[i]);
		//}
	}

	void init(
		bool isStereo,
		int numCams,
		int camImageWidth,
		int camImageHeight,
		int numNovelViews,
		float vergeAtInfinitySlabDisplacement) {

		if (isStereo) {
			initStereoWarps(numCams, camImageWidth, camImageHeight, numNovelViews, vergeAtInfinitySlabDisplacement);
		} else {
			initMonoWarps(numCams, camImageWidth, camImageHeight, numNovelViews);
		}

		preImageLs.assign(numCams, UMat());
		preImageRs.assign(numCams, UMat());
		preFlowLtoRs.assign(numCams, UMat());
		preFlowRtoLs.assign(numCams, UMat());

		this->isStereo = isStereo;
		this->numCams = numCams;
		this->camImageHeight = camImageHeight;
		this->camImageWidth = camImageWidth;
		this->numNovelViews = numNovelViews;
		this->vergeAtInfinitySlabDisplacement = vergeAtInfinitySlabDisplacement;
	}

	void release() {
		preImageLs.clear();
		preImageRs.clear();
		preFlowLtoRs.clear();
		preFlowRtoLs.clear();

		warps.clear();
		warpLs.clear();
		warpRs.clear();

		isStereo = false;
		numCams = 0;
		camImageHeight = 0;
		camImageWidth = 0;
		numNovelViews = 0;
		vergeAtInfinitySlabDisplacement = 0.0f;
	}

	void resetPrevious() {
		preImageLs.assign(numCams, UMat());
		preImageRs.assign(numCams, UMat());
		preFlowLtoRs.assign(numCams, UMat());
		preFlowRtoLs.assign(numCams, UMat());
	}

    void startThreads(int numThreads = 4) {
        for (int i=0; i < numThreads; i++) {
            threads.push_back(thread(renderChunkThread, this));
        }
    }

    void stopThreads() {
		for (thread& t : threads) {
			RenderTask stopTask = { -1 };
			inQueue.enqueue(stopTask);
		}
        for (thread& t : threads) {
            t.join();
        }
		threads.clear();
    }

	static void renderChunkThread(RenderContext* c) {
		// initialize OpenCL and SVM if necessary
		ocl::useOpenCL();
		if (ocl::haveSVM()) {
			ocl::Context::getDefault().useSVM();
		}

		LOGD("render thread %u is started\n", this_thread::get_id());
        
		while (1) {
			// get render task from input queue
			LOGD("render thread %u is waiting input task\n", this_thread::get_id());
			RenderTask t = c->inQueue.dequeue();

			LOGD("render thread %u got input task: %d\n", this_thread::get_id(), t.index);
			// exit this thread if stop task received
			if (t.index == -1) {
				break;
			}

			// compute optical flows
			UMat flowLtoR;
			UMat flowRtoL;
			oclComputeOpticalFlow(
				*(t.imageL),
				*(t.imageR),
				c->preFlowLtoRs[t.index],
				c->preImageLs[t.index],
				c->preImageRs[t.index],
				flowLtoR,
				DirectionHint::LEFT,
				t.motionThreshold);
			oclComputeOpticalFlow(
				*(t.imageR),
				*(t.imageL),
				c->preFlowRtoLs[t.index],
				c->preImageRs[t.index],
				c->preImageLs[t.index],
				flowRtoL,
				DirectionHint::RIGHT,
				t.motionThreshold);

			// combine novel views
			if (c->isStereo) {
				oclCombineNovelViews(
					c->warpLs[t.index],
					c->warpRs[t.index],
					*(t.imageL),
					*(t.imageR),
					flowLtoR,
					flowRtoL,
					*(t.chunkL),
					*(t.chunkR));
			} else {
				oclCombineNovelViews(
					c->warps[t.index],
					*(t.imageL),
					*(t.imageR),
					flowLtoR,
					flowRtoL,
					*(t.chunkL));
			}

			// save previous images/flows
			t.imageL->copyTo(c->preImageLs[t.index]);
			t.imageR->copyTo(c->preImageRs[t.index]);
			c->preFlowLtoRs[t.index] = flowLtoR;
			c->preFlowRtoLs[t.index] = flowRtoL;

			// wait for opencl completed
			ocl::Queue::getDefault().finish();
			LOGD("render thread %u finished input task: %d\n", this_thread::get_id(), t.index);
 
			// put render result to output queue
			c->outQueue.enqueue(t);
			LOGD("render thread %u enqueued output result: %d\n", this_thread::get_id(), t.index);
        }

		ocl::Queue::getDefault().~Queue();
		LOGD("render thread %u is exited\n", this_thread::get_id());
	}


	void renderChunk(int index, const UMat& imageL, const UMat& imageR, UMat& chunkL, UMat& chunkR, float motionThreshold) {

		// compute optical flows
		UMat flowLtoR;
		UMat flowRtoL;
		oclComputeOpticalFlow(
			imageL,
			imageR,
			preFlowLtoRs[index],
			preImageLs[index],
			preImageRs[index],
			flowLtoR,
			DirectionHint::LEFT,
			motionThreshold);
		oclComputeOpticalFlow(
			imageR,
			imageL,
			preFlowRtoLs[index],
			preImageRs[index],
			preImageLs[index],
			flowRtoL,
			DirectionHint::RIGHT,
			motionThreshold);

		// combine novel views
		if (isStereo) {
			oclCombineNovelViews(
				warpLs[index],
				warpRs[index],
				imageL,
				imageR,
				flowLtoR,
				flowRtoL,
				chunkL,
				chunkR);
		} else {
			oclCombineNovelViews(
				warps[index],
				imageL,
				imageR,
				flowLtoR,
				flowRtoL,
				chunkL);
		}

		// save previous images/flows
		imageL.copyTo(preImageLs[index]);
		imageR.copyTo(preImageRs[index]);
		preFlowLtoRs[index] = flowLtoR;
		preFlowRtoLs[index] = flowRtoL;
	}


	void renderChunks(const vector<UMat>& imgLs, const vector<UMat>& imgRs, vector<UMat>& chunkLs, vector<UMat>& chunkRs, float motionThreshold) {
		
		// re-assign output chunks if size different
		if (chunkLs.size() != imgLs.size()) {
			chunkLs.assign(imgLs.size(), UMat());
		}
		if (chunkRs.size() != imgRs.size()) {
			chunkRs.assign(imgRs.size(), UMat());
		}

		// no render threads
		if (threads.size() == 0) {
			for (int index = 0; index < imgLs.size(); ++index) {
				renderChunk(index, imgLs[index], imgRs[index], chunkLs[index], chunkRs[index], motionThreshold);
				ocl::finish();
			}
			return;
		}

		// put task to input queue
		for (int index = 0; index < imgLs.size(); ++index) {
			RenderTask task = { index, &imgLs[index], &imgRs[index], &chunkLs[index], &chunkRs[index], motionThreshold };
			inQueue.enqueue(task);
		}

		// get chunks from output queue
		for (int index = 0; index < imgLs.size(); ++index) {
			RenderTask out = outQueue.dequeue();
			chunkLs[out.index] = *(out.chunkL);
			chunkRs[out.index] = *(out.chunkR);
		}
	}
};


static void setBufferPoolSize(size_t size) {
	MatAllocator* allocator = ocl::getOpenCLAllocator();
	BufferPoolController* controller = allocator->getBufferPoolController("OCL");
	if (controller) {
		controller->setMaxReservedSize(size);
	}
	controller = allocator->getBufferPoolController("HOST_ALLOC");
	if (controller) {
		controller->setMaxReservedSize(size);
	}
	controller = allocator->getBufferPoolController("SVM");
	if (controller) {
		controller->setMaxReservedSize(size);
	}
}

static void releaseBufferPool() {
	MatAllocator* allocator = ocl::getOpenCLAllocator();
	BufferPoolController* controller = allocator->getBufferPoolController("OCL");
	if (controller) {
		controller->freeAllReservedBuffers();
	}
	controller = allocator->getBufferPoolController("HOST_ALLOC");
	if (controller) {
		controller->freeAllReservedBuffers();
	}
	controller = allocator->getBufferPoolController("SVM");
	if (controller) {
		controller->freeAllReservedBuffers();
	}
}


CV_EXPORTS_W void oclRenderStereoPanoramaChunks(
	const vector<UMat>& imageLs,
	const vector<UMat>& imageRs,
	vector<UMat>& chunks,
	float motionThreshold) {
	RenderContext& context = RenderContext::instance();
	if (!context.isInit()) {
		context.init(false, imageLs.size(), 2 * imageLs[0].cols , imageLs[0].rows, imageLs[0].cols, 0.0f);
		context.startThreads();
	}
	vector<UMat> chunkDummys;
	context.renderChunks(imageLs, imageRs, chunks, chunkDummys, motionThreshold);
}
CV_EXPORTS_W void oclRenderStereoPanoramaChunks(
	const std::vector<UMat>& imageLs,
	const std::vector<UMat>& imageRs,
	std::vector<UMat>& chunkLs,
	std::vector<UMat>& chunkRs,
	float motionThreshold) {
	RenderContext& context = RenderContext::instance();
	CV_Assert(context.isStereo && context.isInit());
	context.renderChunks(imageLs, imageRs, chunkLs, chunkRs, motionThreshold);
}


CV_EXPORTS_W void oclClearPreviousFrames() {
	RenderContext& context = RenderContext::instance();
	context.resetPrevious();
	ocl::finish();
}

static bool oclSelectDevice(string& device) {
	// no platforms
	vector<ocl::PlatformInfo> platformInfos;
	ocl::getPlatfomsInfo(platformInfos);
	if (platformInfos.size() == 0) {
		return false;
	}

	// select GPU devices
	vector<ocl::Device> devices;
	for (ocl::PlatformInfo& platform : platformInfos) {
		for (int i = 0; i < platform.deviceNumber(); i++) {
			ocl::Device device;
			platform.getDevice(device, i);
			if (device.available()
				&& device.type() == CL_DEVICE_TYPE_GPU
				&& (device.globalMemSize() >> 30) >= 4) {
				devices.push_back(device);
			}
		}
	}
	if (devices.size() == 0) {
		return false;
	}

	// select the device that has max global mem size
	string deviceName = devices[0].name();
	size_t maxMemSize = devices[0].globalMemSize();
	for (int i = 1; i < devices.size(); ++i) {
		if (devices[i].globalMemSize() > maxMemSize) {
			deviceName = devices[i].name();
			maxMemSize = devices[i].globalMemSize();
		}
	}
	device = ":GPU:" + deviceName;
}



CV_EXPORTS_W bool oclDeviceAvailable() {
	string device;
	return oclSelectDevice(device);
}


CV_EXPORTS_W bool oclInitialize(const OclInitParameters* params) {
	string device;
	if (!oclSelectDevice(device)) {
		return false;
	}

	char value[64];
	if (!getenv("OPENCV_OPENCL_DEVICE")) {
		snprintf(value, sizeof(value), "OPENCV_OPENCL_DEVICE=%s", device.c_str());
		putenv(value);
	}

	if (ocl::haveOpenCL()) {
		ocl::setUseOpenCL(true);
		if (ocl::haveSVM()) {
			ocl::Context::getDefault().useSVM();
		}
	}
	if (!ocl::useOpenCL()) {
        return false;
    }
	
	size_t maxBufferPoolSize = ocl::Device::getDefault().globalMemSize();
	setBufferPoolSize(maxBufferPoolSize);

	if (params) {
		// pre-alloc buffers and warm up
		int numThreads = 4;
		bool succeed = oclInitBuffers(
			params->numCams,
			Size(params->camImageWidth - params->numNovelViews, params->camImageHeight),
			Size(params->numNovelViews, params->camImageHeight),
			numThreads);
		if (!succeed) {
			return false;
		}
		releaseBufferPool();

		// TODO: check the init parameters
		RenderContext& context = RenderContext::instance();
		context.init(
			params->isStereo,
			params->numCams,
			params->camImageWidth,
			params->camImageHeight,
			params->numNovelViews,
			params->vergeAtInfinitySlabDisplacement);
		context.startThreads(numThreads);
	}
	oclInitGammaLUT();
    return true;
}

CV_EXPORTS_W void oclRelease() {
	RenderContext& context = RenderContext::instance();
	context.stopThreads();
	context.release();
	oclReleaseGammaLUT();
	releaseBufferPool();
	ocl::finish();
}




}	// namespace imvt
}	// namespace ocl
}	// namespace cv


