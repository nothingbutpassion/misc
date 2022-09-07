#include <gtest/gtest.h>

#include "vpu_lib_mock.h"
#include "vpu_io_mock.h"
#include "vpu_fb_mock.h"
#include "vpu_utils_mock.h"

#include "vpu_codec.h"
#include "vpu_dec.h"


//
// int vpu_dec_check(const vpu_dec_config* config, const vpu_dec_in* in, vpu_dec_out* out)
//
TEST(vpu_dec_check, DecFmtInvalid) {
    vpu_dec_config config;
    config.decfmt = VPU_FMT_VP8;
    vpu_dec_in in;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_check(&config, &in, &out));
}
TEST(vpu_dec_check, InputBufInvalid) {
    vpu_dec_config config;
    config.decfmt = VPU_FMT_JPEG;
    vpu_dec_in in;
    in.buf = NULL;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_check(&config, &in, &out));
}
TEST(vpu_dec_check, InputBufLenInvalid) {
    vpu_dec_config config;
    config.decfmt = VPU_FMT_JPEG;
    vpu_dec_in in;
    char dummy;
    in.buf = &dummy;
    in.buflen = 0;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_check(&config, &in, &out));
}
TEST(vpu_dec_check, OutputBufInvalid) {
    vpu_dec_config config;
    config.decfmt = VPU_FMT_JPEG;
    vpu_dec_in in;
    char dummy;
    in.buf = &dummy;
    in.buflen = 1;
    vpu_dec_out out;
    out.buf = NULL;
    EXPECT_EQ(-1, vpu_dec_check(&config, &in, &out));
}
TEST(vpu_dec_check, OutputBufLenInvalid) {
    vpu_dec_config config;
    config.decfmt = VPU_FMT_JPEG;
    vpu_dec_in in;
    char dummy;
    in.buf = &dummy;
    in.buflen = 1;
    vpu_dec_out out;
    out.buf = &dummy;
    out.buflen = 0;
    EXPECT_EQ(-1, vpu_dec_check(&config, &in, &out));
} 
TEST(vpu_dec_check, ArgsCorrect) {
    vpu_dec_config config;
    config.decfmt = VPU_FMT_JPEG;
    vpu_dec_in in;
    char dummy;
    in.buf = &dummy;
    in.buflen = 1;
    vpu_dec_out out;
    out.buf = &dummy;
    out.buflen = 1;
    EXPECT_EQ(0, vpu_dec_check(&config, &in, &out));
} 


//
// int vpu_dec_ps_alloc(VPUDec* dec)
// void vpu_dec_ps_free(VPUDec* dec);
//
TEST(vpu_dec_ps_alloc, IOGetPhyMemFailed) {
    IOGetPhyMem_return = -1;
    VPUDec dec;
    EXPECT_EQ(-1, vpu_dec_ps_alloc(&dec));
}
TEST(vpu_dec_ps_alloc, IOGetPhyMemSucceed) {
    IOGetPhyMem_return = 0;
    VPUDec dec;
    EXPECT_EQ(0, vpu_dec_ps_alloc(&dec));
}
TEST(vpu_dec_ps_free, Succeed) {
    VPUDec dec;
    dec.ps.size = 1;
    ASSERT_NO_THROW({
        vpu_dec_ps_free(&dec);
    });
}

//
// int vpu_dec_bs_alloc(VPUDec* dec)
//
TEST(vpu_dec_bs_alloc, IOGetPhyMemFailed) {
    IOGetPhyMem_return = -1;
    VPUDec dec;
    EXPECT_EQ(-1, vpu_dec_bs_alloc(&dec));
}
TEST(vpu_dec_bs_alloc, IOGetVirtMemFailed) {
    IOGetPhyMem_return = 0;
    IOGetVirtMem_return = -1;
    VPUDec dec;
    EXPECT_EQ(-1, vpu_dec_bs_alloc(&dec));
}
TEST(vpu_dec_bs_alloc, Succeed) {
    IOGetPhyMem_return = 0;
    IOGetVirtMem_return = 1;
    VPUDec dec;
    EXPECT_EQ(0, vpu_dec_bs_alloc(&dec));
}
TEST(vpu_dec_bs_free, Succeed) {
    VPUDec dec;
    dec.bitstream.size = 1;
    ASSERT_NO_THROW({
        vpu_dec_bs_free(&dec);
    });
}

