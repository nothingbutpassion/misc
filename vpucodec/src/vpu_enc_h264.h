#ifndef VPU_ENC_H264_H
#define VPU_ENC_H264_H

#include "vpu_codec.h"
#include "vpu_enc.h"

struct H264Enc: public VPUEnc {	};

H264Enc* vpu_enc_h264_open(const vpu_enc_config* config);

int vpu_enc_h264_close(H264Enc* jpegEnc);

int vpu_enc_h264_start(H264Enc* jpegEnc, const vpu_enc_in* in, vpu_enc_out* out);


#endif // VPU_ENC_H264_H

