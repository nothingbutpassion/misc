#include "vpu_fb.h"

int vpu_fb_alloc_return;
int vpu_fb_alloc(vpu_mem_desc* mem, int format, int strideY, int height, int mvCol, FrameBuffer* fb) {
	return vpu_fb_alloc_return;
}

void vpu_fb_free(vpu_mem_desc* mem, FrameBuffer* fb) {}

