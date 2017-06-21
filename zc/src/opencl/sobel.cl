/**
 * @brief the following macros are used for accessing img(x, y)
 */
#define rmat32fc1(addr, x, y) ((__global const float*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc1(addr, x, y) ((__global float*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat(addr, x, y) 	rmat32fc1(addr, x, y)
#define wmat(addr, x, y) 	wmat32fc1(addr, x, y)

/*
__constant sobel_x_kernel[3][3] = { 
	{-1,  0, 1},
	{-2,  0, 2},
	{-1,  0, 1}
};
__constant int sobel_y_kernel[3][3] = { 
	{-1, -2, -1},
	{ 0,  0,  0},
	{ 1,  2,  1} 
};

__kernel void sobel_x_3_border_replicate(
	__global const float* src, int src_step, int src_offset,
	__global float* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float sum = 0.0f;
		for (int dy=-1; dy < 2; ++dy) {
			for (int dx=-1; dx < 2; ++dx) {
				int src_x = min(max(0, x+dx), dst_cols - 1);
				int src_y = min(max(0, y+dy), dst_rows - 1);			
				sum += rmat(src, src_x, src_y)*sobel_x_kernel[dy+1][dx+1];
			}
		}
		wmat(dst, x, y) = sum;
	}
}

__kernel void sobel_y_3_border_replicate(
	__global const float* src, int src_step, int src_offset,
	__global float* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float sum = 0.0f;
		for (int dy=-1; dy < 2; ++dy) {
			for (int dx=-1; dx < 2; ++dx) {
				int src_x = min(max(0, x+dx), dst_cols - 1);
				int src_y = min(max(0, y+dy), dst_rows - 1);			
				sum += rmat(src, src_x, src_y)*sobel_y_kernel[dy+1][dx+1];
			}
		}
		wmat(dst, x, y) = sum;
	}
}
*/

__kernel void sobel_x_1_border_replicate(
	__global const float* src, int src_step, int src_offset,
	__global float* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float sum = 0.0f;
		int x2 = min(max(0, x + 1), dst_cols - 1);
		int x1 = min(max(0, x - 1), dst_cols - 1);
		wmat(dst, x, y) = rmat(src, x2, y) - rmat(src, x1, y);
	}
}

__kernel void sobel_y_1_border_replicate(
	__global const float* src, int src_step, int src_offset,
	__global float* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float sum = 0.0f;
		int y2 = min(max(0, y + 1), dst_rows - 1);
		int y1 = min(max(0, y - 1), dst_rows - 1);
		wmat(dst, x, y) = rmat(src, x, y2) - rmat(src, x, y1);
	}
}
