/**
 * @brief the following macros are used for accessing img(x, y)
 */
#define rmat8uc4(addr, x, y) ((__global const uchar4*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc1(addr, x, y) ((__global const float*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc2(addr, x, y) ((__global const float2*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc1(addr, x, y) ((__global float*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc2(addr, x, y) ((__global float2*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat(addr, x, y) 	rmat32fc1(addr, x, y)
#define rmat2(addr, x, y) 	rmat32fc2(addr, x, y)
#define wmat(addr, x, y) 	wmat32fc1(addr, x, y)
#define wmat2(addr, x, y) 	wmat32fc2(addr, x, y)


/**
 * @brief motion detection
 */
__kernel void motion_detection(
	__global const uchar4* cur, int cur_step, int cur_offset, int cur_rows, int cur_cols,
	__global const uchar4* pre, int pre_step, int pre_offset,
	__global float* motion, int motion_step, int motion_offset) 
{
    int x = get_global_id(0);
    int y = get_global_id(1);
	if (x < cur_cols && y < cur_rows) {
		wmat(motion, x, y) = 
			( fabs((float)rmat8uc4(cur, x, y).x - (float)rmat8uc4(pre, x, y).x)
			+ fabs((float)rmat8uc4(cur, x, y).y - (float)rmat8uc4(pre, x, y).y)
			+ fabs((float)rmat8uc4(cur, x, y).z - (float)rmat8uc4(pre, x, y).z))/(3.0f*255.0f);
	}
}

__kernel void motion_detection_v2(
	__global const uchar4* cur, int cur_step, int cur_offset, int cur_rows, int cur_cols,
	__global const uchar4* pre, int pre_step, int pre_offset,
	__global float* motion, int motion_step, int motion_offset) 
{
    int x = get_global_id(0);
    int y = get_global_id(1);
	if (x < cur_cols && y < cur_rows) {
		float m = 
			( fabs((float)rmat8uc4(cur, x, y).x - (float)rmat8uc4(pre, x, y).x)
			+ fabs((float)rmat8uc4(cur, x, y).y - (float)rmat8uc4(pre, x, y).y)
			+ fabs((float)rmat8uc4(cur, x, y).z - (float)rmat8uc4(pre, x, y).z))/(3.0f*255.0f);
		
		const float top_bottom_thresh = 0.3f;  
		float delta;
		if (y < cur_rows * top_bottom_thresh) {
			delta = y / (cur_rows * top_bottom_thresh); 
		} else if (y > cur_rows* (1.0f - top_bottom_thresh)) {
			delta = (cur_rows - y) / (cur_rows * top_bottom_thresh);
		} else {
			delta = 1.01f;
		}
		if (delta < 1.01f) {
			if (delta < 0.1f) {
				delta = 0.1f;
			}
			m *= delta;
		}
		wmat(motion, x, y) = m;
	}
}

/**
 * @brief adjust flow toward previous
 */
__kernel void adjust_flow_toward_previous(
	__global float2* cur, int cur_step, int cur_offset, int cur_rows, int cur_cols,
	__global const float2* pre, int pre_step, int pre_offset,
	__global const float* motion, int motion_step, int motion_offset) 
{
    int x = get_global_id(0);
    int y = get_global_id(1);
	if (x < cur_cols && y < cur_rows) {
		float w = 1.0f - rmat(motion, x, y);
		wmat2(cur, x, y) = (1.0f - w) * rmat2(cur, x, y) + w * rmat2(pre, x, y);
	}
}
__kernel void adjust_flow_toward_previous_v2(
	__global float2* cur, int cur_step, int cur_offset, int cur_rows, int cur_cols,
	__global const float2* pre, int pre_step, int pre_offset,
	__global const float* motion, int motion_step, int motion_offset,
	float motion_threshhold) 
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	if (x < cur_cols && y < cur_rows) {
		const float top_bottom_thresh = 0.2f;
		float flowMotion = rmat(motion, x, y);
		if (y < cur_rows * top_bottom_thresh || y > cur_rows* (1 - top_bottom_thresh)) {
			//pass we assume there is little move in such area.. 
		} else {
			if (flowMotion > motion_threshhold) {
				flowMotion = 1.0;
			}
		}
		float w = 1.0f - flowMotion;
		wmat2(cur, x, y) = (1.0f - w) * rmat2(cur, x, y) + w * rmat2(pre, x, y);
		/* @optimized
		const float top_bottom_thresh = 0.2f;
		float flowMotion = rmat(motion, x, y);
		if (y < cur_rows * top_bottom_thresh || y > cur_rows* (1 - top_bottom_thresh) || flowMotion <= motion_threshhold) {
			wmat2(cur, x, y)  = flowMotion * rmat2(cur, x, y)  + (1 - flowMotion) * rmat2(pre, x, y);
		}
		*/
	}
}

