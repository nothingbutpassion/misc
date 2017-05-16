/**
* Copyright (c) 2016-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE_render file in the root directory of this subproject. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include <string>
#include <vector>
#include "precomp.hpp"
#include "opencl_kernels_imvt.hpp"
#include "opencv2/imvt.hpp"
#include "surround360_novelview.hpp"


namespace cv {
namespace imvt {

using namespace std;
using namespace cv;

CV_EXPORTS_W void oclRemap(const UMat& src, UMat& dst, const UMat& map) {
    CV_Assert(src.type() == CV_8UC4 || src.type() == CV_32FC2);
    CV_Assert(map.type() == CV_32FC2);
	UMat s = src;
    dst.create(src.size(), s.type()); 
    string srcType = s.type() == CV_8UC4 ? "_8UC4" : "_32FC2";
    string mapType = "_32FC2";
    string kernelName = string("remap") + srcType + mapType;
    ocl::Kernel k(kernelName.c_str(), ocl::imvt::remap_oclsrc);
    k.args(ocl::KernelArg::ReadOnly(src),
        ocl::KernelArg::WriteOnly(dst), 
        ocl::KernelArg::ReadOnlyNoSize(map));
    size_t globalsize[] = {dst.cols, dst.rows};
    size_t localsize[] = {16, 16};
    k.run(2, globalsize, localsize, false);
}

CV_EXPORTS_W void oclGetFlowWarpMap(const UMat& flow, UMat& warpMap, double t) {
    CV_Assert(flow.size() == warpMap.size());
    ocl::Kernel k("get_flow_warp_map", ocl::imvt::novelview_oclsrc);
    k.args(ocl::KernelArg::ReadOnlyNoSize(flow),
		ocl::KernelArg::WriteOnly(warpMap),
        ocl::KernelArg::Constant(&t, sizeof(t)));
    size_t globalsize[] = {warpMap.cols, warpMap.rows};
    size_t localsize[] = {16, 16};
    k.run(2, globalsize, localsize, false);
}

CV_EXPORTS_W void oclCombineNovelViews(
    const UMat& imageL, float blendL,
    const UMat& imageR, float blendR,
    const UMat& flowLtoR, const UMat& flowRtoL, UMat& blendImage)  {
    ocl::Kernel k("combine_novel_views", ocl::imvt::novelview_oclsrc);
    k.args(ocl::KernelArg::ReadOnlyNoSize(imageL),
        ocl::KernelArg::ReadOnlyNoSize(imageR),
        ocl::KernelArg::ReadOnlyNoSize(flowLtoR),
        ocl::KernelArg::ReadOnlyNoSize(flowRtoL),
        ocl::KernelArg::WriteOnly(blendImage),
        ocl::KernelArg::Constant(&blendL, sizeof(blendL)),
        ocl::KernelArg::Constant(&blendR, sizeof(blendR)));
    size_t globalsize[] = {blendImage.cols, blendImage.rows};
    size_t localsize[] = {16, 16};
    k.run(2, globalsize, localsize, false);
}

CV_EXPORTS_W void oclCombineLazyViews(
    const UMat& imageL, const UMat& imageR, 
    const UMat& flowMagL, const UMat& flowMagR, UMat& blendImage) {
    ocl::Kernel k("combine_lazy_views", ocl::imvt::novelview_oclsrc);
    k.args(ocl::KernelArg::ReadOnlyNoSize(imageL),
		ocl::KernelArg::ReadOnlyNoSize(imageR),
        ocl::KernelArg::ReadOnlyNoSize(flowMagL),
        ocl::KernelArg::ReadOnlyNoSize(flowMagR),
		ocl::KernelArg::WriteOnly(blendImage));
    size_t globalsize[] = {blendImage.cols, blendImage.rows};
    size_t localsize[] = {16, 16};
    k.run(2, globalsize, localsize, false);
}

CV_EXPORTS_W void oclGetWarpOpticalFlow(const UMat& warpBuffer, UMat& warpFlow) {
    ocl::Kernel k("get_warp_optical_flow", ocl::imvt::novelview_oclsrc);
    k.args(ocl::KernelArg::ReadOnlyNoSize(warpBuffer),
		ocl::KernelArg::WriteOnly(warpFlow));
    size_t globalsize[] = {warpFlow.cols, warpFlow.rows};
    size_t localsize[] = {16, 16};
    k.run(2, globalsize, localsize, false);
}

CV_EXPORTS_W void oclGetWarpComposition(const UMat& warpBuffer, const UMat& warpFlow, UMat& warpComposition, int invertT) {
    ocl::Kernel k("get_warp_composition", ocl::imvt::novelview_oclsrc);
    k.args(ocl::KernelArg::ReadOnlyNoSize(warpBuffer),
        ocl::KernelArg::ReadOnlyNoSize(warpFlow),
		ocl::KernelArg::WriteOnly(warpComposition),
		ocl::KernelArg::Constant(&invertT, sizeof(invertT)));
    size_t globalsize[] = {warpComposition.cols, warpComposition.rows};
    size_t localsize[] = {16, 16};
    k.run(2, globalsize, localsize, false);
}

CV_EXPORTS_W void oclGetNovelViewFlowMag(const UMat& warpBuffer, const UMat& warpFlow, UMat& novelView, UMat& flowMag, int invertT) {
    ocl::Kernel k("get_novel_view_flow_mag", ocl::imvt::novelview_oclsrc);
    k.args(ocl::KernelArg::ReadOnlyNoSize(warpBuffer),
        ocl::KernelArg::ReadOnlyNoSize(warpFlow),
        ocl::KernelArg::ReadWriteNoSize(novelView),
		ocl::KernelArg::WriteOnly(flowMag),
		ocl::KernelArg::Constant(&invertT, sizeof(invertT)));
    size_t globalsize[] = {flowMag.cols, flowMag.rows};
    size_t localsize[] = {16, 16};
    k.run(2, globalsize, localsize, false);
}


// when rendering panoramas from slices of many novel views, there is lots of
// wasted computation. this is an idea for reducing that computation: build up
// a datastructure of just the pieces of the novel views we need, then do it
// all in 1 pass
struct OCLLazyNovelViewBuffer {
    /* @deleted
    int width, height;
    // warpL[u][v] = (x, y, t). in the final panorama image at pixel coord u, v
    // we will take a piece of the novel view image at x, y, and time shift t.
    vector<vector<Point3f>> warpL;
    vector<vector<Point3f>> warpR;
    

    OCLLazyNovelViewBuffer(int width, int height) {
        this->width = width;
        this->height = height;
        warpL = vector<vector<Point3f>>(width, vector<Point3f>(height));
        warpR = vector<vector<Point3f>>(width, vector<Point3f>(height));
    }
    */
	UMat warpL;
	UMat warpR;
};

