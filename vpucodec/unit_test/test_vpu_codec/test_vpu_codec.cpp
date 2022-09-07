#include <gtest/gtest.h>

#include "vpu_codec.h"
#include "vpu_lib_mock.h"
#include "vpu_utils_mock.h"
#include "vpu_enc_mock.h"
#include "vpu_enc_jpeg_mock.h"
#include "vpu_enc_h264_mock.h"
#include "vpu_dec_mock.h"
#include "vpu_dec_jpeg_mock.h"
#include "vpu_dec_h264_mock.h"

//using namespace std;


//
// vpu_error()
//
TEST(vpu_error, CheckReturn) { 
    EXPECT_EQ(vpu_get_error_return, vpu_error());
}


//
// int vpu_init()
//
TEST(vpu_init, Succeed) {
    vpu_Init_return = RETCODE_SUCCESS;
    EXPECT_EQ(0, vpu_init());
}
TEST(vpu_init, Failed) {
    vpu_Init_return = RETCODE_FAILURE;
    EXPECT_EQ(-1, vpu_init());
}


//
// int vpu_denit()
//
TEST(vpu_deinit, Succeed) {
    EXPECT_EQ(0, vpu_deinit());
}


//
// void* vpu_enc_open(const vpu_enc_config* enc_config)
//
TEST(vpu_enc_open, NullConfig) {
    EXPECT_EQ(NULL, vpu_enc_open(NULL));
}
TEST(vpu_enc_open, InvalidConfig) {
    vpu_enc_check_return = -1;
    vpu_enc_config config;
    EXPECT_EQ(NULL, vpu_enc_open(&config));
}
TEST(vpu_enc_open, InvalidConfigEncfmt) {
    vpu_enc_check_return = 0;
    vpu_enc_config config;
    config.encfmt = VPU_FMT_VP8;
    EXPECT_EQ(NULL, vpu_enc_open(&config));
}
TEST(vpu_enc_open, OpenJPEGEncoderFailed) {
    vpu_enc_check_return = 0;
    vpu_enc_jpeg_open_return = NULL;
    vpu_enc_config config;
    config.encfmt = VPU_FMT_JPEG;
    EXPECT_EQ(NULL, vpu_enc_open(&config));
}
TEST(vpu_enc_open, OpenJPEGEncoderSucceed) {
    vpu_enc_check_return = 0;
    JPEGEnc jpegEnc;
    vpu_enc_jpeg_open_return = &jpegEnc;
    vpu_enc_config config;
    config.encfmt = VPU_FMT_JPEG;
    EXPECT_TRUE(NULL != vpu_enc_open(&config));
}
TEST(vpu_enc_open, OpenH264EncoderFailed) {
    vpu_enc_check_return = 0;
    vpu_enc_h264_open_return = NULL;
    vpu_enc_config config;
    config.encfmt = VPU_FMT_H264;
    EXPECT_EQ(NULL, vpu_enc_open(&config));
}
TEST(vpu_enc_open, OpenH264EncoderSucceed) {
    vpu_enc_check_return = 0;
    H264Enc enc;
    vpu_enc_h264_open_return = &enc;
    vpu_enc_config config;
    config.encfmt = VPU_FMT_H264;
    EXPECT_TRUE(NULL != vpu_enc_open(&config));
}


//
// int vpu_enc_close(void* enc_handle)
//
TEST(vpu_enc_close, NullHandle) {
    EXPECT_EQ(-1, vpu_enc_close(NULL));
}
TEST(vpu_enc_close, CloseJPEGEncoderFailed) {
    vpu_enc_jpeg_close_return = -1;
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_JPEG;
    EXPECT_EQ(-1, vpu_enc_close(&enc));
}
TEST(vpu_enc_close, CloseJPEGEncoderSucceed) {
    vpu_enc_jpeg_close_return = 0;
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_JPEG;
    EXPECT_EQ(0, vpu_enc_close(&enc));
}
TEST(vpu_enc_close, CloseH264Encoder) {
    vpu_enc_h264_close_return = 0;
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_H264;
    EXPECT_EQ(0, vpu_enc_close(&enc));
}

//
// int vpu_encode(void* enc_handle, const vpu_enc_in* enc_in, vpu_enc_out* enc_out)
//
TEST(vpu_encode, NullArgs) {
    EXPECT_EQ(-1, vpu_encode(NULL, NULL, NULL));
}
TEST(vpu_encode, EncoderIOError) {
    vpu_enc_check_return = -1;
    VPUEnc enc;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_encode(&enc, &in, &out));
}
TEST(vpu_encode, JPEGEncodingFailed) {
    vpu_enc_check_return = 0;
    vpu_enc_jpeg_start_return = -1;
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_JPEG;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_encode(&enc, &in, &out));
}
TEST(vpu_encode, JPEGEncodingSucceed) {
    vpu_enc_check_return = 0;
    vpu_enc_jpeg_start_return = 0;
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_JPEG;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(0, vpu_encode(&enc, &in, &out));
}
TEST(vpu_encode, H264Encoding) {
    vpu_enc_check_return = 0;
    vpu_enc_jpeg_start_return = 0;
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_H264;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(0, vpu_encode(&enc, &in, &out));
}