/**
 * @brief the follwoing constants must be same as the ones defined in host program
 */
__constant int   kPyrMinImageSize 				= 24;
// __constant int   kPyrMaxLevels 				= 1000;
__constant float kGradEpsilon 					= 0.001f; // for finite differences
__constant float kUpdateAlphaThreshold 			= 0.9f;   // pixels with alpha below this aren't updated by proposals
// __constant int   kMedianBlurSize 			= 5;      // medianBlur max size is 5 pixels for CV_32FC2
// __constant int   kPreBlurKernelWidth 		= 5;
// __constant float kPreBlurSigma 				= 0.25f;  // amount to blur images before pyramids
// __constant int   kFinalFlowBlurKernelWidth 	= 3;
// __constant float kFinalFlowBlurSigma 		= 1.0f;   // blur that is applied to flow at the end after upscaling
// __constant int   kGradientBlurKernelWidth 	= 3;
// __constant float kGradientBlurSigma 			= 0.5f;   // amount to blur image gradients
// __constant int   kBlurredFlowKernelWidth 	= 15;     // for regularization/smoothing/diffusion
// __constant float kBlurredFlowSigma 			= 8.0f;
//
// the following values is specified in original implementation
//
// __constant float kPyrScaleFactor 			= 0.9f;
__constant float kSmoothnessCoef 				= 0.001f;
__constant float kVerticalRegularizationCoef 	= 0.01f;
__constant float kHorizontalRegularizationCoef 	= 0.01f;
__constant float kGradientStepSize 				= 0.5f;
// __constant float kDownscaleFactor 			= 0.5f;
__constant float kDirectionalRegularizationCoef = 0.0f;
__constant int   kUseDirectionalRegularization 	= 0;
__constant int   kMaxPercentage 				= 0;	// NOTES: this value can't be zero, and should be same as the one in host program


float compute_patch_error(
	__global const float* I0, int I0_step, int I0_offset, int I0_rows, int I0_cols, int i0x, int i0y, 
	__global const float* I1, int I1_step, int I1_offset, int I1_rows, int I1_cols, int i1x, int i1y,
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset)
{
	// compute sum-of-absolute-differences in 5x5 patch
	const int kPatchRadius = 2;
	float sad = 0;
	float alpha = 0;
	for (int dy = -kPatchRadius; dy <= kPatchRadius; ++dy) {
		for (int dx = -kPatchRadius; dx <= kPatchRadius; ++dx) {
			int d0y = i0y + dy;
			int d0x = i0x + dx;
			
			if (0 <= d0y && d0y < I0_rows && 0 <= d0x && d0x < I0_cols) {
				int d1y = min(max(0, i1y + dy), I1_rows - 1);
				int d1x = min(max(0, i1x + dx), I1_cols - 1);
				
				sad += fabs(rmat(I0, d0x, d0y) - rmat(I1, d1x, d1y));
				alpha += rmat(alpha0, d0x, d0y) * rmat(alpha1, d1x, d1y);
			}
		}
	}
	// normalize sad to sum of alphas (fine to go to infinity)
	sad /= alpha;
	// scale sad as flow vector length increases to favor short vectorss
	float length = distance((float2)(i1x, i1y), (float2)(i0x, i0y));
	// we search a fraction of the coarsest pyramid level
	float search_distance = (kPyrMinImageSize * kMaxPercentage + 50)/100;
	sad *= 1 + length/search_distance;
	return sad;
}

