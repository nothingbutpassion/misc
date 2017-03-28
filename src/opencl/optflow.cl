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