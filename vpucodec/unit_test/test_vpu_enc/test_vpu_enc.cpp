#include <gtest/gtest.h>

#include "vpu_lib_mock.h"
#include "vpu_io_mock.h"
#include "vpu_fb_mock.h"
#include "vpu_utils_mock.h"

#include "vpu_codec.h"
#include "vpu_enc.h"


//
// int vpu_enc_check(const vpu_enc_config* config, const vpu_enc_in* in, vpu_enc_out* out);
//
TEST(vpu_enc_check, InvalidEncoderFmt) {
    vpu_enc_config config;
    config.encfmt = VPU_FMT_VP8;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_check(&config, &in, &out));
}

TEST(vpu_enc_check, InvalidPixelFmt) {
    vpu_enc_config config;
    config.encfmt = VPU_FMT_JPEG;
    config.pixfmt = (vpu_pixel_format)(~VPU_YUV420P);
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_check(&config, &in, &out));
}
TEST(vpu_enc_check, InvalidPictureSize) {
    vpu_enc_config config;
    config.encfmt = VPU_FMT_JPEG;
    config.pixfmt = VPU_YUV420P;
    config.width = 0;
    config.height = 0;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_check(&config, &in, &out));
}
TEST(vpu_enc_check, InvalidH264PictureSize) {
    vpu_enc_config config;
    config.encfmt = VPU_FMT_H264;
    config.pixfmt = VPU_YUV420P;
    config.width = 800;
    config.height = 600;
    vpu_enc_in in;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_check(&config, &in, &out));
}
TEST(vpu_enc_check, InvalidInputBuffer) {
    vpu_enc_config config;
    config.encfmt = VPU_FMT_H264;
    config.pixfmt = VPU_YUV420P;
    config.width = 1280;
    config.height = 800;
    vpu_enc_in in;
    in.buf = NULL;
    in.buflen = 0;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_check(&config, &in, &out));
} 
TEST(vpu_enc_check, InvalidInputBufferLength) {
    vpu_enc_config config;
    config.encfmt = VPU_FMT_H264;
    config.pixfmt = VPU_YUV420P;
    config.width = 1280;
    config.height = 800;
    vpu_enc_in in;
    char dummy;
    in.buf = &dummy;
    in.buflen = 0;
    vpu_enc_out out;
    EXPECT_EQ(-1, vpu_enc_check(&config, &in, &out));
}
TEST(vpu_enc_check, InvalidOutputBuffer) {
    vpu_enc_config config;
    config.encfmt = VPU_FMT_H264;
    config.pixfmt = VPU_YUV420P;
    config.width = 1280;
    config.height = 800;
    vpu_enc_in in;
    char dummy;
    in.buf = &dummy;
    in.buflen = 1280*800*3/2;
    vpu_enc_out out;
    out.buf = NULL;
    EXPECT_EQ(-1, vpu_enc_check(&config, &in, &out));
} 
TEST(vpu_enc_check, InvalidOutputBufferLength) {
    vpu_enc_config config;
    config.encfmt = VPU_FMT_H264;
    config.pixfmt = VPU_YUV420P;
    config.width = 1280;
    config.height = 800;
    vpu_enc_in in;
    char dummy;
    in.buf = &dummy;
    in.buflen = 1280*800*3/2;
    vpu_enc_out out;
    out.buf = &dummy;
    out.buflen = 0;
    EXPECT_EQ(-1, vpu_enc_check(&config, &in, &out));
}
TEST(vpu_enc_check, CorrectArgs) {
    vpu_enc_config config;
    config.encfmt = VPU_FMT_H264;
    config.pixfmt = VPU_YUV420P;
    config.width = 1280;
    config.height = 800;
    vpu_enc_in in;
    char dummy;
    in.buf = &dummy;
    in.buflen = 1280*800*3/2;
    vpu_enc_out out;
    out.buf = &dummy;
    out.buflen = 1;
    EXPECT_EQ(0, vpu_enc_check(&config, &in, &out));
}

