#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <arm_neon.h>
#include "hud_undistort.h"

struct Timer {
    Timer() { 
        gettimeofday(&start, nullptr);
    }
    int duration() {
        timeval end = {0};
        gettimeofday(&end, nullptr);
        return int(end.tv_sec - start.tv_sec)*1000 + (int(end.tv_usec) - int(start.tv_usec))/1000;
    }
private:
    timeval start = {0};
};


void raw_bilinear_remap(const unsigned char* src, int src_rows, int src_cols, int src_step, int src_channels,
					unsigned char* dst, int dst_rows, int dst_cols, int dst_step, int dst_channels,
					const unsigned char* map, int map_rows, int map_cols, int map_step) {
	for (int i = 0; i < map_rows; ++i) {
		for (int j = 0; j < map_cols; ++j) {
			float x = *(float*)(map + i*map_step + j*8);
			float y = *(float*)(map + i*map_step + j*8 + 4);
			int x0 = int(x);
			int y0 = int(y);
			int x1 = x0 + 1;
			int y1 = y0 + 1;
			float xr = x - x0;
			float yr = y - y0;
			for (int c = 0; c < src_channels; ++c) {
				float f00 = 0.0f;
				float f01 = 0.0f;
				float f10 = 0.0f;
				float f11 = 0.0f;
				if (0 <= x0 && x0 < src_cols && 0 <= y0 && y0 < src_rows)
					f00 = src[y0*src_step + 3*x0 + c];
				if (0 <= x0 && x0 < src_cols && 0 <= y1 && y1 < src_rows)
					f01 = src[y1*src_step + 3*x0 + c];
				if (0 <= x1 && x1 < src_cols && 0 <= y0 && y0 < src_rows)
					f10 = src[y0*src_step + 3*x1 + c];
				if (0 <= x1 && x1 < src_cols && 0 <= y1 && y1 < src_rows)
					f11 = src[y1*src_step + 3*x1 + c];
				dst[i*dst_step + 3*j + c] = (unsigned char)(f00*(1 - xr)*(1 - yr) + f10*xr*(1 - yr) + f01*(1 - xr)*yr + f11*xr*yr);
			}
		}
	}
}


void raw_bilinear_remap(
    const unsigned char* src, int src_rows, int src_cols, int src_step,
    unsigned char* dst, int dst_rows, int dst_cols, int dst_step,
    const float* mapx, int mapx_rows, int mapx_cols, int mapx_step,
    const float* mapy, int mapy_rows, int mapy_cols, int mapy_step) {

  	for (int i = 0; i < dst_rows; ++i) {
		for (int j = 0; j < dst_cols; ++j) {
			float x = *(float*)((char*)mapx + i*mapx_step + 4*j);
			float y = *(float*)((char*)mapy + i*mapy_step + 4*j);
			int x0 = int(x);
			int y0 = int(y);
			int x1 = x0 + 1;
			int y1 = y0 + 1;
			float xr = x - x0;
			float yr = y - y0;
			for (int c = 0; c < 4; ++c) {
				float f00 = 0.0f;
				float f01 = 0.0f;
				float f10 = 0.0f;
				float f11 = 0.0f;
				if (0 <= x0 && x0 < src_cols && 0 <= y0 && y0 < src_rows)
					f00 = src[y0*src_step + 4*x0 + c];
				if (0 <= x0 && x0 < src_cols && 0 <= y1 && y1 < src_rows)
					f01 = src[y1*src_step + 4*x0 + c];
				if (0 <= x1 && x1 < src_cols && 0 <= y0 && y0 < src_rows)
					f10 = src[y0*src_step + 4*x1 + c];
				if (0 <= x1 && x1 < src_cols && 0 <= y1 && y1 < src_rows)
					f11 = src[y1*src_step + 4*x1 + c];
				dst[i*dst_step + 4*j + c] = (unsigned char)(f00*(1 - xr)*(1 - yr) + f10*xr*(1 - yr) + f01*(1 - xr)*yr + f11*xr*yr);
			}
		}
	}  

}


