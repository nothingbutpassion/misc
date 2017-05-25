/**
 * @brief the following macros are used for accessing img(x, y)
 */
#define rmat8uc4(addr, x, y) ((__global const uchar4*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc1(addr, x, y) ((__global const float*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc2(addr, x, y) ((__global const float2*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc3(addr, x, y) ((__global const float3*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step + (x)*12))[0]
#define wmat8uc4(addr, x, y) ((__global uchar4*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc1(addr, x, y) ((__global float*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc2(addr, x, y) ((__global float2*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]

#define rmat(addr, x, y)	rmat32fc1(addr, x, y)
#define rmat2(addr, x, y) 	rmat32fc2(addr, x, y)
#define rmat3(addr, x, y) 	rmat32fc3(addr, x, y)
#define wmat(addr, x, y)	wmat32fc1(addr, x, y)
#define wmat2(addr, x, y) 	wmat32fc2(addr, x, y)

#define lerp(x0, x1, alpha)	 ((x0)*(1 - (alpha)) + (x1)*(alpha))

/*
Mat warpMap = Mat(Size(w, h), CV_32FC2);
for (int y = 0; y < h; ++y) {
for (int x = 0; x < w; ++x) {
	Point2f flowDir = flow.at<Point2f>(y, x);
	warpMap.at<Point2f>(y, x) = Point2f(x + flowDir.x * t, y + flowDir.y * t);
}
}
*/
__kernel void get_flow_warp_map(
	__global const float2* flow, int flow_step, int flow_offset,
	__global float2* warpMap, int warpMap_step, int warpMap_offset, int warpMap_rows, int warpMap_cols,
	float t)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < warpMap_cols && y < warpMap_rows) {
		wmat2(warpMap, x, y) = (float2)(x, y) + rmat2(flow, x, y)*t;
	}		
}