//
// int vpu_dec_bs_fill(VPUDec* dec, const vpu_dec_in* dec_in)
//

TEST(vpu_dec_bs_fill, vpu_DecGetBitstreamBufferFailed) {
    vpu_DecGetBitstreamBuffer_return =RETCODE_FAILURE;
    VPUDec dec;
    vpu_dec_in in;
    EXPECT_EQ(-1, vpu_dec_bs_fill(&dec, &in));
}
TEST(vpu_dec_bs_fill, InputBufLenInvalid) {
    vpu_DecGetBitstreamBuffer_return =RETCODE_SUCCESS;
    Uint32 size = 0;
    vpu_DecGetBitstreamBuffer_size = &size;
    VPUDec dec;
    vpu_dec_in in;
    in.buflen = 1;
    EXPECT_EQ(-1, vpu_dec_bs_fill(&dec, &in));
    vpu_DecGetBitstreamBuffer_size = NULL;
}
TEST(vpu_dec_bs_fill, vpu_DecUpdateBitstreamBufferFailed) {
    vpu_DecGetBitstreamBuffer_return =RETCODE_SUCCESS;
    Uint32 size = 0;
    vpu_DecGetBitstreamBuffer_size = &size;
    PhysicalAddress write = 0;
    vpu_DecGetBitstreamBuffer_write = &write;
    vpu_DecUpdateBitstreamBuffer_return = RETCODE_FAILURE;
    VPUDec dec;
    dec.bitstream.virt_uaddr = 0;
    dec.bitstream.phy_addr = 0;
    dec.bitstream.size = 0;
    vpu_dec_in in;
    in.buflen = 0;
    EXPECT_EQ(-1, vpu_dec_bs_fill(&dec, &in));
    
    vpu_DecGetBitstreamBuffer_size = NULL;
    vpu_DecGetBitstreamBuffer_write = NULL;
}
TEST(vpu_dec_bs_fill, vpu_DecUpdateBitstreamBufferSucceed) {
    vpu_DecGetBitstreamBuffer_return = RETCODE_SUCCESS;
    Uint32 size = 0;
    vpu_DecGetBitstreamBuffer_size = &size;
    PhysicalAddress write = 0;
    vpu_DecGetBitstreamBuffer_write = &write;
    vpu_DecUpdateBitstreamBuffer_return = RETCODE_SUCCESS;
    VPUDec dec;
    dec.bitstream.virt_uaddr = 0;
    dec.bitstream.phy_addr = 0;
    dec.bitstream.size = 0;
    vpu_dec_in in;
    in.buflen = 0;
    EXPECT_EQ(0, vpu_dec_bs_fill(&dec, &in));
    
    vpu_DecGetBitstreamBuffer_size = NULL;
    vpu_DecGetBitstreamBuffer_write = NULL;
}