/**
 * @brief estimate flow by searching the closet rect area
 */
__kernel void estimate_flow(
	__global const float* I0, int I0_step, int I0_offset, int I0_rows, int I0_cols,
	__global const float* I1, int I1_step, int I1_offset, int I1_rows, int I1_cols,
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global float2* flow, int flow_step, int flow_offset,
	int box_x, int box_y, int box_width, int box_height)
{
	int i0x = get_global_id(0);
	int i0y = get_global_id(1);
	if (i0x < I0_cols && i0y < I0_rows && rmat(alpha0, i0x, i0y) > kUpdateAlphaThreshold) {
		const float kFraction = 0.8f; // lower the fraction to increase affinity
		float errorBest = kFraction * compute_patch_error(
			I0, I0_step, I0_offset, I0_rows, I0_cols, i0x, i0y,
			I1, I1_step, I1_offset, I1_rows, I1_cols, i0x, i0y, 
			alpha0, alpha0_step, alpha0_offset,
			alpha1, alpha1_step, alpha1_offset);
		
		// look for better patch in the box
		int i1xBest = i0x;
		int i1yBest = i0y;
		for (int dy = box_y; dy < box_y + box_height; ++dy) {
			for (int dx = box_x; dx < box_x + box_width; ++dx) {
				int i1x = i0x + dx;
				int i1y = i0y + dy;
				
				if (0 <= i1x && i1x < I1_cols && 0 <= i1y && i1y < I1_rows) {
					float error = compute_patch_error(
						I0, I0_step, I0_offset, I0_rows, I0_cols, i0x, i0y,
						I1, I1_step, I1_offset, I1_rows, I1_cols, i1x, i1y, 
						alpha0, alpha0_step, alpha0_offset,
						alpha1, alpha1_step, alpha1_offset);
					if (errorBest > error) {
						errorBest = error;
						i1xBest = i1x;
						i1yBest = i1y;
					}
				}
			}
		}	
		// use the best match
		wmat2(flow, i0x, i0y) = (float2)(i1xBest - i0x, i1yBest - i0y);
	}
}

/**
 * @brief low alpha flow diffusion
 */
__kernel void alpha_flow_diffusion(
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset, int flow_rows, int flow_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < flow_cols && y < flow_rows) {
		float diffusionCoef = 1 - rmat(alpha0, x, y) * rmat(alpha1, x, y);
		wmat2(flow, x, y) = diffusionCoef * rmat2(blurred, x, y) +  (1-diffusionCoef) * rmat2(flow, x, y);
	}
}


float get_pix_bilinear32f_extend(
	__global const float* img, int img_step, int img_offset, 
	int img_rows, int img_cols,
	float x, float y)
{
	x = min(img_cols - 2.0f, max(0.0f, x));
	y = min(img_rows - 2.0f, max(0.0f, y));
	int x0 = convert_int_rtz(x);
	int y0 = convert_int_rtz(y);
	float xR = x - x0;
	float yR = y - y0;
	float f00 = rmat(img, x0, y0);
	float f01 = rmat(img, x0, y0 + 1);
	float f10 = rmat(img, x0 + 1, y0);
	float f11 = rmat(img, x0 + 1, y0 + 1);
	return f00 + (f10 - f00)*xR + (f01 - f00)*yR + (f00 + f11 - f10 - f01)*xR*yR;
}

