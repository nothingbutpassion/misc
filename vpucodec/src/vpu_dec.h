#ifndef VPU_DEC_H
#define VPU_DEC_H

extern "C" {
#include <vpu_lib.h>
#include <vpu_io.h>
}
#include "vpu_codec.h"


#define MAX_FB_NUM			16
#define STREAM_BUF_SIZE 	0x200000
#define PS_SAVE_SIZE		0x080000
#define STREAM_END_SIZE		0



struct VPUDec {
	vpu_dec_config config;					// Common config
	DecHandle handle;						// Decoder handle
	
	int totalfb;							// Total number of framebuffers allocated
	FrameBuffer  fb[MAX_FB_NUM];			// Frame buffer base given to decoder
	vpu_mem_desc fbmem[MAX_FB_NUM]; 		// Frame buffer memory desc
	
	vpu_mem_desc bitstream; 				// Bitstream buffer

	vpu_mem_desc ps;						// PS (SPS/PPS) save buffer, must be 8 byte-aligned. Only for H.264.

	int width;								// Picture Width
	int height;								// Picture Height
};


int vpu_dec_check(const vpu_dec_config* config, const vpu_dec_in* in, vpu_dec_out* out);

int vpu_dec_ps_alloc(VPUDec* dec);
void vpu_dec_ps_free(VPUDec* dec);

int vpu_dec_bs_alloc(VPUDec* dec);
void vpu_dec_bs_free(VPUDec* dec);
int vpu_dec_bs_fill(VPUDec* dec, const vpu_dec_in* dec_in);

int vpu_dec_fb_alloc(VPUDec* dec);
void vpu_dec_fb_free(VPUDec* dec);

int vpu_dec_start_frame(VPUDec* dec);
int vpu_dec_get_output(VPUDec* dec, vpu_dec_out* dec_out);


#endif // VPU_DEC_H

