#ifndef VPU_ENC_H
#define VPU_ENC_H

extern "C" {
#include <vpu_lib.h>
#include <vpu_io.h>
}
#include "vpu_codec.h"


#define MAX_FB_NUM		16
#define STREAM_BUF_SIZE 0x200000


struct VPUEnc {
	vpu_enc_config config;					// Common config
	
	EncHandle handle;						// Encoder handle

	int totalfb;							// Total number of framebuffers allocated
	int srcfb;								// Index of frame buffer that contains YUV image
	FrameBuffer  fb[MAX_FB_NUM]; 			// Frame buffer base given to encoder
	vpu_mem_desc fbmem[MAX_FB_NUM];			// Frame buffer memory desc
	
	vpu_mem_desc bitstream;					// Bitstream buffer
};


int vpu_enc_check(const vpu_enc_config* config, const vpu_enc_in* in, vpu_enc_out* out);

int vpu_enc_fb_alloc(VPUEnc* enc);
void vpu_enc_fb_free(VPUEnc* enc);
int vpu_enc_fb_fill(VPUEnc* enc, const vpu_enc_in* enc_in);

int vpu_enc_bs_alloc(VPUEnc* enc);
void vpu_enc_bs_free(VPUEnc* enc);

int vpu_enc_fill_header(VPUEnc* enc, vpu_enc_out* enc_out, int* filled);
int vpu_enc_fill_body(VPUEnc* enc, vpu_enc_out* enc_out, int offset);
int vpu_enc_start_frame(VPUEnc* enc);


#endif // VPU_ENC_H