//
// int vpu_dec_fb_alloc(VPUDec* dec);
// void vpu_dec_fb_free(VPUDec* dec)
//
TEST(vpu_dec_fb_alloc, vpu_DecGetInitialInfoFailed) {
    vpu_DecGetInitialInfo_return = RETCODE_FAILURE;
    VPUDec dec;
    EXPECT_EQ(-1, vpu_dec_fb_alloc(&dec));
}
TEST(vpu_dec_fb_alloc, streamInfoObtainedFailed) {
    vpu_DecGetInitialInfo_return = RETCODE_SUCCESS;
    DecInitialInfo initinfo;
    initinfo.streamInfoObtained = 0;
    vpu_DecGetInitialInfo_DecInitialInfo = &initinfo;
    VPUDec dec;
    EXPECT_EQ(-1, vpu_dec_fb_alloc(&dec));
    vpu_DecGetInitialInfo_DecInitialInfo = NULL;
}
TEST(vpu_dec_fb_alloc, PicSizeInvalid) {
    vpu_DecGetInitialInfo_return = RETCODE_SUCCESS;
    DecInitialInfo initinfo;
    initinfo.streamInfoObtained = 1;
    initinfo.picWidth = 0;
    initinfo.picHeight = 0;
    initinfo.minFrameBufferCount = 0;
    vpu_DecGetInitialInfo_DecInitialInfo = &initinfo;
    VPUDec dec;
    EXPECT_EQ(-1, vpu_dec_fb_alloc(&dec));
    vpu_DecGetInitialInfo_DecInitialInfo = NULL;
}
TEST(vpu_dec_fb_alloc, FBAllocFailed) {
    vpu_DecGetInitialInfo_return = RETCODE_SUCCESS;
    DecInitialInfo initinfo;
    initinfo.streamInfoObtained = 1;
    initinfo.picWidth = 1;
    initinfo.picHeight = 1;
    initinfo.minFrameBufferCount = 0;
    vpu_DecGetInitialInfo_DecInitialInfo = &initinfo;
    vpu_fb_alloc_return = -1;
    VPUDec dec;
    dec.totalfb = 1;
    EXPECT_EQ(-1, vpu_dec_fb_alloc(&dec));
    vpu_DecGetInitialInfo_DecInitialInfo = NULL;
}
TEST(vpu_dec_fb_alloc, vpu_DecRegisterFrameBufferFailed) {
    vpu_DecGetInitialInfo_return = RETCODE_SUCCESS;
    DecInitialInfo initinfo;
    initinfo.streamInfoObtained = 1;
    initinfo.picWidth = 1;
    initinfo.picHeight = 1;
    initinfo.minFrameBufferCount = 0;
    vpu_DecGetInitialInfo_DecInitialInfo = &initinfo;
    vpu_fb_alloc_return = 0;
    vpu_DecRegisterFrameBuffer_return = RETCODE_FAILURE;
    VPUDec dec;
    dec.totalfb = 1;
    EXPECT_EQ(-1, vpu_dec_fb_alloc(&dec));
    vpu_DecGetInitialInfo_DecInitialInfo = NULL;
}
TEST(vpu_dec_fb_alloc, vpu_DecRegisterFrameBufferSucceed) {
    vpu_DecGetInitialInfo_return = RETCODE_SUCCESS;
    DecInitialInfo initinfo;
    initinfo.streamInfoObtained = 1;
    initinfo.picWidth = 1;
    initinfo.picHeight = 1;
    initinfo.minFrameBufferCount = 0;
    vpu_DecGetInitialInfo_DecInitialInfo = &initinfo;
    vpu_fb_alloc_return = 0;
    vpu_DecRegisterFrameBuffer_return = RETCODE_SUCCESS;
    VPUDec dec;
    dec.totalfb = 1;
    EXPECT_EQ(0, vpu_dec_fb_alloc(&dec));
    vpu_DecGetInitialInfo_DecInitialInfo = NULL;
}

//
// int vpu_dec_start_frame(VPUDec* dec);
//
TEST(vpu_dec_start_frame, vpu_DecUpdateBitstreamBufferFailed) {
    vpu_DecUpdateBitstreamBuffer_return = RETCODE_FAILURE;
    VPUDec dec;
    EXPECT_EQ(-1, vpu_dec_start_frame(&dec));
}
TEST(vpu_dec_start_frame, vpu_DecStartOneFrameFailed) {
    vpu_DecUpdateBitstreamBuffer_return = RETCODE_SUCCESS;
    vpu_DecStartOneFrame_return = RETCODE_FAILURE;
    VPUDec dec;
    dec.config.decfmt = VPU_FMT_JPEG;
    EXPECT_EQ(-1, vpu_dec_start_frame(&dec));
}
TEST(vpu_dec_start_frame, vpu_WaitForIntFailed) {
    vpu_DecUpdateBitstreamBuffer_return = RETCODE_SUCCESS;
    vpu_DecStartOneFrame_return = RETCODE_SUCCESS;
    vpu_IsBusy_return = 1;
    vpu_WaitForInt_return = -1;
    VPUDec dec;
    dec.config.decfmt = VPU_FMT_JPEG;
    EXPECT_EQ(-1, vpu_dec_start_frame(&dec));
}
TEST(vpu_dec_start_frame, vpu_WaitForIntSucceed) {
    vpu_DecUpdateBitstreamBuffer_return = RETCODE_SUCCESS;
    vpu_DecStartOneFrame_return = RETCODE_SUCCESS;
    vpu_IsBusy_return = 1;
    vpu_WaitForInt_return = 0;
    VPUDec dec;
    dec.config.decfmt = VPU_FMT_JPEG;
    EXPECT_EQ(0, vpu_dec_start_frame(&dec));
}


