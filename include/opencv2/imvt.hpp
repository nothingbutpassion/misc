/*
By downloading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.


                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2013, OpenCV Foundation, all rights reserved.
Third party copyrights are property of their respective owners.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the names of the copyright holders nor the names of the contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as is" and
any express or implied warranties, including, but not limited to, the implied
warranties of merchantability and fitness for a particular purpose are
disclaimed. In no event shall copyright holders or contributors be liable for
any direct, indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/

#ifndef __OPENCV_IMVT_HPP__
#define __OPENCV_IMVT_HPP__

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"



namespace cv {
namespace imvt {

CV_EXPORTS_W void remap(
    InputArray src, 
    OutputArray dst,
    InputArray map1, 
    InputArray map2,
    int interpolation,
    int borderMode = BORDER_CONSTANT,
    const Scalar& borderValue = Scalar());


						 
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


}	// end namespace cv
}	// end namespace imvt



#endif	// end __OPENCV_IMVT_HPP__