//
// int vpu_enc_fb_alloc(VPUEnc* enc) ; 
// void vpu_enc_fb_free(VPUEnc* enc);
//
TEST(vpu_enc_fb_alloc, vpu_EncGetInitialInfoFailed) {
    vpu_EncGetInitialInfo_return = RETCODE_FAILURE;
    VPUEnc enc;
    EXPECT_EQ(-1, vpu_enc_fb_alloc(&enc));
}
TEST(vpu_enc_fb_alloc, FBAllocFailed) {
    vpu_EncGetInitialInfo_return = RETCODE_SUCCESS;
    vpu_fb_alloc_return = -1;
    VPUEnc enc;
    EXPECT_EQ(-1, vpu_enc_fb_alloc(&enc));
}

TEST(vpu_enc_fb_alloc, vpu_EncRegisterFrameBufferFailed) {
    vpu_EncGetInitialInfo_return = RETCODE_SUCCESS;
    vpu_fb_alloc_return = 0;
    vpu_EncRegisterFrameBuffer_return = RETCODE_FAILURE;
    VPUEnc enc;
    EXPECT_EQ(-1, vpu_enc_fb_alloc(&enc));
}
TEST(vpu_enc_fb_alloc, vpu_EncRegisterFrameBufferSucceed) {
    vpu_EncGetInitialInfo_return = RETCODE_SUCCESS;
    vpu_fb_alloc_return = 0;
    vpu_EncRegisterFrameBuffer_return = RETCODE_SUCCESS;
    VPUEnc enc;
    EXPECT_EQ(0, vpu_enc_fb_alloc(&enc));
}

//
// int vpu_enc_fb_fill(VPUEnc* enc, const vpu_enc_in* enc_in) 
//
TEST(vpu_enc_fb_fill, EncoderInputBufferLenNotEnough) {

    VPUEnc enc;
    enc.config.pixfmt = VPU_YUV420P;
    enc.config.width = 800;
    enc.config.height = 600;
    enc.srcfb = 0;
    vpu_enc_in in;
    in.buflen = 0;
    EXPECT_EQ(-1, vpu_enc_fb_fill(&enc, &in));
}
TEST(vpu_enc_fb_fill, EncoderInputBufferLenAsRequried) {

    VPUEnc enc;
    enc.config.pixfmt = VPU_YUV420P;
    enc.config.width = 0;
    enc.config.height = 0;
    enc.srcfb = 0;
    enc.fbmem[0].size = 0; 
    vpu_enc_in in;
    in.buflen = 0;
    EXPECT_EQ(0, vpu_enc_fb_fill(&enc, &in));
}
TEST(vpu_enc_fb_fill, EncoderInputBufferLenOK) {

    VPUEnc enc;
    enc.config.pixfmt = VPU_YUV420P;
    enc.config.width = 0;
    enc.config.height = 0;
    enc.srcfb = 0;
    enc.fbmem[0].size = 1; 
    vpu_enc_in in;
    in.buflen = 0;
    EXPECT_EQ(0, vpu_enc_fb_fill(&enc, &in));
}

//
// int vpu_enc_bs_alloc(VPUEnc* enc);
// void vpu_enc_bs_free(VPUEnc* enc);
//
TEST(vpu_enc_bs_alloc, IOGetPhyMemFailed) {
    VPUEnc enc;
    IOGetPhyMem_return = -1;
    EXPECT_EQ(-1, vpu_enc_bs_alloc(&enc));
}
TEST(vpu_enc_bs_alloc, IOGetVirtMemFailed) {
    IOGetPhyMem_return = 0;
    IOGetVirtMem_return = -1;
    VPUEnc enc;
    EXPECT_EQ(-1, vpu_enc_bs_alloc(&enc));
}
TEST(vpu_enc_bs_alloc, IOGetVirtMemSucceed) {
    IOGetPhyMem_return = 0;
    IOGetVirtMem_return = 1;
    VPUEnc enc;
    EXPECT_EQ(0, vpu_enc_bs_alloc(&enc));
}
TEST(vpu_enc_bs_free, Succeed) {
    VPUEnc enc;
    enc.bitstream.size = 1;
    ASSERT_NO_THROW({
        vpu_enc_bs_free(&enc);
    });
}

