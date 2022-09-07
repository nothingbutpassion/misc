#include <gtest/gtest.h>

#include "vpu_enc_jpeg.h"
#include "vpu_dec_jpeg.h"

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
// JPEGDec* vpu_dec_jpeg_open(const vpu_dec_config* config);
//
TEST(vpu_dec_jpeg_open, callocFailed) { 
    calloc_return = NULL;
    vpu_dec_config config;
    EXPECT_EQ(NULL, vpu_dec_jpeg_open(&config));
}
TEST(vpu_dec_jpeg_open, BsAllocFailed) { 
    JPEGDec dec;
    calloc_return = &dec;
    vpu_dec_bs_alloc_return = -1;
    vpu_dec_config config;
    EXPECT_EQ(NULL, vpu_dec_jpeg_open(&config));
}
TEST(vpu_dec_jpeg_open, vpu_DecOpenFailed) {
    JPEGDec dec;
    calloc_return = &dec;
    vpu_dec_bs_alloc_return = 0;
    vpu_DecOpen_return = RETCODE_FAILURE;
    vpu_dec_config config;
    EXPECT_EQ(NULL, vpu_dec_jpeg_open(&config));
}
TEST(vpu_dec_jpeg_open, vpu_DecOpenSucceed) {
    JPEGDec dec;
    calloc_return = &dec;
    vpu_dec_bs_alloc_return = 0;
    vpu_DecOpen_return = RETCODE_SUCCESS;
    vpu_dec_config config;
    EXPECT_TRUE(NULL != vpu_dec_jpeg_open(&config));
}


//
// int vpu_dec_jpeg_close(JPEGDec* jpegDec)
//
TEST(vpu_dec_jpeg_close, vpu_DecCloseFailed) {
    vpu_DecClose_return = RETCODE_FRAME_NOT_COMPLETE;
    JPEGDec dec;
    EXPECT_EQ(-1, vpu_dec_jpeg_close(&dec));
}
TEST(vpu_dec_jpeg_close, vpu_DecCloseSucceed) {
    vpu_DecClose_return = RETCODE_SUCCESS;
    JPEGDec dec;
    EXPECT_EQ(0, vpu_dec_jpeg_close(&dec));
}


//
// int vpu_dec_jpeg_start(JPEGDec* jpegDec, const vpu_dec_in* in, vpu_dec_out* out)
//
TEST(vpu_dec_jpeg_start, BSFillFailed) {
    vpu_dec_bs_fill_return = -1;
    JPEGDec dec;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_jpeg_start(&dec, &in, &out));
}
TEST(vpu_dec_jpeg_close, FBAllocFailed) {
    vpu_dec_bs_fill_return = 0;
    vpu_dec_fb_alloc_return = -1;
    JPEGDec dec;
    dec.totalfb = 0;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_jpeg_start(&dec, &in, &out));
}
TEST(vpu_dec_jpeg_close, StartFrameFailed) {
    vpu_dec_bs_fill_return = 0;
    vpu_dec_fb_alloc_return = 0;
    vpu_dec_start_frame_return = -1;
    JPEGDec dec;
    dec.totalfb = 0;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_jpeg_start(&dec, &in, &out));
}
TEST(vpu_dec_jpeg_close, GetOutputFailed) {
    vpu_dec_bs_fill_return = 0;
    vpu_dec_fb_alloc_return = 0;
    vpu_dec_start_frame_return = 0;
    vpu_dec_get_output_return = -1;
    JPEGDec dec;
    dec.totalfb = 0;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_jpeg_start(&dec, &in, &out));
}
TEST(vpu_dec_jpeg_close, GetOutputSucceed) {
    vpu_dec_bs_fill_return = 0;
    vpu_dec_fb_alloc_return = 0;
    vpu_dec_start_frame_return = 0;
    vpu_dec_get_output_return = 0;
    JPEGDec dec;
    dec.totalfb = 0;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(0, vpu_dec_jpeg_start(&dec, &in, &out));
}


