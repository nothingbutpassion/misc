#include <stdlib.h>
#include <string.h>

extern "C" { 
#include <vpu_lib.h>
#include <vpu_io.h>
}

#include "vpu_utils.h"
#include "vpu_enc_jpeg.h"


#define TAG "JPEGEnc"


static unsigned char lumaDcBits[16] = {
    0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char lumaDcValue[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char lumaAcBits[16] = {
    0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
    0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D,
};
static unsigned char lumaAcValue[168] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
    0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
    0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
    0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
    0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char chromaDcBits[16] = {
    0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char chromaDcValue[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char chromaAcBits[16] = {
    0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
    0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
};
static unsigned char chromaAcValue[168] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
    0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
    0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
    0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
    0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
    0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static unsigned char lumaQ2[64] = {
    0x06, 0x04, 0x04, 0x04, 0x05, 0x04, 0x06, 0x05,
    0x05, 0x06, 0x09, 0x06, 0x05, 0x06, 0x09, 0x0B,
    0x08, 0x06, 0x06, 0x08, 0x0B, 0x0C, 0x0A, 0x0A,
    0x0B, 0x0A, 0x0A, 0x0C, 0x10, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x10, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
};
static unsigned char chromaBQ2[64] = {
    0x07, 0x07, 0x07, 0x0D, 0x0C, 0x0D, 0x18, 0x10,
    0x10, 0x18, 0x14, 0x0E, 0x0E, 0x0E, 0x14, 0x14,
    0x0E, 0x0E, 0x0E, 0x0E, 0x14, 0x11, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x11, 0x11, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x11, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
};
static unsigned char cInfoTable[5][24] = {
    { 00, 02, 02, 00, 00, 00, 01, 01, 01, 01, 01, 01, 02, 01, 01, 01, 01, 01, 03, 00, 00, 00, 00, 00 }, //420
    { 00, 02, 01, 00, 00, 00, 01, 01, 01, 01, 01, 01, 02, 01, 01, 01, 01, 01, 03, 00, 00, 00, 00, 00 }, //422H
    { 00, 01, 02, 00, 00, 00, 01, 01, 01, 01, 01, 01, 02, 01, 01, 01, 01, 01, 03, 00, 00, 00, 00, 00 }, //422V
    { 00, 01, 01, 00, 00, 00, 01, 01, 01, 01, 01, 01, 02, 01, 01, 01, 01, 01, 03, 00, 00, 00, 00, 00 }, //444
    { 00, 01, 01, 00, 00, 00, 01, 00, 00, 00, 00, 00, 02, 00, 00, 00, 00, 00, 03, 00, 00, 00, 00, 00 }, //400
};


static void jpgGetHuffTable(EncMjpgParam *param) {
    // Rearrange and insert pre-defined Huffman table to deticated variable.
    memcpy(param->huffBits[DC_TABLE_INDEX0], lumaDcBits, 16);       // Luma DC BitLength
    memcpy(param->huffVal[DC_TABLE_INDEX0], lumaDcValue, 16);       // Luma DC HuffValue

    memcpy(param->huffBits[AC_TABLE_INDEX0], lumaAcBits, 16);       // Luma DC BitLength
    memcpy(param->huffVal[AC_TABLE_INDEX0], lumaAcValue, 162);      // Luma DC HuffValue

    memcpy(param->huffBits[DC_TABLE_INDEX1], chromaDcBits, 16);     // Chroma DC BitLength
    memcpy(param->huffVal[DC_TABLE_INDEX1], chromaDcValue, 16);     // Chroma DC HuffValue

    memcpy(param->huffBits[AC_TABLE_INDEX1], chromaAcBits, 16);     // Chroma AC BitLength
    memcpy(param->huffVal[AC_TABLE_INDEX1], chromaAcValue, 162);    // Chorma AC HuffValue
}

static void jpgGetQMatrix(EncMjpgParam *param) {
	// Rearrange and insert pre-defined Q-matrix to deticated variable.

    //
    // The following code is used only for testing target 
    // Testing results: Q=5, 15, 80, 90  works well, Q=95, 100, leads to Floating point exception (Core dumped)
//    int Q = 80;
//    int S = (Q < 50) ? 5000/Q : 200 - 2*Q;
//    for (int i=0; i < 64; i++) {
//        lumaQ2[i] = Uint8((S*lumaQ2[i] + 50)/100);
//        chromaBQ2[i] =Uint8((S*chromaBQ2[i] + 50)/100);
//    }
    
    memcpy(param->qMatTab[DC_TABLE_INDEX0], lumaQ2, 64);
    memcpy(param->qMatTab[AC_TABLE_INDEX0], chromaBQ2, 64);

    memcpy(param->qMatTab[DC_TABLE_INDEX1], param->qMatTab[DC_TABLE_INDEX0], 64);
    memcpy(param->qMatTab[AC_TABLE_INDEX1], param->qMatTab[AC_TABLE_INDEX0], 64);
}

static void jpgGetCInfoTable(EncMjpgParam *param) {
	int format = param->mjpg_sourceFormat;
	memcpy(param->cInfoTab, cInfoTable[format], 6 * 4);
}


JPEGEnc* vpu_enc_jpeg_open(const vpu_enc_config* config) {

    // Allocate JPEG implementation handle
    JPEGEnc* jpegEnc = (JPEGEnc*)calloc(1, sizeof(JPEGEnc));
    if (!jpegEnc) {
        LOGE(TAG, "calloc failed");
		return NULL;
    }

    // Obtain bitstream memory
    if (vpu_enc_bs_alloc(jpegEnc) == -1) {
        free(jpegEnc);
        return NULL;
    }

     // Fill common encoder param
	EncHandle handle = {0};
	EncOpenParam encop = {0};
	encop.bitstreamBuffer = jpegEnc->bitstream.phy_addr;// bitstream buffer Physical Address
	encop.bitstreamBufferSize = 0x200000;               // bitstream buffer size
	encop.bitstreamFormat = STD_MJPG;                   // bitstream format
	encop.picWidth = config->width;                     // picture width
	encop.picHeight = config->height;                   // picture height
	encop.frameRateInfo = 30;                           // Note: Frame rate cannot be less than 15fps per H.263 spec
	encop.bitRate = 0;                                  // Note: bit rate in kbps. 0 - no rate control. For MJPEG, ignored
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

    // Fill JPEG param
    encop.EncStdParam.mjpgParam.mjpg_sourceFormat = config->pixfmt;  // chrominance size 0 = 4:2:0, 1 = 4:2:2 horizontal, 2 = 4:2:2 vertical, 3 = 4:4:4, 4 = 4:0:0.
    encop.EncStdParam.mjpgParam.mjpg_restartInterval = 60;
    encop.EncStdParam.mjpgParam.mjpg_thumbNailEnable = 0;
    encop.EncStdParam.mjpgParam.mjpg_thumbNailWidth = 0;
    encop.EncStdParam.mjpgParam.mjpg_thumbNailHeight = 0;
    jpgGetHuffTable(&encop.EncStdParam.mjpgParam);
    jpgGetQMatrix(&encop.EncStdParam.mjpgParam);
    jpgGetCInfoTable(&encop.EncStdParam.mjpgParam);

    // Open encoder
    RetCode ret = vpu_EncOpen(&handle, &encop);
	if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_EncOpen failed: %d", ret);
		return NULL;
	}
    
    // Save the encoder handle
    jpegEnc->config = *config;
    jpegEnc->handle = handle;
    return jpegEnc;
}


int vpu_enc_jpeg_close(JPEGEnc* jpegEnc) {

    // Close encoder
	RetCode ret = vpu_EncClose(jpegEnc->handle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE) {
		vpu_SWReset(jpegEnc->handle, 0);
		ret = vpu_EncClose(jpegEnc->handle);
	}

    // Free framebuffer memory
    vpu_enc_fb_free(jpegEnc);

    // Free bitstream memory 
    vpu_enc_bs_free(jpegEnc);

    // Free handle
    free(jpegEnc);

    return ret == RETCODE_SUCCESS ? 0 : -1;
}



int vpu_enc_jpeg_start(JPEGEnc* jpegEnc, const vpu_enc_in* in, vpu_enc_out* out) {
//    int intraRefreshMode = 1;
//    vpu_EncGiveCommand(jpegEnc->handle, ENC_SET_INTRA_REFRESH_MODE, &intraRefreshMode);

    // Allocate framebuffer if not allocated.
    if (jpegEnc->totalfb == 0) {
        if (vpu_enc_fb_alloc(jpegEnc) == -1) {
            return -1;
        }
    }

    // Fill framebuffer
    if (vpu_enc_fb_fill(jpegEnc, in) == -1) {
        return -1;
    }

    // Fill JPEG header
    int filled = 0;
    if (vpu_enc_fill_header(jpegEnc, out, &filled) == -1) {
        return -1;
    }
    
    // Start one frame
    if (vpu_enc_start_frame(jpegEnc) == -1) {
        return -1;
    }
    
    // Fill JPEG body
    if (vpu_enc_fill_body(jpegEnc, out, filled) == -1) {
        return -1;
    }
    return 0;
}