float error_function(
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	int flow_rows, int flow_cols, 
	int x, int y, float2 flow)
{	
	float matchX      = x + flow.x;
	float matchY      = y + flow.y;
	float i0x         = rmat(I0x, x, y);
	float i0y         = rmat(I0y, x, y);
	float i1x         = get_pix_bilinear32f_extend(I1x, I1x_step, I1x_offset, flow_rows, flow_cols, matchX, matchY); // NOTES: flow, blurred, I0, I1, I0x, I0y, I1x, I1y has the same width/height
	float i1y         = get_pix_bilinear32f_extend(I1y, I1y_step, I1y_offset, flow_rows, flow_cols, matchX, matchY);
	float smoothness  =	distance(rmat2(blurred, x, y), flow);
	
	float err = distance((float2)(i0x, i0y), (float2)(i1x, i1y))
		+ smoothness * kSmoothnessCoef
		+ kVerticalRegularizationCoef * fabs(flow.y) / flow_cols;	// NOTES: ... /flow_rows is better?
		+ kHorizontalRegularizationCoef * fabs(flow.x) / flow_cols;	
		
	if (kUseDirectionalRegularization) {
		float2 blurredFlow = rmat2(blurred, x, y);
		const float kEpsilon = 0.001f;
		// NOTES: ... dot(normalize(blurredFlow), normalize(flow)) is better ?
		err -= kDirectionalRegularizationCoef * dot(blurredFlow/(length(blurredFlow) + kEpsilon), flow/(length(flow) + kEpsilon));
	}
	return err;
}

void propose_flow_update(
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset, int flow_rows, int flow_cols,
	int updateX, int updateY, float2 proposedFlow, float* currErr) 
{
	float proposalErr = error_function(
		I0x, I0x_step, I0x_offset,
		I0y, I0y_step, I0y_offset,
		I1x, I1x_step, I1x_offset,
		I1y, I1y_step, I1y_offset,
		blurred, blurred_step, blurred_offset,
		flow_rows, flow_cols,
		updateX, updateY, proposedFlow);
	if (proposalErr < (*currErr)) {
		wmat2(flow, updateX, updateY) = proposedFlow;
		*currErr = proposalErr;
	}
}

float2 error_gradient(
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset,
	int flow_rows, int flow_cols,
	int x, int y, float currErr) 
{
	float2 dx = (float2)(kGradEpsilon, 0.0f);
	float2 dy = (float2)(0.0f, kGradEpsilon);
	
	float fx = error_function(
		I0x, I0x_step, I0x_offset,
		I0y, I0y_step, I0y_offset,
		I1x, I1x_step, I1x_offset,
		I1y, I1y_step, I1y_offset,
		blurred, blurred_step, blurred_offset,
		flow_rows, flow_cols,
		x, y, rmat2(flow, x, y)+dx);		
	float fy = error_function(
		I0x, I0x_step, I0x_offset,
		I0y, I0y_step, I0y_offset,
		I1x, I1x_step, I1x_offset,
		I1y, I1y_step, I1y_offset,
		blurred, blurred_step, blurred_offset,
		flow_rows, flow_cols,
		x, y, rmat2(flow, x, y)+dy);
	
	//return (float2)((fx - currErr)/kGradEpsilon, (fy - currErr)/kGradEpsilon);
	return (float2)((fx - currErr)*1000.0f, (fy - currErr)*1000.0f);
}

/**
 * @brief sweep from top left
 */
