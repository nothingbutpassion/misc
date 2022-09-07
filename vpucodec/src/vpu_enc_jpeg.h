#ifndef VPU_ENC_JPEG_H
#define VPU_ENC_JPEC_H

#include "vpu_codec.h"
#include "vpu_enc.h"

struct JPEGEnc: public VPUEnc {	};

JPEGEnc* vpu_enc_jpeg_open(const vpu_enc_config* config);

int vpu_enc_jpeg_close(JPEGEnc* jpegEnc);

int vpu_enc_jpeg_start(JPEGEnc* jpegEnc, const vpu_enc_in* in, vpu_enc_out* out);


#endif // VPU_ENC_JPEC_H