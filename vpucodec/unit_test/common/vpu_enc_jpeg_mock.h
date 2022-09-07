#include "vpu_enc_jpeg.h"

JPEGEnc* vpu_enc_jpeg_open_return = NULL;
JPEGEnc* vpu_enc_jpeg_open(const vpu_enc_config* config) {
	return vpu_enc_jpeg_open_return;
}

int vpu_enc_jpeg_close_return = 0;
int vpu_enc_jpeg_close(JPEGEnc* jpegEnc) {
	return vpu_enc_jpeg_close_return;
}

int vpu_enc_jpeg_start_return = 0;
int vpu_enc_jpeg_start(JPEGEnc* jpegEnc, const vpu_enc_in* in, vpu_enc_out* out) {
	return vpu_enc_jpeg_start_return;
}

