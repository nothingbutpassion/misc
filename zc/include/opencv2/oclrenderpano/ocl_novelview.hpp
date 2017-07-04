#ifndef __OCL_NOVELVIEW_HPP__
#define __OCL_NOVELVIEW_HPP__

#include "opencv2/core.hpp"

namespace cv {	
namespace ocl {
namespace imvt {

CV_EXPORTS_W void oclRemap(const UMat& src, UMat& dst, const UMat& map);
CV_EXPORTS_W void oclGetFlowWarpMap(const UMat& flow, UMat& warpMap, float t);
CV_EXPORTS_W void oclCombineNovelViews(
    const UMat& imageL, float blendL, 
	const UMat& imageR, float blendR,
    const UMat& flowLtoR, const UMat& flowRtoL, UMat& blendImage);
CV_EXPORTS_W void oclCombineLazyViews(
    const UMat& imageL, const UMat& imageR, 
    const UMat& flowMagL, const UMat& flowMagR, UMat& blendImage);
CV_EXPORTS_W void oclGetWarpOpticalFlow(const UMat& warpBuffer, UMat& warpFlow);
CV_EXPORTS_W void oclGetWarpComposition(const UMat& warpBuffer, const UMat& warpFlow, UMat& warpComposition, int invertT);
CV_EXPORTS_W void oclGetNovelViewFlowMag(const UMat& warpBuffer, const UMat& warpFlow, UMat& novelView, UMat& flowMag, int invertT);

CV_EXPORTS_W std::pair<UMat, UMat> oclCombineLazyNovelViews(
	const UMat& warpL,
	const UMat& warpR,
	const UMat& imageL,
	const UMat& imageR,
	const UMat& flowLtoR,
	const UMat& flowRtoL);
CV_EXPORTS_W UMat oclCombineLazyNovelViews(
	const UMat& warp,
	const UMat& imageL,
	const UMat& imageR,
	const UMat& flowLtoR,
	const UMat& flowRtoL);
CV_EXPORTS_W void oclCombineNovelViews(
	const UMat& warp,
	const UMat& imageL,
	const UMat& imageR,
	const UMat& flowLtoR,
	const UMat& flowRtoL,
	UMat& combined);
}	// namespace imvt
}	// namespace ocl
}	// namespace cv

#endif	// __OCL_NOVELVIEW_HPP__