#define bilinear_interpolate(pindex)                                        \
    if (p##pindex) {                                                        \
        uint8x8_t v0 = vld1_u8(src + vgetq_lane_s32(src_0, pindex));       \
        uint8x8_t v1 = vld1_u8(src + vgetq_lane_s32(src_1, pindex));       \
        float32x4_t v00, v10, v01, v11;                     \
        v00 = vsetq_lane_f32(vget_lane_u8(v0, 0), v00, 0);  \
        v00 = vsetq_lane_f32(vget_lane_u8(v0, 1), v00, 1);  \
        v00 = vsetq_lane_f32(vget_lane_u8(v0, 2), v00, 2);  \
        v00 = vsetq_lane_f32(vget_lane_u8(v0, 3), v00, 3);  \
        v10 = vsetq_lane_f32(vget_lane_u8(v0, 4), v10, 0);  \
        v10 = vsetq_lane_f32(vget_lane_u8(v0, 5), v10, 1);  \
        v10 = vsetq_lane_f32(vget_lane_u8(v0, 6), v10, 2);  \
        v10 = vsetq_lane_f32(vget_lane_u8(v0, 7), v10, 3);  \
        v01 = vsetq_lane_f32(vget_lane_u8(v1, 0), v01, 0);  \
        v01 = vsetq_lane_f32(vget_lane_u8(v1, 1), v01, 1);  \
        v01 = vsetq_lane_f32(vget_lane_u8(v1, 2), v01, 2);  \
        v01 = vsetq_lane_f32(vget_lane_u8(v1, 3), v01, 3);  \
        v11 = vsetq_lane_f32(vget_lane_u8(v1, 4), v11, 0);  \
        v11 = vsetq_lane_f32(vget_lane_u8(v1, 5), v11, 1);  \
        v11 = vsetq_lane_f32(vget_lane_u8(v1, 6), v11, 2);  \
        v11 = vsetq_lane_f32(vget_lane_u8(v1, 7), v11, 3);  \
        float32x4_t v = vaddq_f32(vaddq_f32(vmulq_f32(v00, r00), vmulq_f32(v10, r10)),          \
                                  vaddq_f32(vmulq_f32(v01, r01), vmulq_f32(v11, r11)));         \
        dst[vgetq_lane_s32(dst_p##pindex, 0)] = (unsigned char)vgetq_lane_f32(v, 0);            \
        dst[vgetq_lane_s32(dst_p##pindex, 1)] = (unsigned char)vgetq_lane_f32(v, 1);            \
        dst[vgetq_lane_s32(dst_p##pindex, 2)] = (unsigned char)vgetq_lane_f32(v, 2);            \
        dst[vgetq_lane_s32(dst_p##pindex, 3)] = (unsigned char)vgetq_lane_f32(v, 3);            \
    }else {                                                                                     \
       *(uint32_t*) &dst[vgetq_lane_s32(dst_p0, 0)] = 0;                                        \
    }


void neno_bilinear_remap(
    const unsigned char* src, int src_rows, int src_cols, int src_step,
    unsigned char* dst, int dst_rows, int dst_cols, int dst_step,
    const float* mapx, int mapx_rows, int mapx_cols, int mapx_step,
    const float* mapy, int mapy_rows, int mapy_cols, int mapy_step) {

    float32x4_t zero = vdupq_n_f32(0);
    float32x4_t one = vdupq_n_f32(1);
    float32x4_t x_max = vdupq_n_f32(src_cols-1);    
    float32x4_t y_max = vdupq_n_f32(src_rows-1);
    int32x4_t l = vdupq_n_s32(1);
    int32x4_t s_step = vdupq_n_s32(src_step);
    int32x4_t d_step = vdupq_n_s32(dst_step);
    for (int i = 0; i < dst_rows; ++i) {
        for (int j = 0; j < dst_cols; j +=4) {
            float32x4_t x = vld1q_f32((const float32_t*) ((const char*)mapx + i*mapx_step + j*4));
            float32x4_t y = vld1q_f32((const float32_t*) ((const char*)mapy + i*mapy_step + j*4));
            int32x4_t x0 = vcvtq_s32_f32(x);
            int32x4_t y0 = vcvtq_s32_f32(y);    
            int32x4_t x1 = vaddq_s32(x0, l);
            int32x4_t y1 = vaddq_s32(y0, l);
            float32x4_t xr = vsubq_f32(x, vcvtq_f32_s32(x0));   // xr
            float32x4_t yr = vsubq_f32(y, vcvtq_f32_s32(y0));   // yr
            float32x4_t l_xr = vsubq_f32(one, xr);              // 1-xr
            float32x4_t l_yr = vsubq_f32(one, yr);              // 1-yr
            float32x4_t r00 = vmulq_f32(l_xr, l_yr);            // (1-xr)*(1-yr)
            float32x4_t r10 = vmulq_f32(xr,   l_yr);            // xr*(1-yr)
            float32x4_t r01 = vmulq_f32(l_xr, yr);              // (1-xr)*yr
            float32x4_t r11 = vmulq_f32(xr,   yr);              // xr*yr
                
            // If the comparison is true for a lane, the result in that lane is all bits set to one. 
            // If the comparison is false for a lane, all bits are set to zero.
            // 0 <= x && x < x_max && 0 <= y && y < y_max
            uint32x4_t mask = vandq_u32(vandq_u32(vcleq_f32(zero, x), vcltq_f32(x, x_max)), 
                                        vandq_u32(vcleq_f32(zero, y), vcltq_f32(y, y_max)));

            // Each time, calculate 4-nearest points in same row of dst
            uint32_t p0 = vgetq_lane_u32(mask, 0);
            uint32_t p1 = vgetq_lane_u32(mask, 1);
            uint32_t p2 = vgetq_lane_u32(mask, 2);
            uint32_t p3 = vgetq_lane_u32(mask, 3);
            
            int32x4_t src_0 = vaddq_s32(vmulq_s32(s_step, y0), x0*4);
            int32x4_t src_1 = vaddq_s32(vmulq_s32(s_step, y1), x0*4);

            int32_t   dst_base = i*dst_step+j*4;
            int32x4_t dst_esize = vdupq_n_s32(4);
            int32x4_t dst_p0, dst_p1, dst_p2, dst_p3;
            dst_p0 = vsetq_lane_s32(dst_base,   dst_p0, 0);
            dst_p0 = vsetq_lane_s32(dst_base+1, dst_p0, 1);
            dst_p0 = vsetq_lane_s32(dst_base+2, dst_p0, 2);
            dst_p0 = vsetq_lane_s32(dst_base+3, dst_p0, 3);
            dst_p1 = vaddq_s32(dst_p0, dst_esize);
            dst_p2 = vaddq_s32(dst_p1, dst_esize);
            dst_p3 = vaddq_s32(dst_p2, dst_esize);
            
            bilinear_interpolate(0);
            bilinear_interpolate(1);
            bilinear_interpolate(2);
            bilinear_interpolate(3);
        }     
    }

}



static int read_file(const char* path, void* buffer, int len) {
    FILE * fp = fopen(path, "rb");
    int ret = -1;
    if (fp != nullptr) {
        ret = fread(buffer, 1, len, fp);
        fclose(fp);
    } else {
        printf("can't open %s for readint\n", path);
    }
    return ret;
}  

static int write_file(const char* path, void* buffer, int len) {
    FILE * fp = fopen(path, "wb");
    int ret = -1;
    if (fp != nullptr) {
        ret = fwrite(buffer, 1, len, fp);
        fclose(fp);
    } else {
        printf("can't open %s for writting\n", path);
    }
    return ret;
}


int hud_undistort_main(int argc, char** argv) {

    if (argc < 5) {
        printf("usage <in-raw-image> <out-raw-image> <width> <height> \n");
        return -1;
    }
    
    const char* infile = argv[1];
    const char* outfile = argv[2];
    int w = atoi(argv[3]);
    int h = atoi(argv[4]);
    
    unsigned char* inbuf = new unsigned char[w*h*4];
    unsigned char* outbuf = new unsigned char[w*h*4];

    float* mapx = new float[w*h];
    float* mapy = new float[w*h];
    for (int y=0; y < h; ++y)
        for (int x=0; x < w; ++x)
            mapx[y*w+x] = w-x-1, mapy[y*w+x]=h-y-1;

    read_file(infile, inbuf, w*h*4);

    if (argc == 5) {
        Timer t;
        neno_bilinear_remap(
            inbuf, h, w, 4*w, 
            outbuf, h, w, 4*w,
            mapx, h, w, 4*w,
            mapy, h, w, 4*w);
        printf("using neno_bilinear_remap: %dms\n", t.duration());

    } else {
        Timer t;
        raw_bilinear_remap(
            inbuf, h, w, 4*w, 
            outbuf, h, w, 4*w,
            mapx, h, w, 4*w,
            mapy, h, w, 4*w);
        printf("using raw_bilinear_remap: %dms\n", t.duration());
    }


    write_file(outfile, outbuf, w*h*4);

    delete[] inbuf;
    delete[] outbuf;
    return 0;
}
    

