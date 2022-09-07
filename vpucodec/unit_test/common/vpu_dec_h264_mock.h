#include "vpu_dec_h264.h"

H264Dec* vpu_dec_h264_open_return = NULL;
H264Dec* vpu_dec_h264_open(const vpu_dec_config* config) {
	return vpu_dec_h264_open_return;
}

int vpu_dec_h264_close_return = 0;
int vpu_dec_h264_close(H264Dec* h264Dec) {
	return vpu_dec_h264_close_return;
}

int vpu_dec_h264_start_return = 0;
int vpu_dec_h264_start(H264Dec* h264Dec, const vpu_dec_in* in, vpu_dec_out* out) {
	return vpu_dec_h264_start_return;
}

