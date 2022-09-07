#include "vpu_enc.h"

int vpu_enc_check_return = 0;
int vpu_enc_check(const vpu_enc_config* config, const vpu_enc_in* in, vpu_enc_out* out) {
	return vpu_enc_check_return;
}

int vpu_enc_fb_alloc_return = 0;
int vpu_enc_fb_alloc(VPUEnc* enc) {
	return vpu_enc_fb_alloc_return;
}

void vpu_enc_fb_free(VPUEnc* enc) {}

int vpu_enc_fb_fill_return = 0;
int vpu_enc_fb_fill(VPUEnc* enc, const vpu_enc_in* enc_in) {
	return vpu_enc_fb_fill_return;
}

int vpu_enc_bs_alloc_return = 0;
int vpu_enc_bs_alloc(VPUEnc* enc) {
	return vpu_enc_bs_alloc_return;
}

void vpu_enc_bs_free(VPUEnc* enc) {}

int vpu_enc_fill_header_return = 0; 
int vpu_enc_fill_header(VPUEnc* enc, vpu_enc_out* enc_out, int* filled) {
	return vpu_enc_fill_header_return;
}

int vpu_enc_fill_body_return = 0;
int vpu_enc_fill_body(VPUEnc* enc, vpu_enc_out* enc_out, int offset) {
	return vpu_enc_fill_body_return;
}

int vpu_enc_start_frame_return = 0;
int vpu_enc_start_frame(VPUEnc* enc) {
	return vpu_enc_start_frame_return;
}

