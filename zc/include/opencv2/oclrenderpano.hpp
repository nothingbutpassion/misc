#ifndef __OCL_RENDER_PANO_HPP__
#define __OCL_RENDER_PANO_HPP__

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

namespace cv {
namespace ocl {
namespace imvt {

struct OclOptFlowSmooth3Lines {
	float of_a_x;
	float of_a_y;
	float of_b_x;
	float of_b_y;
	float of_0a_factor;
	float of_ab_factor;
	float of_b1_factor;

	OclOptFlowSmooth3Lines() {};
	OclOptFlowSmooth3Lines(
		float of_a_x, float of_a_y, float of_b_x, float of_b_y, 
		float of_0a_factor, float of_ab_factor, float of_b1_factor) {	
		this->of_a_x = of_a_x;
		this->of_a_y = of_a_y;
		this->of_b_x = of_b_x;
		this->of_b_y = of_b_y;
		this->of_0a_factor = of_0a_factor;
		this->of_ab_factor = of_ab_factor;
		this->of_b1_factor = of_b1_factor;
	}
};

/**
* @brief The parameters used for initializing OpenCL.
*/
struct OclInitParameters {
	bool isMonoMode = true;

	int  numSideCams;
	int  numNovelViews;
	int  camImageWidth;

	Size opticalFlowSize;
	OclOptFlowSmooth3Lines smooth3LinesFactor;

	float vergeAtInfinitySlabDisplacement = 0.0f;
	float inputMotionThreshold = 0.0f;
	bool usingBilateralFilter = false;
	bool computeMotionUsingLpair = true;

	// 
	// @unnecessary
	//
	// bool isOpticalFlow = true;
	// int outputResolution = 1;
	// std::string flowAlgName = "pixflow_low";
};


/**
* @brief Check whether there are proper OpenCL devices available.
*/
CV_EXPORTS_W bool oclDeviceAvailable();


/**
* @brief Initialize OpenCL: check device avaliablity and alloc related resource.
*
* 
* @parma initParams	the parameters used for initializing OpenCL.
*
* @return true if succeed, otherwise false.
*
* @note This function must be called before any UMat operations.
*/
CV_EXPORTS_W bool oclInitialize(const OclInitParameters* initParams = nullptr);


/**
* @brief Release OpenCL-related resource
*/
CV_EXPORTS_W void oclRelease();


/**
* @brief Remap each image in srcImages with specified x/y map
*
* @param srcImages	the input images to be remaped, (type must be CV_8UC3 or CV_8UC4)
* @param xmap		the x direction map (type must be CV_32FC1)
* @param ymap		the y direction map.(type must be CV_32FC1)
* @param dstImages	return the remapped images (type is CV_8UC4)
*
*/
CV_EXPORTS_W void oclProjection(
	const std::vector<UMat>& srcImages,
	const std::vector<UMat>& xmap,
	const std::vector<UMat>& ymap,
	std::vector<UMat>& dstImages);

/**
* @brief Pre-adjust images color by gamma method
*/
CV_EXPORTS_W bool oclPreColorAdjustByGamma(
	std::vector<UMat>& spheres,
	int standard,
	float project_width_degree,
	float adjust_ratio,
	bool save_debug,
	bool mean_color,
	bool is_hdr);


/**
* @brief Render each imageLs[i] and imageRs[i] to generate chunks[i].
*  
* @param imageLs				the left overlap images.
* @param imageRs				the right overlap images.
* @param chunks					return the generated panorama chunks.
* @param motionThreshhold		the motion threshhold for computing the optical flow. 
*
* @note This function reserve the copy of imageLs[i]/imageRs[i] for next invokation.  
*		You can call clearPreviousFrames() to clear these reserved frame buffers if necessary.
*/
CV_EXPORTS_W void oclRenderStereoPanoramaChunks(
	const std::vector<UMat>& imageLs,
	const std::vector<UMat>& imageRs,
	std::vector<UMat>& chunks,
	float motionThreshold = 1.0f);

CV_EXPORTS_W void oclRenderStereoPanoramaChunks(
	const std::vector<UMat>& imageLs,
	const std::vector<UMat>& imageRs,
	std::vector<UMat>& chunkLs,
	std::vector<UMat>& chunkRs,
	float motionThreshold = 1.0f);


/**
* @brief Clear the previous frame buffers reserved by oclRenderStereoPanoramaChunks().
*/
CV_EXPORTS_W void oclClearPreviousFrames();


/**
* @brief Applies horizontal concatenation to given matrices.
*
* @param srcImages	input array or vector of matrices. all of the matrices must have the same number of rows and the same depth (CV_8UC4).
* @param dstImage	output array. It has the same number of rows and depth as the src, and the sum of cols of the src.
*/
CV_EXPORTS_W void oclStackHorizontal(
	const std::vector<UMat>& srcImages,
	UMat& dstImage);


/**
* @brief Applies vertical concatenation to given matrices.
*
* @param srcImages	input array or vector of matrices. all of the matrices must have the same number of rows and the same depth (CV_8UC4).
* @param dstImage	output array. It has the same number of rows and depth as the src, and the sum of cols of the src.
*/
CV_EXPORTS_W void oclStackVertical(
	const std::vector<UMat>& srcImages,
	UMat& dstImage);


/**
* @brief Smooth pano image with previous image and specified threshold.
*/
CV_EXPORTS_W void oclSmoothImage(
	UMat& pano,
	const UMat& previous,
	float thresh_hold,
	bool isPano);

/**
* @brief Sharp the image with specified factor.
*/
CV_EXPORTS_W void oclSharpImage(
	UMat& sphericalImage,
	float factor);

/**
* @brief Remap image with specified offset.
*/
CV_EXPORTS_W void oclOffsetHorizontalWrap(
	UMat& image,
	float offset);

/**
* @brief Remove the chunk lines for input chunks.
*/
CV_EXPORTS_W void oclRemoveChunkLines(std::vector<UMat>& chunks);


}	// namespace imvt
}	// namespace ocl
}	// namespace cv


#endif	// __OCL_RENDER_PANO_HPP__