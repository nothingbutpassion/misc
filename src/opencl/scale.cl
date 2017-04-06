/**
 * @brief the following macros are used for accessing img(x, y)
 */
#define mat32fc1(img, x, y)	img##[((img##_offset + (y)*img##_step)>>2)+(x)]
#define mat32fc2(img, x, y) img##[((img##_offset + (y)*img##_step)>>3)+(x)]
#define mat32fc4(img, x, y) img##[((img##_offset + (y)*img##_step)>>4)+(x)]
#define mat(img, x, y) 		mat32fc1(img, x, y)
#define mat2(img, x, y) 	mat32fc2(img, x, y)
#define mat4(img, x, y) 	mat32fc4(img, x, y)

/**
 * @brief float umat scaling
 */
__kernel void scale_32FC1(
	__global const float* src, int src_step, int src_offset, int src_rows, int src_cols, 
	__global float* dst, int dst_step, int dst_offset,
	float factor)
{
	int x = get_global_id(0);
    int y = get_global_id(1);
	if (x < src_cols && y < src_rows) {
		mat(dst, x, y) = factor * mat(src, x, y);
	}
	
}
__kernel void scale_32FC2(
	__global const float2* src, int src_step, int src_offset, int src_rows, int src_cols,
	__global float2* dst, int dst_step, int dst_offset,
	float factor)
{
	int x = get_global_id(0);
    int y = get_global_id(1);
	if (x < src_cols && y < src_rows) {
		mat2(dst, x, y) = factor * mat2(src, x, y);
	}
}
__kernel void scale_32FC4(
	__global const float4* src, int src_step, int src_offset, int src_rows, int src_cols,
	__global float4* dst, int dst_step, int dst_offset,
	float factor)
{
	int x = get_global_id(0);
    int y = get_global_id(1);
	if (x < src_cols && y < src_rows) {
		mat4(dst, x, y) = factor * mat4(src, x, y);
	}
}