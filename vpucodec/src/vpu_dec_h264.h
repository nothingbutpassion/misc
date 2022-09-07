#ifndef VPU_DEC_H264_H
#define VPU_DEC_H264_H

#include "vpu_codec.h"
#include "vpu_dec.h"

struct H264Dec: public VPUDec {	};

H264Dec* vpu_dec_h264_open(const vpu_dec_config* config);

int vpu_dec_h264_close(H264Dec* h264Dec);

int vpu_dec_h264_start(H264Dec* h264Dec, const vpu_dec_in* in, vpu_dec_out* out);



#endif // VPU_DEC_H264_H