//
// int vpu_dec_get_output(VPUDec* dec, vpu_dec_out* dec_out)
//
TEST(vpu_dec_start_frame, vpu_DecGetOutputInfoFailed) {
    vpu_DecGetOutputInfo_return = RETCODE_FAILURE;
    VPUDec dec;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_get_output(&dec, &out));
}
TEST(vpu_dec_start_frame, NotdecodingSuccess) {
    vpu_DecGetOutputInfo_return = RETCODE_SUCCESS;
    DecOutputInfo outinfo;
    outinfo.decodingSuccess = 1;
    outinfo.indexFrameDecoded = 1;
    vpu_DecGetOutputInfo_DecOutputInfo = &outinfo;
    VPUDec dec;
    dec.totalfb = 0;
    vpu_dec_out out;
    EXPECT_EQ(-1, vpu_dec_get_output(&dec, &out));

}
TEST(vpu_dec_start_frame, OutputBufLenInvalid) {
    vpu_DecGetOutputInfo_return = RETCODE_SUCCESS;
    DecOutputInfo outinfo;
    outinfo.decodingSuccess = 1;
    outinfo.indexFrameDecoded = 0;
    outinfo.decPicWidth = 1; 
    outinfo.decPicHeight = 1;
    vpu_DecGetOutputInfo_DecOutputInfo = &outinfo;
    VPUDec dec;
    dec.totalfb = 1;
    vpu_dec_out out;
    out.buflen = 0;
    EXPECT_EQ(-1, vpu_dec_get_output(&dec, &out));
}
TEST(vpu_dec_start_frame, vpu_DecClrDispFlagFailed) {
    vpu_DecGetOutputInfo_return = RETCODE_SUCCESS;
    DecOutputInfo outinfo;
    outinfo.decodingSuccess = 1;
    outinfo.indexFrameDecoded = 0;
    outinfo.decPicWidth = 0; 
    outinfo.decPicHeight = 0;
    vpu_DecGetOutputInfo_DecOutputInfo = &outinfo;
    vpu_DecClrDispFlag_return = RETCODE_FAILURE;
    VPUDec dec;
    dec.totalfb = 1;
    dec.config.decfmt = VPU_FMT_H264;
    vpu_dec_out out;
    char dummy;
    out.buf = &dummy;
    out.buflen = 0;
    EXPECT_EQ(-1, vpu_dec_get_output(&dec, &out));
}
TEST(vpu_dec_start_frame, vpu_DecClrDispFlagSucceed) {
    vpu_DecGetOutputInfo_return = RETCODE_SUCCESS;
    DecOutputInfo outinfo;
    outinfo.decodingSuccess = 1;
    outinfo.indexFrameDecoded = 0;
    outinfo.decPicWidth = 0; 
    outinfo.decPicHeight = 0;
    vpu_DecGetOutputInfo_DecOutputInfo = &outinfo;
    vpu_DecClrDispFlag_return = RETCODE_SUCCESS;
    VPUDec dec;
    dec.totalfb = 1;
    dec.config.decfmt = VPU_FMT_JPEG;
    vpu_dec_out out;
    char dummy;
    out.buf = &dummy;
    out.buflen = 0;
    EXPECT_EQ(0, vpu_dec_get_output(&dec, &out));
}


