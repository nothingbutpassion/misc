#include <iostream>
#include "precomp.hpp"
#include "opencl_kernels_imvt.hpp"
#include "opencv2/imvt.hpp"
#include "optflow.hpp"

namespace cv {
namespace imvt {

  // compute the flow field that warps image I1 so that it becomes like image I0.
  // I0 and I1 are 1 byte/channel BGRA format, i.e. they have an alpha channel.
  // it may be the case that I0 and I1 are frames in a video sequence, and some form
  // of temporal regularization may be applied to the flow. in this case, prevFlow stores
  // the last frame's flow for the same camera pair, and prevI0BGRA and prevI1BGRA store
  // the previous frame pixel data. note however that all of the prev Mats may be empty,
  // e.g., if this is the first frame of a sequence, or if we are just rendering a photo.
  // implementations may also chose to ignore previous data regardless.
CV_EXPORTS_W void computeOpticalFlow(
    const Mat& I0BGRA,
    const Mat& I1BGRA,
    const Mat& prevFlow,
    const Mat& prevI0BGRA,
    const Mat& prevI1BGRA,
    Mat& flow,
    DirectionHint hint) {
	static OptFlow optflow;
	optflow.computeOpticalFlow(I0BGRA, I1BGRA, prevFlow, prevI0BGRA, prevI1BGRA, flow, hint);
}


// scale operation: dst == src * ration 
CV_EXPORTS_W void oclScale(const UMat& src, UMat& dst, float factor) {
	CV_Assert(src.type() == CV_32FC1 || src.type() == CV_32FC2 || src.type() == CV_32FC4 || src.type() == dst.type());
    string kernelName = string("scale") + (src.type() == CV_32FC1 ? "_32FC1" : src.type() == CV_32FC2 ? "_32FC2" : "_32FC4");
    ocl::Kernel k(kernelName.c_str(), ocl::imvt::optflow_oclsrc);
    k.args(ocl::KernelArg::ReadWriteNoSize(src), ocl::KernelArg::ReadWriteNoSize(dst), ocl::KernelArg::Constant(&factor, sizeof(factor)));
    size_t globalsize[] = {src.cols, src.rows};
    k.run(2, globalsize, NULL, false);
}


// do motion detection vs. previous frame's images
CV_EXPORTS_W void oclMotionDetection(const UMat& cur, const UMat& pre, UMat& motion) {
    ocl::Kernel k("motion_detection", ocl::imvt::optflow_oclsrc);
    k.args(ocl::KernelArg::ReadOnlyNoSize(cur), ocl::KernelArg::ReadOnlyNoSize(pre), ocl::KernelArg::WriteOnlyNoSize(motion));
    size_t globalsize[] = {motion.cols, motion.rows};
    k.run(2, globalsize, NULL, false);
}


CV_EXPORTS_W void oclAdjustFlowTowardPrevious(const UMat& prevFlow, const UMat& motion, UMat& flow) {

}



struct OCLOptFlow {
	static constexpr int   kPyrMinImageSize = 24;
	static constexpr int   kPyrMaxLevels = 1000;
	static constexpr float kGradEpsilon = 0.001f; // for finite differences
	static constexpr float kUpdateAlphaThreshold = 0.9f;   // pixels with alpha below this aren't updated by proposals
	static constexpr int   kMedianBlurSize = 5;      // medianBlur max size is 5 pixels for CV_32FC2
	static constexpr int   kPreBlurKernelWidth = 5;
	static constexpr float kPreBlurSigma = 0.25f;  // amount to blur images before pyramids
	static constexpr int   kFinalFlowBlurKernelWidth = 3;
	static constexpr float kFinalFlowBlurSigma = 1.0f;   // blur that is applied to flow at the end after upscaling
	static constexpr int   kGradientBlurKernelWidth = 3;
	static constexpr float kGradientBlurSigma = 0.5f;   // amount to blur image gradients
	static constexpr int   kBlurredFlowKernelWidth = 15;     // for regularization/smoothing/diffusion
	static constexpr float kBlurredFlowSigma = 8.0f;

	// the following values is specified in original implementation
	static constexpr float kPyrScaleFactor = 0.9f;
	static constexpr float kSmoothnessCoef = 0.001f;
	static constexpr float kVerticalRegularizationCoef = 0.01f;
	static constexpr float kHorizontalRegularizationCoef = 0.01f;
	static constexpr float kGradientStepSize = 0.5f;
	static constexpr float kDownscaleFactor = 0.5f;
	static constexpr float kDirectionalRegularizationCoef = 0.0f;
	static constexpr bool  kUseDirectionalRegularization = false;
	static constexpr int   kMaxPercentage = 0;


