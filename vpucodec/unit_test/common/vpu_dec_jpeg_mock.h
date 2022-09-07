#include "vpu_dec_jpeg.h"


JPEGDec* vpu_dec_jpeg_open_return = NULL;
JPEGDec* vpu_dec_jpeg_open(const vpu_dec_config* config) {
	return vpu_dec_jpeg_open_return;
}

int vpu_dec_jpeg_close_return = 0;
int vpu_dec_jpeg_close(JPEGDec* jpegDec) {
	return vpu_dec_jpeg_close_return;
}

int vpu_dec_jpeg_start_return = 0;
int vpu_dec_jpeg_start(JPEGDec* jpegDec, const vpu_dec_in* in, vpu_dec_out* out) {
	return vpu_dec_jpeg_start_return;
}

