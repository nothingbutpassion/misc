#include "vpu_dec.h"

int vpu_dec_check_return = 0;
int vpu_dec_check(const vpu_dec_config* config, const vpu_dec_in* in, vpu_dec_out* out) {
	return vpu_dec_check_return;
}

int vpu_dec_ps_alloc_return = 0;
int vpu_dec_ps_alloc(VPUDec* dec) {
	return vpu_dec_ps_alloc_return;
}

void vpu_dec_ps_free(VPUDec* dec) {}

int vpu_dec_bs_alloc_return = 0;
int vpu_dec_bs_alloc(VPUDec* dec) {
	return vpu_dec_bs_alloc_return;
}

void vpu_dec_bs_free(VPUDec* dec) {}

int vpu_dec_bs_fill_return = 0;
int vpu_dec_bs_fill(VPUDec* dec, const vpu_dec_in* dec_in) {
	return vpu_dec_bs_fill_return;
}

int vpu_dec_fb_alloc_return = 0;
int vpu_dec_fb_alloc(VPUDec* dec) {
	return vpu_dec_fb_alloc_return;
}
void vpu_dec_fb_free(VPUDec* dec) {}

int vpu_dec_start_frame_return = 0;
int vpu_dec_start_frame(VPUDec* dec) {
	return vpu_dec_start_frame_return;
}

int vpu_dec_get_output_return = 0;
int vpu_dec_get_output(VPUDec* dec, vpu_dec_out* dec_out) {
	return vpu_dec_get_output_return;
}