static UMat generateNovelViewSimpleCvRemap(const UMat& srcImage, const UMat& flow, double t) {
    /* @deleted
    const int w = srcImage.cols;
    const int h = srcImage.rows;
    Mat warpMap = Mat(Size(w, h), CV_32FC2);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Point2f flowDir = flow.at<Point2f>(y, x);
            warpMap.at<Point2f>(y, x) = Point2f(x + flowDir.x * t, y + flowDir.y * t);
        }
    }
    Mat novelView;
    remap(srcImage, novelView, warpMap, Mat(), CV_INTER_CUBIC);
    */
    UMat warpMap(srcImage.size(), CV_32FC2);
    oclGetFlowWarpMap(flow, warpMap, t);
    UMat novelView;
    oclRemap(srcImage, novelView, warpMap);
    return novelView;
}

struct OCLNovelViewGeneratorLazyFlow {

    UMat imageL, imageR;
    UMat flowLtoR, flowRtoL;
    
    void generateNovelView(
        const double shiftFromL,
        UMat& outNovelViewMerged,
        UMat& outNovelViewFromL,
        UMat& outNovelViewFromR) {
        /* @deleted
        outNovelViewFromL = NovelViewUtil::generateNovelViewSimpleCvRemap(
            imageL, flowRtoL, shiftFromL);

        outNovelViewFromR = NovelViewUtil::generateNovelViewSimpleCvRemap(
            imageR, flowLtoR, 1.0 - shiftFromL);

        outNovelViewMerged = NovelViewUtil::combineNovelViews(
            outNovelViewFromL, 1.0 - shiftFromL,
            outNovelViewFromR, shiftFromL,
            flowLtoR, flowRtoL);
        */
        outNovelViewFromL = generateNovelViewSimpleCvRemap(imageL, flowRtoL, shiftFromL);
        outNovelViewFromR = generateNovelViewSimpleCvRemap(imageR, flowLtoR, 1.0 - shiftFromL);
        UMat outNovelViewMergedTmp(imageL.size(), CV_8UC4);
        oclCombineNovelViews(
            outNovelViewFromL, 1.0 - shiftFromL,
            outNovelViewFromR, shiftFromL,
            flowLtoR, flowRtoL,
            outNovelViewMergedTmp);
        outNovelViewMerged = outNovelViewMergedTmp; 
    }