__kernel void sweep_from_top_left(
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset, int flow_rows, int flow_cols,
	int start_x, int start_y)
{
	int k = get_global_id(0);
	int x = start_x + k;
	int y = start_y - k;
	
	if (0 <= x && x < flow_cols && 0 <= y && y < flow_rows 
		&& rmat(alpha0, x, y) > kUpdateAlphaThreshold 
		&& rmat(alpha1, x, y) > kUpdateAlphaThreshold) {
		float currErr = error_function(
			I0x, I0x_step, I0x_offset,
			I0y, I0y_step, I0y_offset,
			I1x, I1x_step, I1x_offset,
			I1y, I1y_step, I1y_offset,
			blurred, blurred_step, blurred_offset,
			flow_rows, flow_cols,
			x, y, rmat2(flow, x, y));
			
		if (x > 0) {
			propose_flow_update(
				I0x, I0x_step, I0x_offset,
				I0y, I0y_step, I0y_offset,
				I1x, I1x_step, I1x_offset,
				I1y, I1y_step, I1y_offset,
				blurred, blurred_step, blurred_offset,
				flow, flow_step, flow_offset, flow_rows, flow_cols,
				x, y, rmat2(flow, x-1, y), &currErr);
		}
		if (y > 0) {
			propose_flow_update(
				I0x, I0x_step, I0x_offset,
				I0y, I0y_step, I0y_offset,
				I1x, I1x_step, I1x_offset,
				I1y, I1y_step, I1y_offset,
				blurred, blurred_step, blurred_offset,
				flow, flow_step, flow_offset, flow_rows, flow_cols,
				x, y, rmat2(flow, x, y-1), &currErr);
		}
		
		wmat2(flow, x, y) -= kGradientStepSize * error_gradient(
			I0x, I0x_step, I0x_offset,
			I0y, I0y_step, I0y_offset,
			I1x, I1x_step, I1x_offset,
			I1y, I1y_step, I1y_offset,
			blurred, blurred_step, blurred_offset,
			flow, flow_step, flow_offset, flow_rows, flow_cols,
			x, y, currErr);
	}
}

/**
 * @brief sweep from bottom right
 */
__kernel void sweep_from_bottom_right(
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset, int flow_rows, int flow_cols,
	int start_x, int start_y) 
{
	int k = get_global_id(0);
	int x = start_x + k;
	int y = start_y - k;
	
	if (0 <= x && x < flow_cols && 0 <= y && y < flow_rows
		&& rmat(alpha0, x, y) > kUpdateAlphaThreshold 
		&& rmat(alpha1, x, y) > kUpdateAlphaThreshold) {
		float currErr = error_function(
			I0x, I0x_step, I0x_offset,
			I0y, I0y_step, I0y_offset,
			I1x, I1x_step, I1x_offset,
			I1y, I1y_step, I1y_offset,
			blurred, blurred_step, blurred_offset,
			flow_rows, flow_cols,
			x, y, rmat2(flow, x, y));
			
		if (x < flow_cols - 1) {
			propose_flow_update(
				I0x, I0x_step, I0x_offset,
				I0y, I0y_step, I0y_offset,
				I1x, I1x_step, I1x_offset,
				I1y, I1y_step, I1y_offset,
				blurred, blurred_step, blurred_offset,
				flow, flow_step, flow_offset, flow_rows, flow_cols,
				x, y, rmat2(flow, x+1, y), &currErr);
		}
		if (y < flow_rows - 1) {
			propose_flow_update(
				I0x, I0x_step, I0x_offset,
				I0y, I0y_step, I0y_offset,
				I1x, I1x_step, I1x_offset,
				I1y, I1y_step, I1y_offset,
				blurred, blurred_step, blurred_offset,
				flow, flow_step, flow_offset, flow_rows, flow_cols,
				x, y, rmat2(flow, x, y+1), &currErr);
		}
		
		wmat2(flow, x, y) -= kGradientStepSize * error_gradient(
			I0x, I0x_step, I0x_offset,
			I0y, I0y_step, I0y_offset,
			I1x, I1x_step, I1x_offset,
			I1y, I1y_step, I1y_offset,
			blurred, blurred_step, blurred_offset,
			flow, flow_step, flow_offset, flow_rows, flow_cols,
			x, y, currErr);
	}
}


