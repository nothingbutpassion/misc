/**
 * @brief the following macros are used for accessing img(x, y)
 */
//#define rmat8sc4(addr, x, y) ((__global const char4*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat8uc4(addr, x, y) ((__global const uchar4*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
//#define rmat32fc1(addr, x, y) ((__global const float*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc2(addr, x, y) ((__global const float2*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
//#define rmat32fc4(addr, x, y) ((__global const float4*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]

//#define wmat8sc4(addr, x, y) ((__global char4*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat8uc4(addr, x, y) ((__global uchar4*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
//#define wmat32fc1(addr, x, y) ((__global float*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc2(addr, x, y) ((__global float2*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
//#define wmat32fc4(addr, x, y) ((__global float4*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]

//#define rmat(addr, x, y) 	rmat32fc1(addr, x, y)
#define rmat2(addr, x, y) 	rmat32fc2(addr, x, y)
//#define rmat4(addr, x, y) 	rmat32fc4(addr, x, y)

//#define wmat(addr, x, y) 	wmat32fc1(addr, x, y)
#define wmat2(addr, x, y) 	wmat32fc2(addr, x, y)
//#define wmat4(addr, x, y) 	wmat32fc4(addr, x, y)

float4 get_bicubic_8uc4(float4 p0, float4 p1, float4 p2, float4 p3, float x)
{
	const float A = -0.75f;
	const float w0 = ((A*(x + 1) - 5*A)*(x + 1) + 8*A)*(x + 1) - 4*A;
	const float w1 = ((A + 2)*x - (A + 3))*x*x + 1;
	const float w2 = ((A + 2)*(1 - x) - (A + 3))*(1 - x)*(1 - x) + 1;
	const float w3 = 1.f - w0 - w1 - w2;
	//const float w3 = ((A*(2 - x) - 5*A)*(2 - x) + 8*A)*(2 - x) - 4*A;
	return p0*w0 + p1*w1 + p2*w2 + p3*w3;
}
__kernel void remap_8UC4_32FC2(
	__global const uchar4* src, int src_step, int src_offset, int src_rows, int src_cols,
	__global uchar4* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	__global const float2* map, int map_step, int map_offset)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		
		float2 src_pos = rmat2(map, x, y);
		int x0 = convert_int_rtz(src_pos.x);
		int y0 = convert_int_rtz(src_pos.y);
		float xR = src_pos.x - x0;
		float yR = src_pos.y - y0;
		
		uchar4 u[4];
		float4 v[4];
		
		for (int dy=-1; dy < 3; ++dy) {
			if (y0 + dy < 0 || y0 + dy >= src_rows) {
				v[dy+1] = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
			} else {
				u[0] = (0 <= x0 - 1 && x0 - 1 < src_cols) ? rmat8uc4(src, x0 - 1, y0 + dy) : (uchar4)(0, 0, 0, 0);
				u[1] = (0 <= x0     && x0     < src_cols) ? rmat8uc4(src, x0 	, y0 + dy) : (uchar4)(0, 0, 0, 0);
				u[2] = (0 <= x0 + 1 && x0 + 1 < src_cols) ? rmat8uc4(src, x0 + 1, y0 + dy) : (uchar4)(0, 0, 0, 0);
				u[3] = (0 <= x0 + 2 && x0 + 2 < src_cols) ? rmat8uc4(src, x0 + 2, y0 + dy) : (uchar4)(0, 0, 0, 0);
				v[dy+1] = get_bicubic_8uc4(
					(float4)(u[0].x, u[0].y, u[0].z, u[0].w), 
					(float4)(u[1].x, u[1].y, u[1].z, u[1].w),  
					(float4)(u[2].x, u[2].y, u[2].z, u[2].w),  
					(float4)(u[3].x, u[3].y, u[3].z, u[3].w),  
					xR);
			}				
		}
		wmat8uc4(dst, x, y) = convert_uchar4_sat(get_bicubic_8uc4(v[0], v[1], v[2], v[3], yR));
	}
}



float2 get_bicubic_32fc2(float2 p0, float2 p1, float2 p2, float2 p3, float x)
{
	const float A = -0.75f;
	const float w0 = ((A*(x + 1) - 5*A)*(x + 1) + 8*A)*(x + 1) - 4*A;
	const float w1 = ((A + 2)*x - (A + 3))*x*x + 1;
	const float w2 = ((A + 2)*(1 - x) - (A + 3))*(1 - x)*(1 - x) + 1;
	const float w3 = 1.f - w0 - w1 - w2;
	//const float w3 = ((A*(2 - x) - 5*A)*(2 - x) + 8*A)*(2 - x) - 4*A;
	return p0*w0 + p1*w1 + p2*w2 + p3*w3;
}
__kernel void remap_32FC2_32FC2(
	__global const float2* src, int src_step, int src_offset, int src_rows, int src_cols,
	__global float2* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	__global const float2* map, int map_step, int map_offset)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < dst_cols && y < dst_rows) {
		float2 src_pos = rmat2(map, x, y);
		int x0 = convert_int_rtz(src_pos.x);
		int y0 = convert_int_rtz(src_pos.y);
		float xR = src_pos.x - x0;
		float yR = src_pos.y - y0;
		
		float2 u[4];
		float2 v[4];
		for (int dy=-1; dy < 3; ++dy) {
			if (y0 + dy < 0 || y0 + dy >= src_rows) {
				v[dy+1] = (float2)(0.0f, 0.0f);
			} else {
				u[0] = (0 <= x0 - 1 && x0 - 1 < src_cols) ? rmat2(src, x0 - 1, y0 + dy) : (float2)(0.0f, 0.0f);
				u[1] = (0 <= x0     && x0     < src_cols) ? rmat2(src, x0 	, y0 + dy) : (float2)(0.0f, 0.0f);
				u[2] = (0 <= x0 + 1 && x0 + 1 < src_cols) ? rmat2(src, x0 + 1, y0 + dy) : (float2)(0.0f, 0.0f);
				u[3] = (0 <= x0 + 2 && x0 + 2 < src_cols) ? rmat2(src, x0 + 2, y0 + dy) : (float2)(0.0f, 0.0f);
				v[dy+1] = get_bicubic_32fc2(u[0], u[1], u[2], u[3], xR);
			}				
		}
		wmat2(dst, x, y) = get_bicubic_32fc2(v[0], v[1], v[2], v[3], yR);;
	}
}


