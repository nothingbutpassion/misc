/**
 * @brief float umat scaling
 */
__kernel void scale_32FC1(
	__global const float* src, int src_step, int src_offset, 
	__global float* dst, int dst_step, int dst_offset,
	float factor)
{
	int x = get_global_id(0);
    int y = get_global_id(1);
	int src_index = ((src_offset + y*src_step) >> 2) + x;
	int dst_index = ((dst_offset + y*dst_step) >> 2) + x;
	dst[dst_index] = factor * src[src_index]; 
}
__kernel void scale_32FC2(
	__global const float2* src, int src_step, int src_offset, 
	__global float2* dst, int dst_step, int dst_offset,
	float factor)
{
	int x = get_global_id(0);
    int y = get_global_id(1);
	int src_index = ((src_offset + y*src_step) >> 4) + x;
	int dst_index = ((dst_offset + y*dst_step) >> 4) + x;
	dst[dst_index] = factor * src[src_index]; 
}
__kernel void scale_32FC4(
	__global const float4* src, int src_step, int src_offset, 
	__global float4* dst, int dst_step, int dst_offset,
	float factor)
{
	int x = get_global_id(0);
    int y = get_global_id(1);
	int src_index = ((src_offset + y*src_step) >> 4) + x;
	int dst_index = ((dst_offset + y*dst_step) >> 4) + x;
	dst[dst_index] = factor * src[src_index]; 
}

/**
 * @brief motion detection
 */
__kernel void motion_detection(
	__global const char4* cur, int cur_step, int cur_offset,
	__global const char4* pre, int pre_step, int pre_offset,
	__global float* motion, int motion_step, int motion_offset) 
{
    int x = get_global_id(0);
    int y = get_global_id(1);
	int cur_index = ((cur_offset + y*cur_step) >> 2) + x;
	int pre_index = ((pre_offset + y*pre_step) >> 2) + x;
	int motion_index = ((motion_offset + y*motion_step) >> 2) + x;
	motion[motion_index] = 
		(abs(cur[cur_index].x - pre[pre_index].x) 
		+ abs(cur[cur_index].y - pre[pre_index].y)
		+ abs(cur[cur_index].z - pre[pre_index].z))/(3.0f*255.0f);
}

/**
 * @brief adjust flow toward previous
 */
__kernel void adjust_flow_toward_previous(
	__global float2* cur, int cur_step, int cur_offset,
	__global const float2* pre, int pre_step, int pre_offset,
	__global const float* motion, int motion_step, int motion_offset) 
{

    int x = get_global_id(0);
    int y = get_global_id(1);
	int cur_index = ((cur_offset + y*cur_step) >> 3) + x;
	int pre_index = ((pre_offset + y*pre_step) >> 3) + x;
	int motion_index = ((motion_offset + y*motion_step) >> 2) + x;
	float w = 1.0f - motion[motion_index];
	cur[cur_index] = (1.0f - w)*cur[cur_index] + w*pre[pre_index];	
}

/**
 * @brief estimate flow by searching the closet rect area
 */
__constant int kPyrMinImageSize         = 24;   	// the minimum image size in pyramid 
__constant float kUpdateAlphaThreshold	= 0.9f;		// pixels with alpha below this aren't updated by proposals
__constant int kMaxPercentage 			= 20;		// NOTES：this value can't be zero, and should be same as the one in host program 

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
				
				int i0_index = ((I0_offset + d0y*I0_step) >> 2) + d0x;
				int i1_index = ((I1_offset + d1y*I1_step) >> 2) + d1x;
				int alpha0_index = ((alpha0_offset + d0y*alpha0_step) >> 2) + d0x;
				int alpha1_index = ((alpha1_offset + d1y*alpha1_step) >> 2) + d1x;
				
				sad += fabs(I0[i0_index] - I1[i1_index]);
				alpha += alpha0[alpha0_index] * alpha1[alpha1_index];
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
	int alpha0_index = ((alpha0_offset + i0y*alpha0_step) >> 2) + i0x;
	int flow_index = ((flow_offset + i0y*flow_step) >> 3) + i0x;
	
	if (alpha0[alpha0_index] > kUpdateAlphaThreshold) {
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
		flow[flow_index] = (float2)(i1xBest - i0x, i1yBest - i0y);
	}
}

/**
 * @brief low alpha flow diffusion
 */
__kernel void alpha_flow_diffusion(
	__global const float* alpha0, int alpha0_step, int alpha0_offset,
	__global const float* alpha1, int alpha1_step, int alpha1_offset,
	__global const float2* blurred_flow, int blurred_step, int blurred_offset,
	__global float2* flow, int flow_step, int flow_offset)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int alpha0_index = ((alpha0_offset + y*alpha0_step) >> 2) + x;
	int alpha1_index = ((alpha1_offset + y*alpha1_step) >> 2) + x;
	int blurred_index = ((blurred_offset + y*blurred_step) >> 3) + x;
	int flow_index = ((flow_offset + y*flow_step) >> 3) + x;
	float diffusionCoef = 1 - alpha0[alpha0_index]*alpha1[alpha1_index]; 
	flow[flow_index] = diffusionCoef*blurred_flow[blurred_index] + (1 - diffusionCoef)*flow[flow_index];
}


/**
 * @brief two pass sweeping  
 * 1) from top/left 	=>  bottom/right
 * 2) from bottom/right => top/left
 */
static inline float getPixBilinear32FExtend(const Mat& img, float x, float y) {
	const cv::Size& imgSize = img.size();
	x                 = min(imgSize.width - 2.0f, max(0.0f, x));
	y                 = min(imgSize.height - 2.0f, max(0.0f, y));
	const int x0      = int(x);
	const int y0      = int(y);
	const float xR    = x - float(x0);
	const float yR    = y - float(y0);
	const float* p    = img.ptr<float>(y0);
	const float f00   = *(p + x0);
	const float f01   = *(p + x0 + img.cols);
	const float f10   = *(p + x0 + 1);
	const float f11   = *(p + x0 + img.cols + 1);
	const float a1    = f00;
	const float a2    = f10 - f00;
	const float a3    = f01 - f00;
	const float a4    = f00 + f11 - f10 - f01;
	return a1 + a2 * xR + a3 * yR + a4 * xR * yR;
}








