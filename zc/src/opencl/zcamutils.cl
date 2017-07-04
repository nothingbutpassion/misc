/**
 * @brief the following macros are used for accessing img(x, y)
 */

#define rmat8uc3(addr, x, y) 		((__global const uchar3*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step + (x)*3))[0]
#define rmat8uc4(addr, x, y) 		((__global const uchar4*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc2(addr, x, y) 	((__global const float2*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat32fc1(addr, x, y) 	((__global const float*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]

#define wmat8uc3(addr, x, y, i) (((__global uchar*)addr) + addr##_offset + (y)*addr##_step + (x)*3)[i]
#define wmat8uc4(addr, x, y) 		((__global uchar4*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc2(addr, x, y) 	((__global float2*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat32fc1(addr, x, y) 	((__global float*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]

#define rmat(addr, x, y) 			rmat32fc1(addr, x, y)
#define rmat2(addr, x, y) 		rmat32fc2(addr, x, y)
#define wmat2(addr, x, y) 		wmat32fc1(addr, x, y)
#define wmat2(addr, x, y) 		wmat32fc2(addr, x, y)

__kernel void smooth_image(
	__global uchar4* pano, int pano_step, int pano_offset, int pano_rows, int pano_cols,
	__global const uchar4* previous, int previous_step, int previous_offset,
	float thresh_hold, int is_pano)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < pano_cols && y < pano_rows) {
		uchar4 cur = rmat8uc4(pano, x, y);
		uchar4 pre = rmat8uc4(previous, x, y);
		
		float motion = (fabs((float)cur.s0 - (float)pre.s0) 
									+ fabs((float)cur.s1 - (float)pre.s1) 
									+ fabs((float)cur.s2 - (float)pre.s2)) / (255.0f*3.0f);
		
		const float top_bottom_thresh = 0.3f;
		float delta;
		if (y> pano_rows*(1.0f - top_bottom_thresh) && is_pano) {
			delta = (pano_rows - y) / (pano_rows * top_bottom_thresh);
		} else if (y < pano_rows* top_bottom_thresh  && is_pano) {
			delta = y / (pano_rows * top_bottom_thresh);
		} else {
			delta = 1.01;
		}
		if (delta < 1.01) {
			if (delta < 0.8) {
				delta = 0.8;
			}
			motion *= delta;
		}
		
		if (motion < thresh_hold/2) {
			wmat8uc4(pano, x, y) = (uchar4) (pre.s0, pre.s1, pre.s2, cur.s3);
		} else if (motion < thresh_hold) {
			float factor = motion / thresh_hold / 2;
			wmat8uc4(pano, x, y) = (uchar4)(convert_uchar_sat(factor*cur.s0 + (1 - factor)*pre.s0), 
																		  convert_uchar_sat(factor*cur.s1 + (1 - factor)*pre.s1),
																		  convert_uchar_sat(factor*cur.s2 + (1 - factor)*pre.s2),
																		  cur.s3);
		}	
	}
}


__kernel void offset_horizontal_wrap(
	__global float2* warp, int warp_step, int warp_offset, int warp_rows, int warp_cols,
	float offset)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < warp_cols && y < warp_rows)  {
		float srcX =  (float)x - offset;
		if (srcX < 0) {
			srcX += warp_cols;
		}
		
		if (srcX >= warp_cols) {
			srcX -= warp_cols;
		}
		
		wmat2(warp, x, y) = (float2) (srcX, (float)y);
	}	
}



__kernel void remove_chunk_line(
	__global uchar4* chunk, int chunk_step, int chunk_offset, int chunk_rows, int chunk_cols)
{
	int y = get_global_id(0);
	if (y < chunk_rows) {
		wmat8uc4(chunk, chunk_cols - 1, y) = rmat8uc4(chunk, chunk_cols - 2, y);
	}
}


