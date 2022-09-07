#include <gtest/gtest.h>

#include "vpu_enc_h264.h"
#include "vpu_dec_h264.h"

#include "vpu_lib_mock.h"
#include "vpu_utils_mock.h"
#include "vpu_enc_mock.h"
#include "vpu_dec_mock.h"

//
// Mock calloc() and free()
//
void* calloc_return = NULL;
void* calloc(size_t nmemb, size_t size) {
    return calloc_return;
}
void free(void *ptr) {}


//
// JPEGDec* vpu_dec_h264_open(const vpu_dec_config* config);
//
TEST(vpu_dec_h264_open, callocFailed) { 
    calloc_return = NULL;
    vpu_dec_config config;
    EXPECT_EQ(NULL, vpu_dec_h264_open(&config));
}
TEST(vpu_dec_h264_open, BsAllocFailed) { 
    H264Dec dec;
    calloc_return = &dec;
    vpu_dec_bs_alloc_return = -1;
    vpu_dec_config config;
    EXPECT_EQ(NULL, vpu_dec_h264_open(&config));
}
TEST(vpu_dec_h264_open, PSAllocFailed) { 
    H264Dec dec;
    calloc_return = &dec;
    vpu_dec_bs_alloc_return = 0;
    vpu_dec_ps_alloc_return = -1;
    vpu_dec_config config;
    EXPECT_EQ(NULL, vpu_dec_h264_open(&config));
}
TEST(vpu_dec_h264_open, vpu_DecOpenFailed) {
    H264Dec dec;
    calloc_return = &dec;
    vpu_dec_bs_alloc_return = 0;
    vpu_dec_ps_alloc_return = 0;
    vpu_DecOpen_return = RETCODE_FAILURE;
    vpu_dec_config config;
    EXPECT_EQ(NULL, vpu_dec_h264_open(&config));
}
TEST(vpu_dec_h264_open, vpu_DecOpenSucceed) {
    H264Dec dec;
    calloc_return = &dec;
    vpu_dec_bs_alloc_return = 0;
     vpu_dec_ps_alloc_return = 0;
    vpu_DecOpen_return = RETCODE_SUCCESS;
    vpu_dec_config config;
    EXPECT_TRUE(NULL != vpu_dec_h264_open(&config));
}


//
// int vpu_dec_h264_close(H264Dec* h264Dec)
//
TEST(vpu_dec_h264_close, vpu_DecCloseFailed) {
    vpu_DecClose_return = RETCODE_FRAME_NOT_COMPLETE;
    H264Dec dec;
    EXPECT_EQ(-1, vpu_dec_h264_close(&dec));
}
TEST(vpu_dec_h264_close, vpu_DecCloseSucceed) {
    vpu_DecClose_return = RETCODE_SUCCESS;
    H264Dec dec;
    EXPECT_EQ(0, vpu_dec_h264_close(&dec));
}


//
// int vpu_dec_h264_start(H264Dec* h264Dec, const vpu_dec_in* in, vpu_dec_out* out)
//
TEST(vpu_dec_h264_start, BSFillFailed) {
    vpu_dec_bs_fill_return = -1;
    H264Dec dec;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_h264_start(&dec, &in, &out));
}
TEST(vpu_dec_h264_close, FBAllocFailed) {
    vpu_dec_bs_fill_return = 0;
    vpu_dec_fb_alloc_return = -1;
    H264Dec dec;
    dec.totalfb = 0;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_h264_start(&dec, &in, &out));
}
TEST(vpu_dec_h264_close, StartFrameFailed) {
    vpu_dec_bs_fill_return = 0;
    vpu_dec_fb_alloc_return = 0;
    vpu_dec_start_frame_return = -1;
    H264Dec dec;
    dec.totalfb = 0;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_h264_start(&dec, &in, &out));
}
TEST(vpu_dec_h264_close, GetOutputFailed) {
    vpu_dec_bs_fill_return = 0;
    vpu_dec_fb_alloc_return = 0;
    vpu_dec_start_frame_return = 0;
    vpu_dec_get_output_return = -1;
    H264Dec dec;
    dec.totalfb = 0;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_h264_start(&dec, &in, &out));
}
TEST(vpu_dec_h264_close, GetOutputSucceed) {
    vpu_dec_bs_fill_return = 0;
    vpu_dec_fb_alloc_return = 0;
    vpu_dec_start_frame_return = 0;
    vpu_dec_get_output_return = 0;
    H264Dec dec;
    dec.totalfb = 0;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(0, vpu_dec_h264_start(&dec, &in, &out));
}