	// these will be modified when running computeOpticalFlow. it becomes true if prevFlow
	// is non-empty.
	bool usePrevFlowTemporalRegularization = false;



	// compute the flow field that warps image I1 so that it becomes like image I0.
	// I0 and I1 are 1 byte/channel BGRA format, i.e. they have an alpha channel.
	// it may be the case that I0 and I1 are frames in a video sequence, and some form
	// of temporal regularization may be applied to the flow. in this case, prevFlow stores
	// the last frame's flow for the same camera pair, and prevI0BGRA and prevI1BGRA store
	// the previous frame pixel data. note however that all of the prev Mats may be empty,
	// e.g., if this is the first frame of a sequence, or if we are just rendering a photo.
	// implementations may also chose to ignore previous data regardless.
	// CPU Implementation
	void computeOpticalFlow(
		const UMat& rgba0byte,
		const UMat& rgba1byte,
		const UMat& prevFlow,
		const UMat& prevI0BGRA,
		const UMat& prevI1BGRA,
		UMat& flow,
		DirectionHint hint) {

		std::cout << "OCLOptFlow::computeOpticalFlow() is starting ..." << std::endl;

		assert(prevFlow.dims == 0 || prevFlow.size() == rgba0byte.size());

		// pre-scale everything to a smaller size. this should be faster + more stable
		UMat rgba0byteDownscaled, rgba1byteDownscaled, prevFlowDownscaled;
		UMat prevI0BGRADownscaled, prevI1BGRADownscaled;
		cv::Size originalSize = rgba0byte.size();
		cv::Size downscaleSize(rgba0byte.cols * kDownscaleFactor, rgba0byte.rows * kDownscaleFactor);
		resize(rgba0byte, rgba0byteDownscaled, downscaleSize, 0, 0, CV_INTER_CUBIC);
		resize(rgba1byte, rgba1byteDownscaled, downscaleSize, 0, 0, CV_INTER_CUBIC);
		UMat motion(downscaleSize, CV_32F);
		
		if (prevFlow.dims > 0) {
			usePrevFlowTemporalRegularization = true;
			resize(prevFlow, prevFlowDownscaled, downscaleSize, 0, 0, CV_INTER_CUBIC);
            
			/* @deleted 
			prevFlowDownscaled *= float(prevFlowDownscaled.rows) / float(prevFlow.rows); 
            */
            oclScale(prevFlowDownscaled, prevFlowDownscaled, float(prevFlowDownscaled.rows)/float(prevFlow.rows));
			resize(prevI0BGRA, prevI0BGRADownscaled, downscaleSize, 0, 0, CV_INTER_CUBIC);
			resize(prevI1BGRA, prevI1BGRADownscaled, downscaleSize, 0, 0, CV_INTER_CUBIC);

			// do motion detection vs. previous frame's images
			/* @deleted
			for (int y = 0; y < rgba0byteDownscaled.rows; ++y) {
				for (int x = 0; x < rgba0byteDownscaled.cols; ++x) {
					motion.at<float>(y, x) =
						(fabs(rgba1byteDownscaled.at<Vec4b>(y, x)[0] - prevI1BGRADownscaled.at<Vec4b>(y, x)[0]) +
							fabs(rgba1byteDownscaled.at<Vec4b>(y, x)[1] - prevI1BGRADownscaled.at<Vec4b>(y, x)[1]) +
							fabs(rgba1byteDownscaled.at<Vec4b>(y, x)[2] - prevI1BGRADownscaled.at<Vec4b>(y, x)[2])) / (255.0f * 3.0f);
				}
			}
			*/
			oclMotionDetection(rgba1byteDownscaled, prevI1BGRADownscaled, motion);
		}

		// convert to various color spaces
		UMat I0Grey, I1Grey, I0, I1, alpha0, alpha1;
		vector<UMat> channels0, channels1;
		split(rgba0byteDownscaled, channels0);
		split(rgba1byteDownscaled, channels1);
		cvtColor(rgba0byteDownscaled, I0Grey, CV_BGRA2GRAY);
		cvtColor(rgba1byteDownscaled, I1Grey, CV_BGRA2GRAY);

		I0Grey.convertTo(I0, CV_32F);
		I1Grey.convertTo(I1, CV_32F);
		/* @deleted
		I0 /= 255.0f;
		I1 /= 255.0f;
		*/
        oclScale(I0, I0, 1.0f/255.0f);
        oclScale(I1, I1, 1.0f/255.0f);
        
		
		channels0[3].convertTo(alpha0, CV_32F);
		channels1[3].convertTo(alpha1, CV_32F);
		/* @deleted
		alpha0 /= 255.0f;
		alpha1 /= 255.0f;
		*/
		oclScale(alpha0, alpha0, 1.0f/255.0f);
		oclScale(alpha1, alpha1, 1.0f/255.0f);
		
		GaussianBlur(I0, I0, Size(kPreBlurKernelWidth, kPreBlurKernelWidth), kPreBlurSigma);
		GaussianBlur(I1, I1, Size(kPreBlurKernelWidth, kPreBlurKernelWidth), kPreBlurSigma);

		vector<UMat> pyramidI0 = buildPyramid(I0);
		vector<UMat> pyramidI1 = buildPyramid(I1);
		vector<UMat> pyramidAlpha0 = buildPyramid(alpha0);
		vector<UMat> pyramidAlpha1 = buildPyramid(alpha1);
		vector<UMat> prevFlowPyramid = buildPyramid(prevFlowDownscaled);
		vector<UMat> motionPyramid = buildPyramid(motion);

       
		if (usePrevFlowTemporalRegularization) {
			// rescale the previous flow values at each level of the pyramid
			for (int level = 0; level < prevFlowPyramid.size(); ++level) {
				/* @deleted
				prevFlowPyramid[level] *= float(prevFlowPyramid[level].rows) / float(prevFlowPyramid[0].rows);
				*/
				oclScale(prevFlowPyramid[level], prevFlowPyramid[level], float(prevFlowPyramid[level].rows)/float(prevFlowPyramid[0].rows) );
			}
		}

		flow = UMat();

		for (int level = pyramidI0.size() - 1; level >= 0; --level) {
			patchMatchPropagationAndSearch(
				pyramidI0[level],
				pyramidI1[level],
				pyramidAlpha0[level],
				pyramidAlpha1[level],
				flow,
				hint);
			
			if (usePrevFlowTemporalRegularization) {
				/* @deleted
				adjustFlowTowardPrevious(prevFlowPyramid[level], motionPyramid[level], flow);
				*/
				oclAdjustFlowTowardPrevious(prevFlow, motion, flow);
			}
			
			if (level > 0) { // scale the flow up to the next size
				resize(flow, flow, pyramidI0[level - 1].size(), 0, 0, CV_INTER_CUBIC);
				/* @deleted
				flow *= (1.0f / kPyrScaleFactor);
				*/
				oclScale(flow, flow, 1.0f/kPyrScaleFactor);
			}
			
		}
		

		// scale the flow result back to full size
		resize(flow, flow, originalSize, 0, 0, CV_INTER_LINEAR);
		/* @deleted
		flow *= (1.0f / kDownscaleFactor);
		*/
		oclScale(flow, flow, 1.0f/kDownscaleFactor);
		GaussianBlur(
			flow,
			flow,
			Size(kFinalFlowBlurKernelWidth, kFinalFlowBlurKernelWidth),
			kFinalFlowBlurSigma);
		

		std::cout << "OCLOptFlow::computeOpticalFlow()  is end !" << std::endl;
	}