/*
Mat blendImage(imageL.size(), CV_8UC4);
for (int y = 0; y < imageL.rows; ++y) {
for (int x = 0; x < imageL.cols; ++x) {
	const Vec4b colorL = imageL.at<Vec4b>(y, x);
	const Vec4b colorR = imageR.at<Vec4b>(y, x);
	Vec4b colorMixed;
	if (colorL[3] == 0 && colorR[3] == 0 ) {
		colorMixed = Vec4b(0, 0, 0, 0);
	} else if (colorL[3] > 0 && colorR[3] == 0) {
		colorMixed = Vec4b(colorL[0], colorL[1], colorL[2], 255);
	} else if (colorL[3] == 0 && colorR[3] > 0) {
		colorMixed = Vec4b(colorR[0], colorR[1], colorR[2], 255);
	} else {
		const Point2f fLR = flowLtoR.at<Point2f>(y, x);
		const Point2f fRL = flowRtoL.at<Point2f>(y, x);
		const float flowMagLR = sqrtf(fLR.x * fLR.x + fLR.y * fLR.y) / float(imageL.cols);
		const float flowMagRL = sqrtf(fRL.x * fRL.x + fRL.y * fRL.y) / float(imageL.cols);
		const float colorDiff =
			(std::abs(colorL[0] - colorR[0]) +
			 std::abs(colorL[1] - colorR[1]) +
			 std::abs(colorL[2] - colorR[2])) / 255.0f;
		static const float kColorDiffCoef = 10.0f;
		static const float kSoftmaxSharpness = 10.0f;
		static const float kFlowMagCoef = 100.0f; // determines how much we prefer larger flows
		const float deghostCoef = tanhf(colorDiff * kColorDiffCoef);
		const float alphaL = colorL[3] / 255.0f;
		const float alphaR = colorR[3] / 255.0f;
		const double expL =
			exp(kSoftmaxSharpness * blendL * alphaL * (1.0 + kFlowMagCoef * flowMagRL));
		const double expR =
			exp(kSoftmaxSharpness * blendR * alphaR * (1.0 + kFlowMagCoef * flowMagLR));
		const double sumExp = expL + expR + 0.00001;
		const float softmaxL = float(expL / sumExp);
		const float softmaxR = float(expR / sumExp);
		colorMixed = Vec4b(
			float(colorL[0]) * lerp(blendL, softmaxL, deghostCoef) + float(colorR[0]) * lerp(blendR, softmaxR, deghostCoef),
			float(colorL[1]) * lerp(blendL, softmaxL, deghostCoef) + float(colorR[1]) * lerp(blendR, softmaxR, deghostCoef),
			float(colorL[2]) * lerp(blendL, softmaxL, deghostCoef) + float(colorR[2]) * lerp(blendR, softmaxR, deghostCoef),
			255);
	}
	blendImage.at<Vec4b>(y, x) = colorMixed;
}
}
*/
__kernel void combine_novel_views(
	__global const uchar4* imageL, int imageL_step, int imageL_offset,
	__global const uchar4* imageR, int imageR_step, int imageR_offset,
	__global float2* flowLtoR, int flowLtoR_step, int flowLtoR_offset, 
	__global float2* flowRtoL, int flowRtoL_step, int flowRtoL_offset,
	__global uchar4* blendImage, int blendImage_step, int blendImage_offset, int blendImage_rows, int blendImage_cols,
	float blendL, float blendR)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < blendImage_cols && y < blendImage_rows) {
		uchar4 colorL = rmat8uc4(imageL, x, y);
		uchar4 colorR = rmat8uc4(imageR, x, y);
		uchar4 colorMixed;
		if (colorL.s3 == 0 && colorR.s3 == 0 ) {
			colorMixed = (uchar4)(0, 0, 0, 0);
		} else if (colorL.s3 > 0 && colorR.s3 == 0) {
			colorMixed =  (uchar4)(colorL.s0, colorL.s1, colorL.s2, 255);
		} else if (colorL.s3 == 0 && colorR.s3 > 0) {
			colorMixed =  (uchar4)(colorR.s0, colorR.s1, colorR.s2, 255);
		} else {
			float2 fLR = rmat2(flowLtoR, x, y);
			float2 fRL = rmat2(flowRtoL, x, y);
			float flowMagLR = sqrt(fLR.x * fLR.x + fLR.y * fLR.y) / blendImage_cols;
			float flowMagRL = sqrt(fRL.x * fRL.x + fRL.y * fRL.y) / blendImage_cols;
			float colorDiff =
				(fabs((float)colorL.s0 - (float)colorR.s0) +
				 fabs((float)colorL.s1 - (float)colorR.s1) +
				 fabs((float)colorL.s2 - (float)colorR.s2)) / 255.0f;
			const float kColorDiffCoef = 10.0f;
			const float kSoftmaxSharpness = 10.0f;
			const float kFlowMagCoef = 100.0f; // determines how much we prefer larger flows
			float deghostCoef = tanh(colorDiff * kColorDiffCoef);
			float alphaL = colorL.s3 / 255.0f;
			float alphaR = colorR.s3 / 255.0f;
			double expL = exp(kSoftmaxSharpness * blendL * alphaL * (1.0 + kFlowMagCoef * flowMagRL));
			double expR = exp(kSoftmaxSharpness * blendR * alphaR * (1.0 + kFlowMagCoef * flowMagLR));
			double sumExp = expL + expR + 0.00001;
			float softmaxL = expL / sumExp;
			float softmaxR = expR / sumExp;
			colorMixed = (uchar4)(
				colorL.s0 * lerp(blendL, softmaxL, deghostCoef) + colorR.s0 * lerp(blendR, softmaxR, deghostCoef),
				colorL.s1 * lerp(blendL, softmaxL, deghostCoef) + colorR.s1 * lerp(blendR, softmaxR, deghostCoef),
				colorL.s2 * lerp(blendL, softmaxL, deghostCoef) + colorR.s2 * lerp(blendR, softmaxR, deghostCoef),
				255);	
		}
		wmat8uc4(blendImage, x, y) = colorMixed;
	}
}


