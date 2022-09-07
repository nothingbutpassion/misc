#include <stdlib.h>
#include <string.h>

extern "C" { 
#include <vpu_lib.h>
#include <vpu_io.h>
}

#include "vpu_utils.h"
#include "vpu_enc_h264.h"


#define TAG "H264Enc"

H264Enc* vpu_enc_h264_open(const vpu_enc_config* config) {

    // Allocate JPEG implementation handle
    H264Enc* h264Enc = (H264Enc*)calloc(1, sizeof(H264Enc));
    if (!h264Enc) {
        LOGE(TAG, "calloc failed");
		return NULL;
    }

    // Obtain bitstream memory
    if (vpu_enc_bs_alloc(h264Enc) == -1) {
        free(h264Enc);
        return NULL;
    }

     // Fill common encoder param
	EncHandle handle = {0};
	EncOpenParam encop = {0};
	encop.bitstreamBuffer = h264Enc->bitstream.phy_addr;// bitstream buffer Physical Address
	encop.bitstreamBufferSize = STREAM_BUF_SIZE;        // bitstream buffer size
	encop.bitstreamFormat = STD_AVC;                    // bitstream format
	encop.picWidth = config->width;                     // picture width
	encop.picHeight = config->height;                   // picture height
	encop.frameRateInfo = 30;                           // Note: Frame rate cannot be less than 15fps per H.263 spec
	encop.bitRate = config->quality;                    // Note: bit rate in kbps. 0 - no rate control. For MJPEG, ignored
	encop.gopSize = config->gop_size;                   // GOP size where 0 = only first picture is I, 1 = all I pictures, 2 = IPIP, 3 = IPPIPP, and so on
	encop.slicemode.sliceMode = 0;	                    // 0: 1 slice per picture, 1: Multiple slices per picture 
	encop.slicemode.sliceSizeMode = 0;                  // 0: silceSize defined by bits,  1: sliceSize defined by MB number
	encop.slicemode.sliceSize = 4000;                   // size of a slice in bits or MB numbers
	encop.initialDelay = 0;
	encop.vbvBufferSize = 0;                            // 0 = ignore 8
	encop.intraRefresh = 0;
	encop.sliceReport = 0;                              // not used in i.MX 6
	encop.mbReport = 0;                                 // not used in i.MX 6
	encop.mbQpReport = 0;                               // not used in i.MX 6
	encop.rcIntraQp = -1;                               // quantization parameter for I frame, -1: auto, For H,264  is from 0-51; For JPEG, ignored
	encop.userQpMax = 0;
	encop.userQpMin = 0;
	encop.userQpMinEnable = 0;
	encop.userQpMaxEnable = 0;
	encop.IntraCostWeight = 0;
	encop.MEUseZeroPmv  = 0;
	encop.MESearchRange = 3;                            // (3: 16x16, 2:32x16, 1:64x32, 0:128x64, H.263(Short Header : always 3)
	encop.userGamma = (Uint32)(0.75*32768);             //  (0*32768 <= gamma <= 1*32768)
	encop.RcIntervalMode= 1;                            // 0: normal, 1: frame_level, 2: slice_level, 3: user defined Mb_level
	encop.MbInterval = 0;
	encop.avcIntra16x16OnlyModeEnable = 0;
	encop.ringBufferEnable = 0;                         // 0 = disable, 1 = enable streaming mode.
	encop.dynamicAllocEnable = 0;                       // not used in i.MX 6.                                        
    encop.chromaInterleave = 0;                         // chromaInterleave. NOTES: this have a effect on how to fill the framebuffer

    // Fill H264 param
    encop.EncStdParam.avcParam.avc_constrainedIntraPredFlag = 0;
    encop.EncStdParam.avcParam.avc_disableDeblk = 0;
    encop.EncStdParam.avcParam.avc_deblkFilterOffsetAlpha = 6;
    encop.EncStdParam.avcParam.avc_deblkFilterOffsetBeta = 0;
    encop.EncStdParam.avcParam.avc_chromaQpOffset = 10;
    encop.EncStdParam.avcParam.avc_audEnable = 0;
    encop.EncStdParam.avcParam.interview_en = 0;
    encop.EncStdParam.avcParam.paraset_refresh_en = 0;
    encop.EncStdParam.avcParam.prefix_nal_en = 0;
    encop.EncStdParam.avcParam.mvc_extension = 0;
    encop.EncStdParam.avcParam.avc_frameCroppingFlag = 0;
    encop.EncStdParam.avcParam.avc_frameCropLeft = 0;
    encop.EncStdParam.avcParam.avc_frameCropRight = 0;
    encop.EncStdParam.avcParam.avc_frameCropTop = 0;
    encop.EncStdParam.avcParam.avc_frameCropBottom = 0;
    if (encop.picHeight == 1080) {
        // In case of AVC encoder, when we want to use unaligned display width, frameCroppingFlag 
        // parameters should be adjusted to displayable rectangle
    	encop.EncStdParam.avcParam.avc_frameCroppingFlag = 1;
    	encop.EncStdParam.avcParam.avc_frameCropBottom = 8;
    }

    // Open encoder
    RetCode ret = vpu_EncOpen(&handle, &encop);
	if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_EncOpen failed: %d", ret);
        vpu_enc_bs_free(h264Enc);
        free(h264Enc);
		return NULL;
	}
    
    // Save the encoder handle
    h264Enc->config = *config;
    h264Enc->handle = handle;
    return h264Enc;
}


int vpu_enc_h264_close(H264Enc* h264Enc) {

    // Close encoder
	RetCode ret = vpu_EncClose(h264Enc->handle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE) {
		vpu_SWReset(h264Enc->handle, 0);
		ret = vpu_EncClose(h264Enc->handle);
	}

    // Free framebuffer memory
    vpu_enc_fb_free(h264Enc);

    // Free bitstream memory 
    vpu_enc_bs_free(h264Enc);

    // Free handle
    free(h264Enc);
    
    return ret == RETCODE_SUCCESS ? 0 : -1;
}



int vpu_enc_h264_start(H264Enc* h264Enc, const vpu_enc_in* in, vpu_enc_out* out) {

    // Allocate framebuffer if not allocated.
    if (h264Enc->totalfb == 0) {
        // Allocate frame buffer
        if (vpu_enc_fb_alloc(h264Enc) == -1) {
            return -1;
        }

        // Set intra refresh mode
        int intraRefreshMode = 1;
        vpu_EncGiveCommand(h264Enc->handle, ENC_SET_INTRA_REFRESH_MODE, &intraRefreshMode);         
    }

    // Fill framebuffer
    if (vpu_enc_fb_fill(h264Enc, in) == -1) {
        return -1;
    }

    // Fill H264 header
    int filled = 0;
    vpu_enc_fill_header(h264Enc, out, &filled);

    // Start one frame
    if (vpu_enc_start_frame(h264Enc) == -1) {
        return -1;
    }
    
    // Fill JPEG body
    if (vpu_enc_fill_body(h264Enc, out, filled) == -1) {
        return -1;
    }
    return 0;
}


