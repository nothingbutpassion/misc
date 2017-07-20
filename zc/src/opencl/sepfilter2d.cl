/**
 * @brief the following macros are used for accessing img(x, y)
 */
#define rmat8uc4(addr, x, y) ((__global const uchar4*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc1(addr, x, y) ((__global const float*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc2(addr, x, y) ((__global const float2*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]

#define wmat8uc4(addr, x, y) ((__global uchar4*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
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

#define KERNEL_X_DATA DIG(0.1065069810f)DIG(0.7869860530f)DIG(0.1065069810f)
#define KERNEL_Y_DATA DIG(0.1065069810f)DIG(0.7869860530f)DIG(0.1065069810f)

#define DIG(a) a,
__constant float kernel_x[] = { KERNEL_X_DATA };
__constant float kernel_y[] = { KERNEL_Y_DATA };


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
			int src_y = EXTRAPOLATE(y + dy, dst_rows - 1);
			sum += rmat2(src, x, src_y)*kernel_y[dy+row_kernel_size/2];
		}
		wmat2(dst, x, y) = sum;
	}
}

__kernel void filter_row_8UC4(
	__global const uchar4* src, int src_step, int src_offset,
	__global uchar4* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	int col_kernel_size)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float4 sum = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
		for (int dx=-col_kernel_size/2; dx <= col_kernel_size/2; ++dx) {
			int src_x = EXTRAPOLATE(x + dx, dst_cols - 1);
			sum += convert_float4(rmat8uc4(src, src_x, y))*kernel_x[dx+col_kernel_size/2];
		}
		wmat8uc4(dst, x, y) = convert_uchar4_sat(sum);
	}
}

__kernel void filter_col_8UC4(
	__global const uchar4* src, int src_step, int src_offset,
	__global uchar4* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	int row_kernel_size)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float4 sum = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
		for (int dy=-row_kernel_size/2; dy <= row_kernel_size/2; ++dy) {
			int src_y = EXTRAPOLATE(y + dy, dst_rows - 1);
			sum += convert_float4(rmat8uc4(src, x, src_y))*kernel_y[dy+row_kernel_size/2];
		}
		wmat8uc4(dst, x, y) = convert_uchar4_sat(sum);
	}
}


#define MAX_LOCAL_ROWS 32
#define MAX_LOCAL_COLS 32

__kernel void filter_row_v2_32FC1(
	__global const float* src, int src_step, int src_offset,
	__global float* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	int col_kernel_size)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	
	int lx = get_local_id(0);
	int ly = get_local_id(1);
	
	int group_size = get_local_size(0);
	int radus = col_kernel_size/2;
	
	__local float ls[MAX_LOCAL_ROWS][MAX_LOCAL_COLS];
	
	if (x < dst_cols && y < dst_rows) {
		int group_start = get_group_id(0)*get_local_size(0);
		if (group_start + group_size > dst_cols) {
			group_size = dst_cols - group_start;
		}
		
		ls[ly][lx + radus] = rmat(src, x, y);
		if (lx < radus) {
			int x1 = EXTRAPOLATE(x - radus, dst_cols - 1);
			int x2 = EXTRAPOLATE(x + group_size, dst_cols - 1);
			ls[ly][lx] = rmat(src, x1, y);
			ls[ly][lx + group_size + radus] = rmat(src, x2, y);
		}
	}
	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	if (x < dst_cols && y < dst_rows) {
		float sum = 0.0f;
		for (int k = 0; k < col_kernel_size; ++k) {
			sum += kernel_x[k]*ls[ly][lx + k];
		}
		wmat(dst, x, y) = sum;
	}
}

__kernel void filter_col_v2_32FC1(
	__global const float* src, int src_step, int src_offset,
	__global float* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	int col_kernel_size)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	
	int lx = get_local_id(0);
	int ly = get_local_id(1);
	
	int group_size = get_local_size(1);
	int radus = col_kernel_size/2;
	
	__local float ls[MAX_LOCAL_ROWS][MAX_LOCAL_COLS];
	
	if (x < dst_cols && y < dst_rows) {
		int group_start = get_group_id(1)*get_local_size(1);
		if (group_start + group_size > dst_rows) {
			group_size = dst_rows - group_start;
		}
		
		ls[ly + radus ][lx] = rmat(src, x, y);
		if (ly < radus) {
			int y1 = EXTRAPOLATE(y - radus, dst_rows - 1);
			int y2 = EXTRAPOLATE(y + group_size, dst_rows - 1);
			ls[ly][lx] = rmat(src, x, y1);
			ls[ly + group_size + radus][lx] = rmat(src, x, y2);
		}
	}
	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	if (x < dst_cols && y < dst_rows) {
		float sum = 0.0f;
		for (int k = 0; k < col_kernel_size; ++k) {
			sum += kernel_y[k]*ls[ly + k][lx];
		}
		wmat(dst, x, y) = sum;
	}
}

