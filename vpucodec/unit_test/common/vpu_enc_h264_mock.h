#include "vpu_enc_h264.h"

H264Enc* vpu_enc_h264_open_return = NULL;
H264Enc* vpu_enc_h264_open(const vpu_enc_config* config) {
	return vpu_enc_h264_open_return;
}

int vpu_enc_h264_close_return = 0;
int vpu_enc_h264_close(H264Enc* jpegEnc) {
	return vpu_enc_h264_close_return;
}

int vpu_enc_h264_start_return = 0;
int vpu_enc_h264_start(H264Enc* jpegEnc, const vpu_enc_in* in, vpu_enc_out* out) {
	return vpu_enc_h264_start_return;
}