__kernel void sweep_from_left(
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset, int flow_rows, int flow_cols)
{
	int y = get_global_id(0);
	if (y < flow_rows) {
		for (int x=0; x < flow_cols; ++x) {
			if (rmat(alpha0, x, y) > kUpdateAlphaThreshold && rmat(alpha1, x, y) > kUpdateAlphaThreshold) {
				float currErr = error_function(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow_rows, flow_cols,
					x, y, rmat2(flow, x, y));
					
				if (x > 0) {
					propose_flow_update(
						I0x, I0x_step, I0x_offset,
						I0y, I0y_step, I0y_offset,
						I1x, I1x_step, I1x_offset,
						I1y, I1y_step, I1y_offset,
						blurred, blurred_step, blurred_offset,
						flow, flow_step, flow_offset, flow_rows, flow_cols,
						x, y, rmat2(flow, x-1, y), &currErr);
				}
				
				wmat2(flow, x, y) -= kGradientStepSize * error_gradient(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow, flow_step, flow_offset, flow_rows, flow_cols,
					x, y, currErr);
			}
		}
	}
}


__kernel void sweep_from_right(
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset, int flow_rows, int flow_cols)
{
	int y = get_global_id(0);
	if (y < flow_rows) {
		for (int x=flow_cols-1; x >=0; --x) {
			if (rmat(alpha0, x, y) > kUpdateAlphaThreshold && rmat(alpha1, x, y) > kUpdateAlphaThreshold) {
				float currErr = error_function(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow_rows, flow_cols,
					x, y, rmat2(flow, x, y));
					
				if (x < flow_cols-1) {
					propose_flow_update(
						I0x, I0x_step, I0x_offset,
						I0y, I0y_step, I0y_offset,
						I1x, I1x_step, I1x_offset,
						I1y, I1y_step, I1y_offset,
						blurred, blurred_step, blurred_offset,
						flow, flow_step, flow_offset, flow_rows, flow_cols,
						x, y, rmat2(flow, x+1, y), &currErr);
				}
				
				wmat2(flow, x, y) -= kGradientStepSize * error_gradient(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow, flow_step, flow_offset, flow_rows, flow_cols,
					x, y, currErr);
			}
		}
	}
}

__kernel void sweep_from_top(
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset, int flow_rows, int flow_cols)
{
	int x = get_global_id(0);
	if (x < flow_cols) {
		for (int y=0; y < flow_rows; ++y) {
			if (rmat(alpha0, x, y) > kUpdateAlphaThreshold && rmat(alpha1, x, y) > kUpdateAlphaThreshold) {
				float currErr = error_function(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow_rows, flow_cols,
					x, y, rmat2(flow, x, y));
					
				if (y > 0) {
					propose_flow_update(
						I0x, I0x_step, I0x_offset,
						I0y, I0y_step, I0y_offset,
						I1x, I1x_step, I1x_offset,
						I1y, I1y_step, I1y_offset,
						blurred, blurred_step, blurred_offset,
						flow, flow_step, flow_offset, flow_rows, flow_cols,
						x, y, rmat2(flow, x, y-1), &currErr);
				}
				
				wmat2(flow, x, y) -= kGradientStepSize * error_gradient(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow, flow_step, flow_offset, flow_rows, flow_cols,
					x, y, currErr);
			}
		}
	}
}

__kernel void sweep_from_bottom(
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset, int flow_rows, int flow_cols)
{
	int x = get_global_id(0);
	if (x < flow_cols) {		
		for (int y=flow_rows-1; y >=0; --y) {
			if (rmat(alpha0, x, y) > kUpdateAlphaThreshold && rmat(alpha1, x, y) > kUpdateAlphaThreshold) {
				float currErr = error_function(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow_rows, flow_cols,
					x, y, rmat2(flow, x, y));
					
				if (y < flow_rows-1) {
					propose_flow_update(
						I0x, I0x_step, I0x_offset,
						I0y, I0y_step, I0y_offset,
						I1x, I1x_step, I1x_offset,
						I1y, I1y_step, I1y_offset,
						blurred, blurred_step, blurred_offset,
						flow, flow_step, flow_offset, flow_rows, flow_cols,
						x, y, rmat2(flow, x, y+1), &currErr);
				}
				
				wmat2(flow, x, y) -= kGradientStepSize * error_gradient(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow, flow_step, flow_offset, flow_rows, flow_cols,
					x, y, currErr);
			}
		}
	}
}