    pair<UMat, UMat> renderLazyNovelView(
        const UMat& novelViewWarpBuffer,
        const UMat& srcImage,
        const UMat& opticalFlow,
        const bool invertT) {
        
        /* @deleted
        // a composition of remap
        Mat warpOpticalFlow = Mat(Size(width, height), CV_32FC2);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const Point3f lazyWarp = novelViewWarpBuffer[x][y];
                warpOpticalFlow.at<Point2f>(y, x) = Point2f(lazyWarp.x, lazyWarp.y);
            }
        }
        Mat remappedFlow;
        remap(opticalFlow, remappedFlow, warpOpticalFlow, Mat(), CV_INTER_CUBIC);
        */
        UMat warpOpticalFlow(novelViewWarpBuffer.size(), CV_32FC2);
        oclGetWarpOpticalFlow(novelViewWarpBuffer, warpOpticalFlow);
        UMat remappedFlow;
        oclRemap(opticalFlow, remappedFlow, warpOpticalFlow);
        
        /* @deleted
        Mat warpComposition = Mat(Size(width, height), CV_32FC2);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const Point3f lazyWarp = novelViewWarpBuffer[x][y];
                Point2f flowDir = remappedFlow.at<Point2f>(y, x);
                // the 3rd coordinate (z) of novelViewWarpBuffer is shift/time value
                const float t = invertT ? (1.0f - lazyWarp.z) : lazyWarp.z;
                warpComposition.at<Point2f>(y, x) =
                    Point2f(lazyWarp.x + flowDir.x * t, lazyWarp.y + flowDir.y * t);
            }
        }
        Mat novelView;
        remap(srcImage, novelView, warpComposition, Mat(), CV_INTER_CUBIC);
        */
        UMat warpComposition(novelViewWarpBuffer.size(), CV_32FC2);
        oclGetWarpComposition(novelViewWarpBuffer, remappedFlow, warpComposition, invertT ? 1 : 0);
        UMat novelView;
        oclRemap(srcImage, novelView, warpComposition);

        /* @deleted
        Mat novelViewFlowMag(novelView.size(), CV_32F);
        // so far we haven't quite set things up to exactly match the original
        // O(n^3) algorithm. we need to blend the two novel views based on the
        // time shift value. we will pack that into the alpha channel here,
        // then use it to blend the two later.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const Point3f lazyWarp = novelViewWarpBuffer[x][y];
                Point2f flowDir = remappedFlow.at<Point2f>(y, x);
                const float t = invertT ? (1.0f - lazyWarp.z) : lazyWarp.z;
                novelView.at<Vec4b>(y, x)[3] =
                    int((1.0f - t) * novelView.at<Vec4b>(y, x)[3]);
                novelViewFlowMag.at<float>(y, x) =
                    sqrtf(flowDir.x * flowDir.x + flowDir.y * flowDir.y);
            }
        }
        */
        UMat novelViewFlowMag(novelView.size(), CV_32F);
        oclGetNovelViewFlowMag(novelViewWarpBuffer, remappedFlow, novelView, novelViewFlowMag, invertT ? 1 : 0);
        return make_pair(novelView, novelViewFlowMag);
    }


