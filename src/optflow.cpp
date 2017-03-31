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



inline void adjustLocalSize(size_t globalsize[], size_t localsize[])   {
    localsize[0] = globalsize[0]%256 == 0 ? 256 : globalsize[0]%8 == 0 ? 8 : 1;
	localsize[0] = localsize[0] > globalsize[0] ? 1 : localsize[0];
    localsize[1] = globalsize[1]%256 == 0 ? 256 : globalsize[1]%8 == 0 ? 8 : 1;
    localsize[1] = localsize[1] > globalsize[1] ? 1: localsize[0];
}


// scale operation: dst == src * ration 
CV_EXPORTS_W void oclScale(const UMat& src, UMat& dst, float factor) {
	CV_Assert(src.type() == CV_32FC1 || src.type() == CV_32FC2 || src.type() == CV_32FC4 || src.type() == dst.type());
    string kernelName = string("scale") + (src.type() == CV_32FC1 ? "_32FC1" : src.type() == CV_32FC2 ? "_32FC2" : "_32FC4");
    ocl::Kernel k(kernelName.c_str(), ocl::imvt::optflow_oclsrc);
    k.args(ocl::KernelArg::ReadWriteNoSize(src), ocl::KernelArg::ReadWriteNoSize(dst), ocl::KernelArg::Constant(&factor, sizeof(factor)));
    size_t globalsize[] = {src.cols, src.rows};
    size_t localsize[] = {1, 1};
    adjustLocalSize(globalsize, localsize);
    k.run(2, globalsize, NULL, false);
}

// do motion detection vs. previous frame's images
CV_EXPORTS_W void oclMotionDetection(const UMat& cur, const UMat& pre, UMat& motion) {
    ocl::Kernel k("motion_detection", ocl::imvt::optflow_oclsrc);
    k.args(ocl::KernelArg::ReadOnlyNoSize(cur),
        ocl::KernelArg::ReadOnlyNoSize(pre),
        ocl::KernelArg::WriteOnlyNoSize(motion));
    size_t globalsize[] = {motion.cols, motion.rows};
    size_t localsize[] = {1, 1};
    adjustLocalSize(globalsize, localsize);
    k.run(2, globalsize, NULL, false);
}

// adjust flow toward previous
CV_EXPORTS_W void oclAdjustFlowTowardPrevious(const UMat& prevFlow, const UMat& motion, UMat& flow) {
    ocl::Kernel k("adjust_flow_toward_previous", ocl::imvt::optflow_oclsrc);
    k.args(ocl::KernelArg::ReadWriteNoSize(flow),
        ocl::KernelArg::ReadOnlyNoSize(prevFlow),
        ocl::KernelArg::ReadOnlyNoSize(motion));
    size_t globalsize[] = {flow.cols, flow.rows};
    size_t localsize[] = {1, 1};
    adjustLocalSize(globalsize, localsize);
    k.run(2, globalsize, localsize, false);
}

// estimate the flow of each pixel in I0 by searching a rectangle
CV_EXPORTS_W void oclEstimateFlow(const UMat& I0, const UMat& I1, const UMat& alpha0, const UMat& alpha1, UMat& flow, const Rect& box) {
    ocl::Kernel k("estimate_flow", ocl::imvt::optflow_oclsrc);
    k.args(ocl::KernelArg::ReadOnly(I0), 
        ocl::KernelArg::ReadOnly(I1),
        ocl::KernelArg::ReadOnlyNoSize(alpha0), 
        ocl::KernelArg::ReadOnlyNoSize(alpha1),
        ocl::KernelArg::ReadWriteNoSize(flow),
        ocl::KernelArg::Constant(&box.x, sizeof(box.x)),
        ocl::KernelArg::Constant(&box.y, sizeof(box.y)),
        ocl::KernelArg::Constant(&box.width, sizeof(box.width)),
        ocl::KernelArg::Constant(&box.height, sizeof(box.height)));
    size_t globalsize[] = {flow.cols, flow.rows};
    size_t localsize[] = {1, 1};
    adjustLocalSize(globalsize, localsize);
    k.run(2, globalsize, localsize, false);
}