//
// JPEGEnc* vpu_enc_jpeg_open(const vpu_enc_config* config)
//
TEST(vpu_enc_jpeg_open, callocFailed) { 
    calloc_return = NULL;
    vpu_enc_config config;
    EXPECT_EQ(NULL, vpu_enc_jpeg_open(&config));
}
TEST(vpu_enc_jpeg_open, BsAllocFailed) { 
    JPEGEnc enc;
    calloc_return = &enc;
    vpu_enc_bs_alloc_return = -1;
    vpu_enc_config config;
    EXPECT_EQ(NULL, vpu_enc_jpeg_open(&config));
}
TEST(vpu_enc_jpeg_open, vpu_EncOpenFailed) { 
    JPEGEnc enc;
    calloc_return = &enc;
    vpu_enc_bs_alloc_return = 0;
    vpu_EncOpen_return = RETCODE_FAILURE;
    vpu_enc_config config;
    config.pixfmt = VPU_YUV420P;
    EXPECT_EQ(NULL, vpu_enc_jpeg_open(&config));
}
TEST(vpu_enc_jpeg_open, vpu_EncOpenSucceed) { 
    JPEGEnc enc;
    calloc_return = &enc;
    vpu_enc_bs_alloc_return = 0;
    vpu_EncOpen_return = RETCODE_SUCCESS;
    vpu_enc_config config;
    config.pixfmt = VPU_YUV420P;
    EXPECT_TRUE(NULL != vpu_enc_jpeg_open(&config));
}

//
// int vpu_enc_jpeg_close(JPEGEnc* jpegEnc)
//
TEST(vpu_enc_jpeg_close, vpu_EncCloseFailed) { 
    vpu_EncClose_return =RETCODE_FRAME_NOT_COMPLETE;
    JPEGEnc enc;
    EXPECT_EQ(-1, vpu_enc_jpeg_close(&enc));
}
TEST(vpu_enc_jpeg_close, vpu_EncCloseSucceed) { 
    vpu_EncClose_return =RETCODE_SUCCESS;
    JPEGEnc enc;
    EXPECT_EQ(0, vpu_enc_jpeg_close(&enc));
}

//
// int vpu_enc_jpeg_start(JPEGEnc* jpegEnc, const vpu_enc_in* in, vpu_enc_out* out)
//
TEST(vpu_enc_jpeg_start, FBAllocFailed) { 
    vpu_enc_fb_alloc_return = -1;
    JPEGEnc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_jpeg_start(&enc, &in, &out));
}
TEST(vpu_enc_jpeg_start, FBFillFailed) { 
    vpu_enc_fb_alloc_return = 0;
    vpu_enc_fb_fill_return = -1;
    JPEGEnc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_jpeg_start(&enc, &in, &out));
}
TEST(vpu_enc_jpeg_start, FillHeaderFailed) { 
    vpu_enc_fb_alloc_return = 0;
    vpu_enc_fb_fill_return = 0;
    vpu_enc_fill_header_return = -1;
    JPEGEnc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_jpeg_start(&enc, &in, &out));
}
TEST(vpu_enc_jpeg_start, StartFrameFailed) { 
    vpu_enc_fb_alloc_return = 0;
    vpu_enc_fb_fill_return = 0;
    vpu_enc_fill_header_return = 0;
    vpu_enc_start_frame_return = -1;
    JPEGEnc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_jpeg_start(&enc, &in, &out));
}
TEST(vpu_enc_jpeg_start, FillBodyFailed) { 
    vpu_enc_fb_alloc_return = 0;
    vpu_enc_fb_fill_return = 0;
    vpu_enc_fill_header_return = 0;
    vpu_enc_start_frame_return = 0;
    vpu_enc_fill_body_return = -1;
    JPEGEnc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_jpeg_start(&enc, &in, &out));
}
TEST(vpu_enc_jpeg_start, FillBodySucceed) { 
    vpu_enc_fb_alloc_return = 0;
    vpu_enc_fb_fill_return = 0;
    vpu_enc_fill_header_return = 0;
    vpu_enc_start_frame_return = 0;
    vpu_enc_fill_body_return = 0;
    JPEGEnc enc;
    enc.totalfb = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(0, vpu_enc_jpeg_start(&enc, &in, &out));
}