__kernel void sweep_to(
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global const float* I0x, int I0x_step, int I0x_offset,
	__global const float* I0y, int I0y_step, int I0y_offset,
	__global const float* I1x, int I1x_step, int I1x_offset,
	__global const float* I1y, int I1y_step, int I1y_offset,
	__global const float2* blurred, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset, int flow_rows, int flow_cols,
	int dx, int dy)
{
	int k = get_global_id(0);
	if (k < flow_rows + flow_cols - 1) {
		/*
		int start_x;
		int start_y;
		if (dx == 1 && dy == 1) {
			// x: 0, 1, 2, ..., cols-1 	0, 	..., 0
			// y: 0, 0, 0, ..., 0,		1, 	..., rows-1
			start_x = k < flow_cols ? k : 0;
			start_y = k < flow_cols ? 0 : k - flow_cols + 1;
		} else if (dx == 1 && dy == -1) {
			// x: 0, 1, 2, ..., cols-1 	0, 	..., 0
			// y: rows-1,, ..., rows-1,	0, 	..., rows-2
			start_x = k < flow_cols ? k : 0;
			start_y = k < flow_cols ? flow_rows - 1 : k - flow_cols;
		} else if (dx == -1 && dy == -1) {
			// x: 0, 1, 2, ..., cols-1 	cols-1, ..., cols-1
			// y: rows-1,, ..., rows-1,	0,      ..., rows-2
			start_x = k < flow_cols ? k : flow_cols - 1;
			start_y = k < flow_cols ? flow_rows - 1 : k - flow_cols;
		} else {
			// x: 0, 1, 2, ..., cols-1 	cols-1, ..., cols-1
			// y: 0, 0, 0, ..., 0,		1,      ..., rows-1
			start_x = k < flow_cols ? k : flow_cols - 1;
			start_y = k < flow_cols ? 0 : k - flow_cols + 1;
		}
		*/
		int start_x = k < flow_cols ? k : dx == 1 ? 0 : flow_cols - 1;
		int start_y = k < flow_cols ? (dy == 1 ? 0 : flow_rows -1) : (dy == 1 ? k - flow_cols + 1 : k - flow_cols);
		
		for (int x=start_x, y=start_y; 0 <= x && x < flow_cols && 0 <= y && y < flow_rows; x += dx, y += dy) {
			
			if (rmat(alpha0, x, y) > kUpdateAlphaThreshold && rmat(alpha1, x, y) > kUpdateAlphaThreshold) {
				float currErr = error_function(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow_rows, flow_cols,
					x, y, rmat2(flow, x, y));
					
				if (0 <= x-dx && x-dx < flow_cols && 0 <= y-dy && y-dy < flow_rows) {
					propose_flow_update(
						I0x, I0x_step, I0x_offset,
						I0y, I0y_step, I0y_offset,
						I1x, I1x_step, I1x_offset,
						I1y, I1y_step, I1y_offset,
						blurred, blurred_step, blurred_offset,
						flow, flow_step, flow_offset, flow_rows, flow_cols,
						x, y, rmat2(flow, x-dx, y-dy), &currErr);
				}
				
				wmat2(flow, x, y) -= kGradientStepSize * error_gradient(
					I0x, I0x_step, I0x_offset,
					I0y, I0y_step, I0y_offset,
					I1x, I1x_step, I1x_offset,
					I1y, I1y_step, I1y_offset,
					blurred, blurred_step, blurred_offset,
					flow, flow_step, flow_offset, flow_rows, flow_cols,
					x, y, currErr);
			}
		}
	}
}




