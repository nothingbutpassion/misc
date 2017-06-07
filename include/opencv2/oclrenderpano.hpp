#ifndef __OCL_RENDER_PANO_HPP__
#define __OCL_RENDER_PANO_HPP__

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

namespace cv {
namespace ocl {
namespace imvt {

/**
* @brief Render each imageLs[i] and imageRs[i] to generate chunks[i].
*  
* @param imageLs				the left overlap images.
* @param imageRs				the right overlap images.
* @param chunks					return the generated panorama chunks.
* @param motionThreshhold		the motion threshhold for computing the optical flow. 
*
* @note 1) This function will increase the reference count of imageLs[i] and imageRs[i] to reserve these images for next invokation.  
*		   Call clearPreviousFrames() to clear these reserved frame buffers if necessary.
*		2) The input chunks are not used (or ignored), and will be replaced with the output chunks. 
*/
CV_EXPORTS_W void oclRenderStereoPanoramaChunks(
	const std::vector<UMat>& imageLs,
	const std::vector<UMat>& imageRs,
	std::vector<UMat>& chunks,
	float motionThreshold = 1.0f);


/**
* @brief Clear the previous frame buffers that reserved by oclRenderStereoPanoramaChunks();
*/
CV_EXPORTS_W void oclClearPreviousFrames();


CV_EXPORTS_W bool oclPreColorAdjustByGamma(
	std::vector<UMat>& spheres,
	int standard,
	float project_width_degree,
	float adjust_ratio,
	bool save_debug,
	bool mean_color,
	bool is_hdr);

/**
* @brief Initialize OpenCL: check device avaliablity and alloc related resource
*
* @note This is function must be called before oclRenderStereoPanoramaChunks()/oclRelease().
* @return true if succeed, otherwise false.
*/
CV_EXPORTS_W bool oclInitialize();

/**
* @brief Release the resource allocated by oclInitialize()
*/
CV_EXPORTS_W void oclRelease();




}	// namespace imvt
}	// namespace ocl
}	// namespace cv


#endif	// __OCL_RENDER_PANO_HPP__