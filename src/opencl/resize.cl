#define mat32fc1(img, x, y)	img##[((img##_offset + (y)*img##_step)>>2)+(x)]
#define mat32fc2(img, x, y) img##[((img##_offset + (y)*img##_step)>>3)+(x)]
#define mat32fc4(img, x, y) img##[((img##_offset + (y)*img##_step)>>4)+(x)]
#define mat(img, x, y) 		mat32fc1(img, x, y)
#define mat2(img, x, y) 	mat32fc2(img, x, y)
#define mat4(img, x, y) 	mat32fc4(img, x, y)

/**
 * @reference: opencv cpu implementation and https://en.wikipedia.org/wiki/Bicubic_interpolation
 */
float get_bicubic_32fc1(float p0, float p1, float p2, float p3, float x)
{
	const float A = -0.75f;
    const float w0 = ((A*(x + 1) - 5*A)*(x + 1) + 8*A)*(x + 1) - 4*A;
    const float w1 = ((A + 2)*x - (A + 3))*x*x + 1;
    const float w2 = ((A + 2)*(1 - x) - (A + 3))*(1 - x)*(1 - x) + 1;
    const float w3 = 1.f - w0 - w1 - w2;
	//const float w3 = ((A*(2 - x) - 5*A)*(2 - x) + 8*A)*(2 - x) - 4*A;
	return p0*w0 + p1*w1 + p2*w2 + p3*w3;
}

__kernel void resize_32FC1(
	__global const float* src, int src_step, int src_offset, int src_rows, int src_cols, 
	__global float* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	float factor_x, float factor_y)
{
	int dst_x = get_global_id(0);
    int dst_y = get_global_id(1);
	if (dst_x < dst_cols && dst_y < dst_rows) {
	
		float src_x = dst_x*factor_x;
		float src_y = dst_y*factor_y;
		
		int x0 = convert_int_rtz(src_x);
		int y0 = convert_int_rtz(src_y);
		
		float xR = src_x - x0;
		float yR = src_y - y0;
		
		float u[4];
		float v[4];
		
		for (int dy=-1; dy < 3; ++dy) {
			u[0] = mat(src, max(x0 - 1, 0), 			min(max(0, y0 + dy), src_rows - 1));
			u[1] = mat(src, x0, 						min(max(0, y0 + dy), src_rows - 1));	
			u[2] = mat(src, x0 + 1, 					min(max(0, y0 + dy), src_rows - 1));
			u[3] = mat(src, min(x0 + 2, src_cols - 1), 	min(max(0, y0 + dy), src_rows - 1));
			v[dy+1] = get_bicubic_32fc1(u[0], u[1], u[2], u[3], xR);
		}
		
		mat(dst, dst_x, dst_y) = get_bicubic_32fc1(v[0], v[1], v[2], v[3], yR);
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

__kernel void resize_32FC2(
	__global const float2* src, int src_step, int src_offset, int src_rows, int src_cols, 
	__global float2* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	float factor_x, float factor_y)
{
	int dst_x = get_global_id(0);
    int dst_y = get_global_id(1);
	if (dst_x < dst_cols && dst_y < dst_rows) {
	
		float src_x = dst_x*factor_x;
		float src_y = dst_y*factor_y;
		
		int x0 = convert_int_rtz(src_x);
		int y0 = convert_int_rtz(src_y);
		
		float xR = src_x - x0;
		float yR = src_y - y0;
		
		float2 u[4];
		float2 v[4];
		
		for (int dy=-1; dy < 3; ++dy) {
			u[0] = mat2(src, max(x0 - 1, 0), 			min(max(0, y0 + dy), src_rows - 1));
			u[1] = mat2(src, x0, 						min(max(0, y0 + dy), src_rows - 1));	
			u[2] = mat2(src, x0 + 1, 					min(max(0, y0 + dy), src_rows - 1));
			u[3] = mat2(src, min(x0 + 2, src_cols - 1), min(max(0, y0 + dy), src_rows - 1));
			v[dy+1] = get_bicubic_32fc2(u[0], u[1], u[2], u[3], xR);
		}
		
		mat2(dst, dst_x, dst_y) = get_bicubic_32fc2(v[0], v[1], v[2], v[3], yR);
	}
}


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

__kernel void resize_8UC4(
	__global const uchar4* src, int src_step, int src_offset, int src_rows, int src_cols, 
	__global uchar4* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
	float factor_x, float factor_y)
{
	int dst_x = get_global_id(0);
    int dst_y = get_global_id(1);
	if (dst_x < dst_cols && dst_y < dst_rows) {
	
		float src_x = dst_x*factor_x;
		float src_y = dst_y*factor_y;
		
		int x0 = convert_int_rtz(src_x);
		int y0 = convert_int_rtz(src_y);
		
		float xR = src_x - x0;
		float yR = src_y - y0;
		
		uchar4 u[4];
		float4 v[4];
		
		for (int dy=-1; dy < 3; ++dy) {
			u[0] = mat(src, max(x0 - 1, 0), 			min(max(0, y0 + dy), src_rows - 1));
			u[1] = mat(src, x0, 						min(max(0, y0 + dy), src_rows - 1));	
			u[2] = mat(src, x0 + 1, 					min(max(0, y0 + dy), src_rows - 1));
			u[3] = mat(src, min(x0 + 2, src_cols - 1), 	min(max(0, y0 + dy), src_rows - 1));
			v[dy+1] = get_bicubic_8uc4(
				(float4)(u[0].x, u[0].y, u[0].z, u[0].w), 
				(float4)(u[1].x, u[1].y, u[1].z, u[1].w),  
				(float4)(u[2].x, u[2].y, u[2].z, u[2].w),  
				(float4)(u[3].x, u[3].y, u[3].z, u[3].w),  
				xR);
		}
		
		float4 s = get_bicubic_8uc4(v[0], v[1], v[2], v[3], yR);
		mat(dst, dst_x, dst_y) = (uchar4)(convert_uchar_sat(s.x), convert_uchar_sat(s.y), convert_uchar_sat(s.z), convert_uchar_sat(s.w)); 
	}
}