    pair<UMat, UMat> combineLazyNovelViews(
        const OCLLazyNovelViewBuffer& lazyBuffer) {

        // two images for the left eye (to be combined)
        pair<UMat, UMat> leftEyeFromLeft = renderLazyNovelView(
            lazyBuffer.warpL,
            imageL,
            flowRtoL,
            false);
        
        pair<UMat, UMat> leftEyeFromRight = renderLazyNovelView(
            lazyBuffer.warpL,
            imageR,
            flowLtoR,
            true);

        // two images for the right eye (to be combined)
        pair<UMat, UMat> rightEyeFromLeft = renderLazyNovelView(
            lazyBuffer.warpR,
            imageL,
            flowRtoL,
            false);
		pair<UMat, UMat> rightEyeFromRight = renderLazyNovelView(
			lazyBuffer.warpR,
            imageR,
            flowLtoR,
            true);

		UMat leftEyeCombined(leftEyeFromLeft.first.size(), CV_8UC4);
		oclCombineLazyViews(
            leftEyeFromLeft.first,
            leftEyeFromRight.first,
            leftEyeFromLeft.second,
            leftEyeFromRight.second,
			leftEyeCombined);
		UMat rightEyeCombined(rightEyeFromLeft.first.size(), CV_8UC4);
		oclCombineLazyViews(
            rightEyeFromLeft.first,
            rightEyeFromRight.first,
            rightEyeFromLeft.second,
            rightEyeFromRight.second,
			rightEyeCombined);
        return make_pair(leftEyeCombined, rightEyeCombined);
    }
    
    /* @added 
     */
     // NOTES: This function is the special version for the above one where warp == lazyBuffer.warpL == lazyBuffer.warpR.
     //        So, the return value == leftEyeCombined == rightEyeCombined.
    UMat combineLazyNovelViews(const UMat& warp) {
        pair<UMat, UMat> leftEyeFromLeft = renderLazyNovelView(
            warp,
            imageL,
            flowRtoL,
            false);
        
        pair<UMat, UMat> leftEyeFromRight = renderLazyNovelView(
            warp,
            imageR,
            flowLtoR,
            true);
        
		UMat leftEyeCombined(leftEyeFromLeft.first.size(), CV_8UC4);
		oclCombineLazyViews(
            leftEyeFromLeft.first,
            leftEyeFromRight.first,
            leftEyeFromLeft.second,
            leftEyeFromRight.second,
			leftEyeCombined);
        
        return leftEyeCombined; 
    }

    
};

// OpenCL version
CV_EXPORTS_W pair<UMat, UMat> combineLazyNovelViews(
    const UMat& warpL,
    const UMat& warpR,
    const UMat& imageL,
    const UMat& imageR,
    const UMat& flowLtoR,
    const UMat& flowRtoL) {

    OCLLazyNovelViewBuffer lazyBuffer;
    lazyBuffer.warpL = warpL;
	lazyBuffer.warpR = warpR;
    
    OCLNovelViewGeneratorLazyFlow generatorLazyFlow;
    generatorLazyFlow.imageL = imageL;
    generatorLazyFlow.imageR = imageR;
    generatorLazyFlow.flowLtoR = flowLtoR;
    generatorLazyFlow.flowRtoL = flowRtoL;
    
    return generatorLazyFlow.combineLazyNovelViews(lazyBuffer);
}

CV_EXPORTS_W UMat combineLazyNovelViews(
    const UMat& warp,
    const UMat& imageL,
    const UMat& imageR,
    const UMat& flowLtoR,
    const UMat& flowRtoL) {
    OCLNovelViewGeneratorLazyFlow generatorLazyFlow;
    generatorLazyFlow.imageL = imageL;
    generatorLazyFlow.imageR = imageR;
    generatorLazyFlow.flowLtoR = flowLtoR;
    generatorLazyFlow.flowRtoL = flowRtoL;
    return generatorLazyFlow.combineLazyNovelViews(warp);
}


// Facebook version
CV_EXPORTS_W pair<Mat, Mat> combineLazyNovelViews(
    const Mat& warpL,
    const Mat& warpR,
    const Mat& imageL,
    const Mat& imageR,
    const Mat& flowLtoR,
    const Mat& flowRtoL) {

    LazyNovelViewBuffer lazyBuffer(warpL.cols, warpL.rows);
    for (int y = 0; y < warpL.rows; ++y) {
        for (int x = 0; x < warpL.cols; ++x) {
            lazyBuffer.warpL[x][y] = warpL.at<Point3f>(y, x);
            lazyBuffer.warpR[x][y] = warpR.at<Point3f>(y, x);
        }
    }

    NovelViewGeneratorLazyFlow generatorLazyFlow;
	generatorLazyFlow.imageL = imageL;
	generatorLazyFlow.imageR = imageR;
	generatorLazyFlow.flowLtoR = flowLtoR;
	generatorLazyFlow.flowRtoL = flowRtoL;

    return generatorLazyFlow.combineLazyNovelViews(lazyBuffer);
}


} // namespace imvt
} // namespace cv


