#ifndef VPU_DEC_JPEG_H
#define VPU_DEC_JPEC_H

#include "vpu_codec.h"
#include "vpu_dec.h"

struct JPEGDec: public VPUDec {	};

JPEGDec* vpu_dec_jpeg_open(const vpu_dec_config* config);

int vpu_dec_jpeg_close(JPEGDec* jpegDec);

int vpu_dec_jpeg_start(JPEGDec* jpegDec, const vpu_dec_in* in, vpu_dec_out* out);


#endif // VPU_DEC_JPEC_H