//
// H264Enc* vpu_enc_h264_open(const vpu_enc_config* config)
//
TEST(vpu_enc_h264_open, callocFailed) { 
    calloc_return = NULL;
    vpu_enc_config config;
    EXPECT_EQ(NULL, vpu_enc_h264_open(&config));
}
TEST(vpu_enc_h264_open, BsAllocFailed) { 
    H264Enc enc;
    calloc_return = &enc;
    vpu_enc_bs_alloc_return = -1;
    vpu_enc_config config;
    EXPECT_EQ(NULL, vpu_enc_h264_open(&config));
}
TEST(vpu_enc_h264_open, vpu_EncOpenFailed) { 
    H264Enc enc;
    calloc_return = &enc;
    vpu_enc_bs_alloc_return = 0;
    vpu_EncOpen_return = RETCODE_FAILURE;
    vpu_enc_config config;
    config.height = 1080;
    EXPECT_EQ(NULL, vpu_enc_h264_open(&config));
}
TEST(vpu_enc_h264_open, vpu_EncOpenSucceed) { 
    H264Enc enc;
    calloc_return = &enc;
    vpu_enc_bs_alloc_return = 0;
    vpu_EncOpen_return = RETCODE_SUCCESS;
    vpu_enc_config config;
    config.height = 1080;
    EXPECT_TRUE(NULL != vpu_enc_h264_open(&config));
}


//
// int vpu_enc_h264_close(H264Enc* h264Enc)
//
TEST(vpu_enc_h264_close, vpu_EncCloseFailed) { 
    vpu_EncClose_return =RETCODE_FRAME_NOT_COMPLETE;
    H264Enc enc;
    EXPECT_EQ(-1, vpu_enc_h264_close(&enc));
}
TEST(vpu_enc_h264_close, vpu_EncCloseSucceed) { 
    vpu_EncClose_return =RETCODE_SUCCESS;
    H264Enc enc;
    EXPECT_EQ(0, vpu_enc_h264_close(&enc));
}

//
// int vpu_enc_h264_start(H264Enc* h264Enc, const vpu_enc_in* in, vpu_enc_out* out)
//
TEST(vpu_enc_h264_start, FBAllocFailed) { 
    vpu_enc_fb_alloc_return = -1;
    H264Enc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_h264_start(&enc, &in, &out));
}
TEST(vpu_enc_h264_start, FBFillFailed) { 
    vpu_enc_fb_alloc_return = 0;
    vpu_enc_fb_fill_return = -1;
    H264Enc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_h264_start(&enc, &in, &out));
}

TEST(vpu_enc_h264_start, StartFrameFailed) { 
    vpu_enc_fb_alloc_return = 0;
    vpu_enc_fb_fill_return = 0;
    vpu_enc_start_frame_return = -1;
    H264Enc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_h264_start(&enc, &in, &out));
}
TEST(vpu_enc_h264_start, FillBodyFailed) { 
    vpu_enc_fb_alloc_return = 0;
    vpu_enc_fb_fill_return = 0;
    vpu_enc_start_frame_return = 0;
    vpu_enc_fill_body_return = -1;
    H264Enc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_h264_start(&enc, &in, &out));
}
TEST(vpu_enc_h264_start, FillBodySucceed) { 
    vpu_enc_fb_alloc_return = 0;
    vpu_enc_fb_fill_return = 0;
    vpu_enc_start_frame_return = 0;
    vpu_enc_fill_body_return = 0;
    H264Enc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(0, vpu_enc_h264_start(&enc, &in, &out));
}