// low alpha flow diffusion
CV_EXPORTS_W void oclAlphaFlowDiffusion(const UMat& alpha0, const UMat& alpha1, const UMat& blurredFlow, UMat& flow) {
    ocl::Kernel k("alpha_flow_diffusion", ocl::imvt::optflow_oclsrc);
    k.args(ocl::KernelArg::ReadOnlyNoSize(alpha0), 
        ocl::KernelArg::ReadOnlyNoSize(alpha1),
        ocl::KernelArg::ReadOnlyNoSize(blurredFlow),
        ocl::KernelArg::ReadWriteNoSize(flow));
    size_t globalsize[] = {flow.cols, flow.rows};
    size_t localsize[] = {1, 1};
    adjustLocalSize(globalsize, localsize);
    k.run(2, globalsize, localsize, false);
}

// sweep from top/left
CV_EXPORTS_W void oclSweepFromTopLeft(
    const UMat& I0, const UMat& I1, const UMat& alpha0, const UMat& alpha1, 
    const UMat& I0x, const UMat& I0y, const UMat& I1x, const UMat& I1y, const UMat&  blurredFlow, UMat& flow) {
    

    int minrc = I0.rows < I0.cols ?  I0.rows : I0.cols;
	// vector<ocl::Kernel*> kernels;
    for (int i=0; i < I0.rows + I0.cols - 1; ++i) {
        int start_y = i < I0.rows ? i : I0.rows - 1;  
        int start_x = i - start_y;
		ocl::Kernel k("sweep_from_top_left", ocl::imvt::optflow_oclsrc);
        k.args(ocl::KernelArg::ReadOnlyNoSize(I0), 
            ocl::KernelArg::ReadOnlyNoSize(I1),
            ocl::KernelArg::ReadOnlyNoSize(alpha0),
            ocl::KernelArg::ReadOnlyNoSize(alpha1),
            ocl::KernelArg::ReadOnlyNoSize(I0x),
            ocl::KernelArg::ReadOnlyNoSize(I0y),
            ocl::KernelArg::ReadOnlyNoSize(I1x),
            ocl::KernelArg::ReadOnlyNoSize(I1y),
            ocl::KernelArg::ReadOnlyNoSize(blurredFlow),
            ocl::KernelArg::ReadWrite(flow),
            ocl::KernelArg::Constant(&start_x, sizeof(start_x)),
            ocl::KernelArg::Constant(&start_y, sizeof(start_y)));

        size_t globalsize[] ={(i < minrc ? i+1 : I0.rows+I0.cols-1-i < minrc ? I0.rows+I0.cols-1-i : minrc), 1};
        size_t localsize[] = {1, 1};
        adjustLocalSize(globalsize, localsize);
        k.run(1, globalsize, localsize, false);
		// cout << flow.getMat(ACCESS_READ) << endl;
    }
}

