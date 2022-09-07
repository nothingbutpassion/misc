#ifndef VPU_FB_H
#define VPU_FB_H

extern "C" {
#include <vpu_lib.h>
#include <vpu_io.h>
}

enum {
    MODE420 = 0,
    MODE422 = 1,
    MODE224 = 2,
    MODE444 = 3,
    MODE400 = 4
};

int vpu_fb_alloc(vpu_mem_desc* mem, int format, int strideY, int height, int mvCol, FrameBuffer* fb);
void vpu_fb_free(vpu_mem_desc* mem, FrameBuffer* fb);

#endif // VPU_FB_H