// 
// int vpu_enc_fill_header(VPUEnc* enc, vpu_enc_out* enc_out, int* filled)
//

TEST(vpu_enc_fill_header, ForH264) {
    VPUEnc enc;
    enc.bitstream.virt_uaddr = 0;
    enc.bitstream.phy_addr = 0;
    enc.config.encfmt = VPU_FMT_H264;
    vpu_enc_out out;
    int filled;
    EXPECT_EQ(0, vpu_enc_fill_header(&enc, &out, &filled));
}
TEST(vpu_enc_fill_header, ForJPEG) {
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_JPEG;
    vpu_enc_out out;
    out.buf = (char*)malloc(STREAM_BUF_SIZE);
    int filled;
    EXPECT_EQ(0, vpu_enc_fill_header(&enc, &out, &filled));
    free(out.buf);
}

//
// int vpu_enc_fill_body(VPUEnc* enc, vpu_enc_out* enc_out, int offset);
//
TEST(vpu_enc_fill_body, vpu_EncGetOutputInfoFailed) {
    vpu_EncGetOutputInfo_return = RETCODE_FAILURE;
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_JPEG;
    vpu_enc_out out;
    int offset;
    EXPECT_EQ(-1, vpu_enc_fill_body(&enc, &out, offset));
}


TEST(vpu_enc_fill_body, OutputH264PFrame) {
    vpu_EncGetOutputInfo_return = RETCODE_SUCCESS;
    EncOutputInfo outputinfo;
    outputinfo.picType = 1;
    outputinfo.bitstreamSize = 0;
    vpu_EncGetOutputInfo_EncOutputInfo = &outputinfo;
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_H264;
    vpu_enc_out out;
    int offset = 0;
    EXPECT_EQ(0, vpu_enc_fill_body(&enc, &out, offset));
    vpu_EncGetOutputInfo_EncOutputInfo = NULL;
}
TEST(vpu_enc_fill_body, OutputIFrame) {
    vpu_EncGetOutputInfo_return = RETCODE_SUCCESS;
    EncOutputInfo outputinfo;
    outputinfo.picType = 0;
    outputinfo.bitstreamSize = 0;
    vpu_EncGetOutputInfo_EncOutputInfo = &outputinfo;
    VPUEnc enc;
    enc.config.encfmt = VPU_FMT_H264;
    vpu_enc_out out;
    int offset = 0;
    EXPECT_EQ(0, vpu_enc_fill_body(&enc, &out, offset));
    vpu_EncGetOutputInfo_EncOutputInfo = NULL;
}


//
// vpu_enc_start_frame(VPUEnc* enc)
//
TEST(vpu_enc_start_frame, vpu_EncStartOneFrameFailed) {
    vpu_EncStartOneFrame_return = RETCODE_FAILURE;
    VPUEnc enc;    
    EXPECT_EQ(-1, vpu_enc_start_frame(&enc));
}

TEST(vpu_enc_start_frame, vpu_WaitForIntFailed) {
    vpu_EncStartOneFrame_return = RETCODE_SUCCESS;
    vpu_IsBusy_return = 1;
    vpu_WaitForInt_return = 1;
    VPUEnc enc;    
    EXPECT_EQ(-1, vpu_enc_start_frame(&enc));
}

TEST(vpu_enc_start_frame, vpu_WaitForIntSucceed) {
    vpu_EncStartOneFrame_return = RETCODE_SUCCESS;
    vpu_IsBusy_return = 1;
    vpu_WaitForInt_return = 0;
    VPUEnc enc;    
    EXPECT_EQ(0, vpu_enc_start_frame(&enc));
}

