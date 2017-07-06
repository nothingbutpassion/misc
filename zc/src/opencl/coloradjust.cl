/**
 * @brief the following macros are used for accessing img(x, y)
 */
#define rmat16uc1(addr, x, y) ((__global const ushort*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define rmat16uc4(addr, x, y) ((__global const ushort4*)(((__global const uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat16uc1(addr, x, y) ((__global ushort*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]
#define wmat16uc4(addr, x, y) ((__global ushort4*)(((__global uchar*)addr) + addr##_offset + (y)*addr##_step))[x]


__kernel void anti_gamma_lut_adjust(
	__global ushort* lut, int lut_step, int lut_offset,
	__global ushort4* img, int img_step, int img_offset, int img_rows, int img_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < img_cols && y < img_rows) {
		ushort4 v = rmat16uc4(img, x, y);
		v.s0 = rmat16uc1(lut, v.s0, 0);
		v.s1 = rmat16uc1(lut, v.s1, 0);
		v.s2 = rmat16uc1(lut, v.s2, 0);
		wmat16uc4(img, x, y) = v;
	}
}

/* @deprecated
__kernel void anti_gamma_lut_adjust_v2(
	__global ushort* lut, int lut_step, int lut_offset,
	__global ushort4* img, int img_step, int img_offset, int img_rows, int img_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < img_cols && y < img_rows) {
		ushort4 v = rmat16uc4(img, x, y);
		v <<= 8;
		v.s0 = rmat16uc1(lut, v.s0, 0);
		v.s1 = rmat16uc1(lut, v.s1, 0);
		v.s2 = rmat16uc1(lut, v.s2, 0);
		wmat16uc4(img, x, y) = v;
	}
}
*/

__kernel void gamma_lut_adjust(
	__global ushort* lut, int lut_step, int lut_offset,
	__global ushort4* img, int img_step, int img_offset, int img_rows, int img_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < img_cols && y < img_rows) {
		ushort4 v = rmat16uc4(img, x, y);
		v.s0 = rmat16uc1(lut, v.s0, 0);
		v.s1 = rmat16uc1(lut, v.s1, 0);
		v.s2 = rmat16uc1(lut, v.s2, 0);
		wmat16uc4(img, x, y) = v;
	}
}

/* @deprecated
__kernel void gamma_lut_adjust_v2(
	__global ushort* lut, int lut_step, int lut_offset,
	__global ushort4* img, int img_step, int img_offset, int img_rows, int img_cols)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < img_cols && y < img_rows) {
		ushort4 v = rmat16uc4(img, x, y);
		v.s0 = rmat16uc1(lut, v.s0, 0);
		v.s1 = rmat16uc1(lut, v.s1, 0);
		v.s2 = rmat16uc1(lut, v.s2, 0);
		v >>= 8; 
		wmat16uc4(img, x, y) = v;
	}
}
*/

__kernel void add_brightness_and_clamp_multi(
	__global ushort4* img, int img_step, int img_offset, int img_rows, int img_cols,
	float val)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < img_cols && y < img_rows) {
		ushort4 color = rmat16uc4(img, x, y);
		wmat16uc4(img, x, y) = (ushort4)(
			clamp((ushort)(color.s0 * val), (ushort)0,  (ushort)65525),
			clamp((ushort)(color.s1 * val), (ushort)0,  (ushort)65525),
			clamp((ushort)(color.s2 * val), (ushort)0,  (ushort)65525),
			color.s3);
	}
}

/* @deprecated
__kernel void add_brightness_and_clamp_multi_v2(
	__global ushort4* img, int img_step, int img_offset,
	__global ushort4* adjusted, int adjusted_step, int adjusted_offset, int adjusted_rows, int adjusted_cols,
	float val)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	if (x < adjusted_cols && y < adjusted_rows) {
		ushort4 color = rmat16uc4(img, x, y);
		wmat16uc4(adjusted, x, y) = (ushort4)(
			clamp((ushort)(color.s0 * val), (ushort)0,  (ushort)65525),
			clamp((ushort)(color.s1 * val), (ushort)0,  (ushort)65525),
			clamp((ushort)(color.s2 * val), (ushort)0,  (ushort)65525),
			color.s3);
	}
}
*/