//
// void* vpu_dec_open(const vpu_dec_config* dec_config) 
//
TEST(vpu_dec_open, NullConfig) {
    EXPECT_EQ(NULL, vpu_dec_open(NULL));
}
TEST(vpu_dec_open, InvalidConfig) {
    vpu_dec_check_return = -1;
    vpu_dec_config config;
    EXPECT_EQ(NULL, vpu_dec_open(&config));
}
TEST(vpu_dec_open, InvalidConfigDecfmt) {
    vpu_dec_check_return = 0;
    vpu_dec_config config;
    config.decfmt = VPU_FMT_VP8;
    EXPECT_EQ(NULL, vpu_dec_open(&config));
}
TEST(vpu_dec_open, OpenJPEGDeocderFailed) {
    vpu_dec_check_return = 0;
    vpu_dec_jpeg_open_return = NULL;
    vpu_dec_config config;
    config.decfmt = VPU_FMT_JPEG;
    EXPECT_EQ(NULL, vpu_dec_open(&config));
}
TEST(vpu_dec_open, OpenJPEGDecoderSucceed) {
    vpu_dec_check_return = 0;
    JPEGDec dec;
    vpu_dec_jpeg_open_return = &dec;
    vpu_dec_config config;
    config.decfmt = VPU_FMT_JPEG;
    EXPECT_TRUE(NULL != vpu_dec_open(&config));
}
TEST(vpu_dec_open, OpenH264DecoderFailed) {
    vpu_dec_check_return = 0;
    vpu_dec_h264_open_return = NULL;
    vpu_dec_config config;
    config.decfmt = VPU_FMT_H264;
    EXPECT_EQ(NULL, vpu_dec_open(&config));
}
TEST(vpu_dec_open, OpenH264DecoderSucceed) {
    vpu_dec_check_return = 0;
    H264Dec dec;
    vpu_dec_h264_open_return = &dec;
    vpu_dec_config config;
    config.decfmt = VPU_FMT_H264;
    EXPECT_TRUE(NULL != vpu_dec_open(&config));
}

//
// int vpu_dec_close(void* dec_handle)
//
TEST(vpu_dec_close, NullHandle) {
    EXPECT_EQ(-1, vpu_dec_close(NULL));
}
TEST(vpu_dec_close, CloseJPEGDecoderFailed) {
    vpu_dec_jpeg_close_return = -1;
    VPUDec dec;
    dec.config.decfmt = VPU_FMT_JPEG;
    EXPECT_EQ(-1, vpu_dec_close(&dec));
}
TEST(vpu_dec_close, CloseJPEGDecoderSucceed) {
    vpu_dec_jpeg_close_return = 0;
    VPUDec dec;
    dec.config.decfmt = VPU_FMT_JPEG;
    EXPECT_EQ(0, vpu_dec_close(&dec));
}
TEST(vpu_dec_close, CloseH264Decoder) {
    vpu_dec_h264_close_return = 0;
    VPUDec dec;
    dec.config.decfmt = VPU_FMT_H264;
    EXPECT_EQ(0, vpu_dec_close(&dec));
}

//
//  int vpu_decode(void* dec_handle, const vpu_dec_in* dec_in, vpu_dec_out* dec_out) 
//
TEST(vpu_decode, NullArgs) {
    EXPECT_EQ(-1, vpu_decode(NULL, NULL, NULL));
}
TEST(vpu_decode, DecoderIOError) {
    vpu_dec_check_return = -1;
    VPUDec dec;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_decode(&dec, &in, &out));
}
TEST(vpu_decode, JPEGDecodingFailed) {
    vpu_dec_check_return = 0;
    vpu_dec_jpeg_start_return = -1;
    VPUDec dec;
    dec.config.decfmt = VPU_FMT_JPEG;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_decode(&dec, &in, &out));
}
TEST(vpu_decode, JPEGDecodingSucceed) {
    vpu_dec_check_return = 0;
    vpu_dec_jpeg_start_return = 0;
    VPUDec dec;
    dec.config.decfmt = VPU_FMT_JPEG;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(0, vpu_decode(&dec, &in, &out));
}
TEST(vpu_decode, H264Decoding) {
    vpu_dec_check_return = 0;
    vpu_dec_jpeg_start_return = 0;
    VPUDec dec;
    dec.config.decfmt = VPU_FMT_H264;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(0, vpu_decode(&dec, &in, &out));
}
    