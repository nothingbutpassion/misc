/**
 * @brief the following macros are used for accessing img(x, y)
 */
#define rmat32fc1(addr, x, y) ((__global const float*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc2(addr, x, y) ((__global const float2*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc1(addr, x, y) ((__global float*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc2(addr, x, y) ((__global float2*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]

#define rmat(addr, x, y) 		rmat32fc1(addr, x, y)
#define rmat2(addr, x, y) 	rmat32fc2(addr, x, y)
#define wmat(addr, x, y) 		wmat32fc1(addr, x, y)
#define wmat2(addr, x, y) 	wmat32fc2(addr, x, y)

#ifdef BORDER_REPLICATE
// BORDER_REPLICATE: aaaaaa|abcdefgh|hhhhhhh
#define EXTRAPOLATE(i, m)	 (i) < 0 ? 0 : ((i) > (m) ? (m) : (i))
#else
// BORDER_REFLECT_101: gfedcb|abcdefgh|gfedcba
#define EXTRAPOLATE(i, m)	(i) < 0 ? -(i) : ((i) > (m) ? ((m)<<1)-(i) : (i))
#endif

//#define KERNEL_X_DATA DIG(0.1065069810f)DIG(0.7869860530f)DIG(0.1065069810f)
//#define KERNEL_Y_DATA DIG(0.1065069810f)DIG(0.7869860530f)DIG(0.1065069810f)

#define DIG(a) a,
__constant float kernel_y[] = { KERNEL_X_DATA };
__constant float kernel_x[] = { KERNEL_Y_DATA };

__kernel void filter_row_32FC1(
	__global const float* src, int src_step, int src_offset,
	__global float* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	int col_kernel_size)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float sum = 0;
		for (int dx=-col_kernel_size/2; dx <= col_kernel_size/2; ++dx) {
			//int src_x = min(max(0, x + dx), dst_cols - 1);
			int src_x = EXTRAPOLATE(x + dx, dst_cols - 1);
			sum += rmat(src, src_x, y)*kernel_x[dx+col_kernel_size/2];
		}
		wmat(dst, x, y) = sum;
	}
}

__kernel void filter_col_32FC1(
	__global const float* src, int src_step, int src_offset,
	__global float* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	int row_kernel_size)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float sum = 0;
		for (int dy=-row_kernel_size/2; dy <= row_kernel_size/2; ++dy) {
			// int src_y = min(max(0, y + dy), dst_rows - 1);
			int src_y = EXTRAPOLATE(y + dy, dst_rows - 1);
			sum += rmat(src, x, src_y)*kernel_y[dy+row_kernel_size/2];
		}
		wmat(dst, x, y) = sum;
	}
}



__kernel void filter_row_32FC2(
	__global const float2* src, int src_step, int src_offset,
	__global float2* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	int col_kernel_size)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float2 sum = (float2)(0.0f, 0.0f);
		for (int dx=-col_kernel_size/2; dx <= col_kernel_size/2; ++dx) {
			//int src_x = min(max(0, x + dx), dst_cols - 1);
			int src_x = EXTRAPOLATE(x + dx, dst_cols - 1);
			sum += rmat2(src, src_x, y)*kernel_x[dx+col_kernel_size/2];
		}
		wmat2(dst, x, y) = sum;
	}
}

__kernel void filter_col_32FC2(
	__global const float2* src, int src_step, int src_offset,
	__global float2* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	int row_kernel_size)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float2 sum = (float2)(0.0f, 0.0f);
		for (int dy=-row_kernel_size/2; dy <= row_kernel_size/2; ++dy) {
			//int src_y = min(max(0, y + dy), dst_rows - 1);
			int src_y = EXTRAPOLATE(y + dy, dst_rows - 1);
			sum += rmat2(src, x, src_y)*kernel_y[dy+row_kernel_size/2];
		}
		wmat2(dst, x, y) = sum;
	}
}

