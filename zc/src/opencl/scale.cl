/**
 * @brief the following macros are used for accessing img(x, y)
 */
#define rmat32fc1(addr, x, y) ((__global const float*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc2(addr, x, y) ((__global const float2*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc4(addr, x, y) ((__global const float4*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc1(addr, x, y) ((__global float*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc2(addr, x, y) ((__global float2*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc4(addr, x, y) ((__global float4*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]

#define rmat(addr, x, y) 	rmat32fc1(addr, x, y)
#define rmat2(addr, x, y) 	rmat32fc2(addr, x, y)
#define rmat4(addr, x, y) 	rmat32fc4(addr, x, y)
#define wmat(addr, x, y) 	wmat32fc1(addr, x, y)
#define wmat2(addr, x, y) 	wmat32fc2(addr, x, y)
#define wmat4(addr, x, y) 	wmat32fc4(addr, x, y)

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
		wmat(dst, x, y) = factor * rmat(src, x, y);
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
		wmat2(dst, x, y) = factor * rmat2(src, x, y);
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
		wmat4(dst, x, y) = factor * rmat4(src, x, y);
	}
}


/**
 * @brief float umat scaling self
 */
__kernel void scale_self_32FC1(
	__global float* src, int src_step, int src_offset, int src_rows, int src_cols, 
	float factor)
{
	int x = get_global_id(0);
    int y = get_global_id(1);
	if (x < src_cols && y < src_rows) {
		wmat(src, x, y) *= factor;
	}
	
}
__kernel void scale_self_32FC2(
	__global float2* src, int src_step, int src_offset, int src_rows, int src_cols,
	float factor)
{
	int x = get_global_id(0);
    int y = get_global_id(1);
	if (x < src_cols && y < src_rows) {
		wmat2(src, x, y) *= factor;
	}
}
__kernel void scale_self_32FC4(
	__global float4* src, int src_step, int src_offset, int src_rows, int src_cols,
	float factor)
{
	int x = get_global_id(0);
    int y = get_global_id(1);
	if (x < src_cols && y < src_rows) {
		wmat4(src, x, y) *= factor;
	}
}