    vector<UMat> buildPyramid(const UMat& src) {
        vector<UMat> pyramid = {src};
        while (pyramid.size() < kPyrMaxLevels) {
            Size newSize(pyramid.back().cols * kPyrScaleFactor + 0.5f, pyramid.back().rows * kPyrScaleFactor + 0.5f);
            if (newSize.height <= kPyrMinImageSize || newSize.width <= kPyrMinImageSize) {
                break;
            }
            UMat scaledImage;
            resize(pyramid.back(), scaledImage, newSize, 0, 0, CV_INTER_LINEAR);
            pyramid.push_back(scaledImage);
        }
        return pyramid;
    }

	void patchMatchPropagationAndSearch(
		const UMat& I0,
		const UMat& I1,
		const UMat& alpha0,
		const UMat& alpha1,
		UMat& flow,
		DirectionHint hint) {

		// image gradients
		Mat I0x, I0y, I1x, I1y;
		const int kSameDepth = -1; // same depth as source image
		const int kKernelSize = 1;
		Sobel(I0, I0x, kSameDepth, 1, 0, kKernelSize, 1, 0.0f, BORDER_REPLICATE);
		Sobel(I0, I0y, kSameDepth, 0, 1, kKernelSize, 1, 0.0f, BORDER_REPLICATE);
		Sobel(I1, I1x, kSameDepth, 1, 0, kKernelSize, 1, 0.0f, BORDER_REPLICATE);
		Sobel(I1, I1y, kSameDepth, 0, 1, kKernelSize, 1, 0.0f, BORDER_REPLICATE);

		// blur gradients
		const cv::Size kGradientBlurSize(kGradientBlurKernelWidth, kGradientBlurKernelWidth);
		GaussianBlur(I0x, I0x, kGradientBlurSize, kGradientBlurSigma);
		GaussianBlur(I0y, I0y, kGradientBlurSize, kGradientBlurSigma);
		GaussianBlur(I1x, I1x, kGradientBlurSize, kGradientBlurSigma);
		GaussianBlur(I1y, I1y, kGradientBlurSize, kGradientBlurSigma);
		/*@fixing
		if (flow.empty()) {
			// initialize to all zeros
			flow = Mat::zeros(I0.size(), CV_32FC2);
			// optionally look for a better flow
			if (kMaxPercentage > 0 && hint != DirectionHint::UNKNOWN) {
				adjustInitialFlow(I0, I1, alpha0, alpha1, flow, hint);
			}
		}

		// blur flow. we will regularize against this
		Mat blurredFlow;
		GaussianBlur(
			flow,
			blurredFlow,
			cv::Size(kBlurredFlowKernelWidth, kBlurredFlowKernelWidth),
			kBlurredFlowSigma);

		const cv::Size imgSize = I0.size();

		// sweep from top/left
		for (int y = 0; y < imgSize.height; ++y) {
			for (int x = 0; x < imgSize.width; ++x) {
				if (alpha0.at<float>(y, x) > kUpdateAlphaThreshold && alpha1.at<float>(y, x) > kUpdateAlphaThreshold) {
					float currErr = errorFunction(I0, I1, alpha0, alpha1, I0x, I0y, I1x, I1y, x, y, flow, blurredFlow, flow.at<Point2f>(y, x));
					if (x > 0) { proposeFlowUpdate(alpha0, alpha1, I0, I1, I0x, I0y, I1x, I1y, flow, blurredFlow, currErr, x, y, flow.at<Point2f>(y, x - 1)); }
					if (y > 0) { proposeFlowUpdate(alpha0, alpha1, I0, I1, I0x, I0y, I1x, I1y, flow, blurredFlow, currErr, x, y, flow.at<Point2f>(y - 1, x)); }
					flow.at<Point2f>(y, x) -= kGradientStepSize * errorGradient(I0, I1, alpha0, alpha1, I0x, I0y, I1x, I1y, x, y, flow, blurredFlow, currErr);
				}
			}
		}
		medianBlur(flow, flow, kMedianBlurSize);

		// sweep from bottom/right
		for (int y = imgSize.height - 1; y >= 0; --y) {
			for (int x = imgSize.width - 1; x >= 0; --x) {
				if (alpha0.at<float>(y, x) > kUpdateAlphaThreshold && alpha1.at<float>(y, x) > kUpdateAlphaThreshold) {
					float currErr = errorFunction(I0, I1, alpha0, alpha1, I0x, I0y, I1x, I1y, x, y, flow, blurredFlow, flow.at<Point2f>(y, x));
					if (x < imgSize.width - 1) { proposeFlowUpdate(alpha0, alpha1, I0, I1, I0x, I0y, I1x, I1y, flow, blurredFlow, currErr, x, y, flow.at<Point2f>(y, x + 1)); }
					if (y < imgSize.height - 1) { proposeFlowUpdate(alpha0, alpha1, I0, I1, I0x, I0y, I1x, I1y, flow, blurredFlow, currErr, x, y, flow.at<Point2f>(y + 1, x)); }
					flow.at<Point2f>(y, x) -= kGradientStepSize * errorGradient(I0, I1, alpha0, alpha1, I0x, I0y, I1x, I1y, x, y, flow, blurredFlow, currErr);
				}
			}
		}
		medianBlur(flow, flow, kMedianBlurSize);
		lowAlphaFlowDiffusion(alpha0, alpha1, flow);
		*/
	}

    
};



CV_EXPORTS_W void computeOpticalFlow(
    const UMat& I0BGRA,
    const UMat& I1BGRA,
    const UMat& prevFlow,
    const UMat& prevI0BGRA,
    const UMat& prevI1BGRA,
    UMat& flow,
    DirectionHint hint) {
	static OCLOptFlow oclOptFlow;
	oclOptFlow.computeOpticalFlow(I0BGRA, I1BGRA, prevFlow, prevI0BGRA, prevI1BGRA, flow, hint);
}


  


}	// end namespace imvt
} 	// end namespace cv