/*
for (int y = 0; y < imageL.rows; ++y) {
for (int x = 0; x < imageL.cols; ++x) {
	const Vec4b colorL = imageL.at<Vec4b>(y, x);
	const Vec4b colorR = imageR.at<Vec4b>(y, x);

	const unsigned char outAlpha = max(colorL[3], colorR[3]) / 255.0f > 0.1 ? 255 : 0;
	Vec4b colorMixed;
	if (colorL[3] == 0 && colorR[3] == 0) {
		colorMixed = Vec4b(0, 0, 0, outAlpha);
	} else if (colorL[3] == 0) {
		colorMixed = Vec4b(colorR[0], colorR[1], colorR[2], outAlpha);
	} else if (colorR[3] == 0) {
		colorMixed = Vec4b(colorL[0], colorL[1], colorL[2], outAlpha);
	} else {
		const float magL = flowMagL.at<float>(y,x) / float(imageL.cols);
		const float magR = flowMagR.at<float>(y,x) / float(imageL.cols);
		float blendL = float(colorL[3]);
		float blendR = float(colorR[3]);
		float norm = blendL + blendR;
		blendL /= norm;
		blendR /= norm;
		const float colorDiff =
			(std::abs(colorL[0] - colorR[0]) +
			 std::abs(colorL[1] - colorR[1]) +
			 std::abs(colorL[2] - colorR[2])) / 255.0f;
		static const float kColorDiffCoef = 10.0f;
		static const float kSoftmaxSharpness = 10.0f;
		static const float kFlowMagCoef = 20.0f; // NOTE: this is scaled differently than the test version due to normalizing magL & magR by imageL.cols
		const float deghostCoef = tanhf(colorDiff * kColorDiffCoef);
		const double expL = exp(kSoftmaxSharpness * blendL * (1.0 + kFlowMagCoef * magL));
		const double expR = exp(kSoftmaxSharpness * blendR * (1.0 + kFlowMagCoef * magR));
		const double sumExp = expL + expR + 0.00001;
		const float softmaxL = float(expL / sumExp);
		const float softmaxR = float(expR / sumExp);
		colorMixed = Vec4b(
			float(colorL[0]) * lerp(blendL, softmaxL, deghostCoef) + float(colorR[0]) * lerp(blendR, softmaxR, deghostCoef),
			float(colorL[1]) * lerp(blendL, softmaxL, deghostCoef) + float(colorR[1]) * lerp(blendR, softmaxR, deghostCoef),
			float(colorL[2]) * lerp(blendL, softmaxL, deghostCoef) + float(colorR[2]) * lerp(blendR, softmaxR, deghostCoef),
			255);
	}
	blendImage.at<Vec4b>(y, x) = colorMixed;
}
}
*/
__kernel void combine_lazy_views(
	__global const uchar4* imageL, int imageL_step, int imageL_offset,
	__global const uchar4* imageR, int imageR_step, int imageR_offset,
	__global const float* flowMagL, int flowMagL_step, int flowMagL_offset,
	__global const float* flowMagR, int flowMagR_step, int flowMagR_offset,	
	__global uchar4* blendImage, int blendImage_step, int blendImage_offset, int blendImage_rows, int blendImage_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < blendImage_cols && y < blendImage_rows) {
		uchar4 colorL = rmat8uc4(imageL, x, y);
		uchar4 colorR = rmat8uc4(imageR, x, y);
		uchar outAlpha = max(colorL.s3, colorR.s3)/255.0f > 0.1 ? 255 : 0;
		uchar4 colorMixed;
		if (colorL.s3 == 0 && colorR.s3 == 0) {
			colorMixed = (uchar4)(0, 0, 0, outAlpha);
		} else if (colorL.s3 == 0) {
			colorMixed = (uchar4)(colorR.s0, colorR.s1, colorR.s2, outAlpha);
		} else if (colorR.s3 == 0) {
			colorMixed = (uchar4)(colorL.s0, colorL.s1, colorL.s2, outAlpha);
		} else {
			float magL = rmat(flowMagL, x, y) / blendImage_cols;
			float magR = rmat(flowMagR, x, y) / blendImage_cols;
			float blendL = colorL.s3;
			float blendR = colorR.s3;
			float norm = blendL + blendR;
			blendL /= norm;	// blendL = colorL.s3/(colorL.s3 + colorR.s3); is better ?
			blendR /= norm;	// blendR = 1.0f - blendL; is better ?
			float colorDiff =
				(fabs((float)colorL.s0 - (float)colorR.s0) +
				 fabs((float)colorL.s1 - (float)colorR.s1) +
				 fabs((float)colorL.s2 - (float)colorR.s2)) / 255.0f;
			const float kColorDiffCoef = 10.0f;
			const float kSoftmaxSharpness = 10.0f;
			const float kFlowMagCoef = 20.0f; // NOTE: this is scaled differently than the test version due to normalizing magL & magR by imageL.cols
			float deghostCoef = tanh(colorDiff * kColorDiffCoef);
			double expL = exp(kSoftmaxSharpness * blendL * (1.0 + kFlowMagCoef * magL));
			double expR = exp(kSoftmaxSharpness * blendR * (1.0 + kFlowMagCoef * magR));
			double sumExp = expL + expR + 0.00001;
			float softmaxL = expL / sumExp;  // float softmaxL = expL / (expL + expR + 0.00001); is better ?
			float softmaxR = expR / sumExp;	 // float softmaxR = expR / (expL + expR + 0.00001); is better ?
			colorMixed = (uchar4)(
				colorL.s0 * lerp(blendL, softmaxL, deghostCoef) + colorR.s0 * lerp(blendR, softmaxR, deghostCoef),
				colorL.s1 * lerp(blendL, softmaxL, deghostCoef) + colorR.s1 * lerp(blendR, softmaxR, deghostCoef),
				colorL.s2 * lerp(blendL, softmaxL, deghostCoef) + colorR.s2 * lerp(blendR, softmaxR, deghostCoef),
				255);
		}
		wmat8uc4(blendImage, x, y) = colorMixed;
	}
}