// sweep from bottom/right
CV_EXPORTS_W void oclSweepFromBottomRight(
    const UMat& I0, const UMat& I1, const UMat& alpha0, const UMat& alpha1, 
    const UMat& I0x, const UMat& I0y, const UMat& I1x, const UMat& I1y, const UMat&  blurredFlow, UMat& flow) {
    ocl::Kernel k("sweep_from_bottom_right", ocl::imvt::optflow_oclsrc);
    
    int minrc = I0.rows < I0.cols ?  I0.rows : I0.cols;    
    for (int i=I0.rows + I0.cols - 2; i >= 0 ; --i) {
        int start_y = i < I0.rows ? i : I0.rows - 1;  
        int start_x = i - start_y;
        k.args(ocl::KernelArg::ReadOnlyNoSize(I0), 
            ocl::KernelArg::ReadOnlyNoSize(I1),
            ocl::KernelArg::ReadOnlyNoSize(alpha0),
            ocl::KernelArg::ReadOnlyNoSize(alpha1),
            ocl::KernelArg::ReadOnlyNoSize(I0x),
            ocl::KernelArg::ReadOnlyNoSize(I0y),
            ocl::KernelArg::ReadOnlyNoSize(I1x),
            ocl::KernelArg::ReadOnlyNoSize(I1y),
            ocl::KernelArg::ReadOnlyNoSize(blurredFlow),
            ocl::KernelArg::ReadWrite(flow),
            ocl::KernelArg::Constant(&start_x, sizeof(start_x)),
            ocl::KernelArg::Constant(&start_y, sizeof(start_y)));
        size_t globalsize[] ={(i < minrc ? i+1 : I0.rows+I0.cols-1-i < minrc ? I0.rows+I0.cols-1-i : minrc), 1};
        size_t localsize[] = {1, 1};
        adjustLocalSize(globalsize, localsize);
        k.run(1, globalsize, localsize, false);

		//cout << flow.getMat(ACCESS_READ) << endl;
    }   
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
				oclAdjustFlowTowardPrevious(prevFlowPyramid[level], motionPyramid[level], flow);
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
		UMat I0x, I0y, I1x, I1y;
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
		
		if (flow.empty()) {
			// initialize to all zeros
			flow = UMat::zeros(I0.size(), CV_32FC2);
			// optionally look for a better flow
			if (kMaxPercentage > 0 && hint != DirectionHint::UNKNOWN) {
				adjustInitialFlow(I0, I1, alpha0, alpha1, flow, hint);
			}
		}

        
		// blur flow. we will regularize against this
		UMat blurredFlow;
		GaussianBlur(
			flow,
			blurredFlow,
			cv::Size(kBlurredFlowKernelWidth, kBlurredFlowKernelWidth),
			kBlurredFlowSigma);

		const cv::Size imgSize = I0.size();
		
		
		/* @deleted
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
		*/
		oclSweepFromTopLeft(I0, I1, alpha0, alpha1, I0x, I0y, I1x, I1y, blurredFlow, flow);
		medianBlur(flow, flow, kMedianBlurSize);

		/* @deleted
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
		*/
        oclSweepFromBottomRight(I0, I1, alpha0, alpha1, I0x, I0y, I1x, I1y, blurredFlow, flow);
		medianBlur(flow, flow, kMedianBlurSize);
        
		lowAlphaFlowDiffusion(alpha0, alpha1, flow);
	}

    void adjustInitialFlow(
        const UMat& I0,
        const UMat& I1,
        const UMat& alpha0,
        const UMat& alpha1,
        UMat& flow,
        const DirectionHint hint) {
        
        // compute a version of I1 that matches I0's intensity
        // this is basically poor man's color correction
        /* @deleted
        Mat I1eq = I1 * computeIntensityRatio(I0, alpha0, I1, alpha1);
        */
		float ratio = computeIntensityRatio(I0.getMat(ACCESS_READ), alpha0.getMat(ACCESS_READ), 
			I1.getMat(ACCESS_READ), alpha1.getMat(ACCESS_READ));
		UMat I1eq = UMat::zeros(I1.size(), I1.type());
		oclScale(I1, I1eq, ratio);
        
        // estimate the flow of each pixel in I0 by searching a rectangle
        Rect box = computeSearchBox(hint);
        /* @deleted
        for (int i0y = 0; i0y < I0.rows; ++i0y) {
            for (int i0x = 0; i0x < I0.cols; ++i0x) {
                if (alpha0.at<float>(i0y, i0x) > kUpdateAlphaThreshold) {
                    // create affinity for (0,0) by using fraction of the actual error
                    float kFraction = 0.8f; // lower the fraction to increase affinity
                    float errorBest = kFraction * computePatchError(I0, alpha0, i0x, i0y, I1eq, alpha1, i0x, i0y);
                    int i1xBest = i0x, i1yBest = i0y;
                    // look for better patch in the box
                    for (int dy = box.y; dy < box.y + box.height; ++dy) {
                        for (int dx = box.x; dx < box.x + box.width; ++dx) {
                            int i1x = i0x + dx;
                            int i1y = i0y + dy;
                            if (0 <= i1x && i1x < I1.cols && 0 <= i1y && i1y < I1.rows) {
                                float error = computePatchError(I0, alpha0, i0x, i0y, I1eq, alpha1, i1x, i1y);
                                if (errorBest > error) {
                                    errorBest = error;
                                    i1xBest = i1x;
                                    i1yBest = i1y;
                                }
                            }
                        }
                    }
                    // use the best match
                    flow.at<Point2f>(i0y, i0x) = Point2f(i1xBest - i0x, i1yBest - i0y);
                }
            }
        }
        */
        oclEstimateFlow(I0, I1, alpha0, alpha1, flow, box);
        
    }

    
    float computeIntensityRatio(
        const Mat& lhs, const Mat& lhsAlpha,
        const Mat& rhs, const Mat& rhsAlpha) {
        // just scale by the ratio between the sums, attenuated by alpha
        //CHECK_EQ(lhs.size(), rhs.size());
        float sumLhs = 0;
        float sumRhs = 0;
        for (int y = 0; y < lhs.rows; ++y) {
            for (int x = 0; x < lhs.cols; ++x) {
                float alpha = lhsAlpha.at<float>(y, x) * rhsAlpha.at<float>(y, x);
                sumLhs += alpha * lhs.at<float>(y, x);
                sumRhs += alpha * rhs.at<float>(y, x);
            }
        }
        return sumLhs / sumRhs;
    }


    Rect computeSearchBox(DirectionHint hint) {
        // we search a rectangle that is a fraction of the coarsest pyramid level
        const int dist = computeSearchDistance();
        // the rectangle extends ortho to each side of the search direction
        static const int kRatio = 8; // aspect ratio of search box
        const int ortho = (dist + kRatio / 2) / kRatio;
        const int thickness = 2 * ortho + 1;
        switch (hint) {
            // opencv rectangles are left, top, width, height
            case DirectionHint::RIGHT: return Rect(0, -ortho, dist + 1, thickness);
            case DirectionHint::DOWN: return Rect(-ortho, 0, thickness, dist + 1);
            case DirectionHint::LEFT: return Rect(-dist, -ortho, dist + 1, thickness);
            case DirectionHint::UP: return Rect(-ortho, -dist, thickness, dist + 1);
            case DirectionHint::UNKNOWN: break; // silence warning
        }
        //LOG(FATAL) << "unexpected direction " << int(hint);
        return Rect();
    }

    static inline int computeSearchDistance() {
        // we search a fraction of the coarsest pyramid level
        return (kPyrMinImageSize * kMaxPercentage + 50) / 100;
    }


    void lowAlphaFlowDiffusion(const UMat& alpha0, const UMat& alpha1, UMat& flow) {
        UMat blurredFlow;
        GaussianBlur(
            flow,
            blurredFlow,
            Size(kBlurredFlowKernelWidth, kBlurredFlowKernelWidth),
            kBlurredFlowSigma);
        /* @deleted
        for (int y = 0; y < flow.rows; ++y) {
            for (int x = 0; x < flow.cols; ++x) {
                const float a0 = alpha0.at<float>(y, x);
                const float a1 = alpha1.at<float>(y, x);
                const float diffusionCoef = 1.0f - a0 * a1;
                flow.at<Point2f>(y, x) = diffusionCoef * blurredFlow.at<Point2f>(y, x) + (1.0f - diffusionCoef) * flow.at<Point2f>(y, x);
            }
        }
        */
        oclAlphaFlowDiffusion(alpha0, alpha1, blurredFlow, flow);
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
