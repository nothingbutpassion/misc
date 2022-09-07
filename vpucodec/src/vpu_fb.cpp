#include <string.h>
#include "vpu_fb.h"
#include "vpu_utils.h"


#define TAG "VPUFB"


int vpu_fb_alloc(vpu_mem_desc* mem, int format, int stride, int height, int mvCol, FrameBuffer* fb) {
    // Make sure init vpu_mem_desc with 0
	memset(mem, 0, sizeof(vpu_mem_desc));
 
    // Calculate memory size
    // NOTE: format is always VPU_YUV420P which is MODE420     
    int divX = (format == MODE420 || format == MODE422) ? 2 : 1;
	int divY = (format == MODE420 || format == MODE224) ? 2 : 1;
	mem->size = (stride * height  + stride / divX * height / divY * 2);
	if (mvCol) {
        mem->size += stride / divX * height / divY;
    }

    // Get physical memory
	int err = IOGetPhyMem(mem);
	if (err) {
        LOGE(TAG, "IOGetPhyMem failed: %d", err);
		memset(mem, 0, sizeof(vpu_mem_desc));
		return -1;
	}
    // Map virtaul memory
	int virtaddr = IOGetVirtMem(mem);
	if (virtaddr <= 0) {
        LOGE(TAG, "IOGetVirtMem failed: %d", virtaddr);
		IOFreePhyMem(mem);
		memset(mem, 0, sizeof(vpu_mem_desc));
		return -1;
	}

    // Fill framebuffer
    fb->bufY = mem->phy_addr;
    fb->bufCb = fb->bufY + stride * height;
    fb->bufCr = fb->bufCb + stride / divX * height / divY;
    fb->strideY = stride;
    fb->strideC = stride / divY;
    if (mvCol) {
        fb->bufMvCol = fb->bufCr + stride / divX * height / divY;
    }
	return 0;
}


void vpu_fb_free(vpu_mem_desc* mem, FrameBuffer* fb) {
    // Release physical and virtual memory allocated
    if (mem->virt_uaddr) {
        IOFreeVirtMem(mem);
    }
    if (mem->phy_addr) {
        IOFreePhyMem(mem);
    }

    // Reset the struct with 0
    memset(mem, 0, sizeof(vpu_mem_desc));
    memset(fb, 0, sizeof(FrameBuffer));
}