/*
Mat warpOpticalFlow = Mat(Size(width, height), CV_32FC2);
for (int y = 0; y < height; ++y) {
for (int x = 0; x < width; ++x) {
	const Point3f lazyWarp = novelViewWarpBuffer[x][y];
	warpOpticalFlow.at<Point2f>(y, x) = Point2f(lazyWarp.x, lazyWarp.y);
}
}
*/
__kernel void get_warp_optical_flow(
	__global const float3* warpBuffer, int warpBuffer_step, int warpBuffer_offset, 
	__global float2* warpFlow, int warpFlow_step, int warpFlow_offset, int warpFlow_rows, int warpFlow_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < warpFlow_cols && y < warpFlow_rows) {
		float3 lazyWarp = rmat3(warpBuffer, x, y);
		wmat2(warpFlow, x, y) = (float2)(lazyWarp.x, lazyWarp.y);
	}
}		
	
/*
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
*/
__kernel void get_warp_composition(
	__global const float3* warpBuffer, int warpBuffer_step, int warpBuffer_offset,
	__global const float2* warpFlow, int warpFlow_step, int warpFlow_offset, 	
	__global float2* warpComposition, int warpComposition_step, int warpComposition_offset, int warpComposition_rows, int warpComposition_cols,
	int invertT)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < warpComposition_cols && y < warpComposition_rows) {
		float3 lazyWarp = rmat3(warpBuffer, x, y);
		float2 flowDir = rmat2(warpFlow, x, y);
		float t = invertT ? (1.0f - lazyWarp.z) : lazyWarp.z;
		wmat2(warpComposition, x, y) = (float2) (lazyWarp.x + flowDir.x * t, lazyWarp.y + flowDir.y * t);	
	}
}

/*
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
__kernel void get_novel_view_flow_mag(
	__global const float3* warpBuffer, int warpBuffer_step, int warpBuffer_offset,
	__global const float2* warpFlow, int warpFlow_step, int warpFlow_offset,
	__global uchar4* novelView, int novelView_step, int novelView_offset, 	
	__global float* flowMag, int flowMag_step, int flowMag_offset, int flowMag_rows, int flowMag_cols,
	int invertT)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < flowMag_cols && y < flowMag_rows) {
		float3 lazyWarp = rmat3(warpBuffer, x, y);
		float t = invertT ? (1.0f - lazyWarp.z) : lazyWarp.z;
		wmat8uc4(novelView, x, y).s3 = (1.0f - t) * rmat8uc4(novelView, x, y).s3;
		float2 flowDir = rmat2(warpFlow, x, y);										// 
		wmat(flowMag, x, y) = sqrt(flowDir.x * flowDir.x + flowDir.y * flowDir.y);	// wmat(flowMag, x, y) = length(rmat2(warpFlow, x, y)) is better ?
	}
}

