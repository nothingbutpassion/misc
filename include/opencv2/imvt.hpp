#ifndef __OPENCV_IMVT_HPP__
#define __OPENCV_IMVT_HPP__

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"


namespace cv {
namespace imvt {
						 
enum class DirectionHint { 
    UNKNOWN, 
    RIGHT,
    DOWN,
    LEFT,
    UP 
};


CV_EXPORTS_W void computeOpticalFlow(
    const Mat& I0BGRA,
    const Mat& I1BGRA,
    const Mat& prevFlow,
    const Mat& prevI0BGRA,
    const Mat& prevI1BGRA,
    Mat& flow,
    DirectionHint hint);


CV_EXPORTS_W void computeOpticalFlow(
    const UMat& I0BGRA,
    const UMat& I1BGRA,
    const UMat& prevFlow,
    const UMat& prevI0BGRA,
    const UMat& prevI1BGRA,
    UMat& flow,
    DirectionHint hint);


CV_EXPORTS_W std::pair<Mat, Mat> combineLazyNovelViews(
	const Mat& warpL,
	const Mat& warpR,
	const Mat& imageL,
	const Mat& imageR,
	const Mat& flowLtoR,
	const Mat& flowRtoL);

CV_EXPORTS_W std::pair<UMat, UMat> combineLazyNovelViews(
	const UMat& warpL,
	const UMat& warpR,
	const UMat& imageL,
	const UMat& imageR,
	const UMat& flowLtoR,
	const UMat& flowRtoL);

CV_EXPORTS_W UMat combineLazyNovelViews(
	const UMat& warp,
	const UMat& imageL,
	const UMat& imageR,
	const UMat& flowLtoR,
	const UMat& flowRtoL);

}	// end namespace cv
}	// end namespace imvt


#endif	// end __OPENCV_IMVT_HPP__