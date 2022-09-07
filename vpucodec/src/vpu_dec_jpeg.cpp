#include <stdlib.h>

#include "vpu_dec_jpeg.h"
#include "vpu_utils.h"

#define TAG "JPEGDec"


JPEGDec* vpu_dec_jpeg_open(const vpu_dec_config* config) {

    // Allocate JPEG decoding implementation handle
    JPEGDec* jpegDec = (JPEGDec*)calloc(1, sizeof(JPEGDec));
    if (!jpegDec) {
        LOGE(TAG, "calloc failed");
		return NULL;
    }

    // Allocate bitstream buffer
    if (vpu_dec_bs_alloc(jpegDec) == -1) {
        free(jpegDec);
        return NULL;
    }


    // Fill JPEG decoder param
	DecHandle handle = {0};
	DecOpenParam oparam = {STD_MJPG};                           // The first field of DecOpenParamis bitstreamFormat
	oparam.bitstreamFormat = STD_MJPG;                          // Bitstream format
	oparam.bitstreamBuffer = jpegDec->bitstream.phy_addr;       // Bitstream buffer physical address
	oparam.bitstreamBufferSize = jpegDec->bitstream.size;       // Bitstream buffer size
	oparam.pBitStream = (Uint8*)jpegDec->bitstream.virt_uaddr;  // Bitstream buffer virtaul address
	oparam.reorderEnable = 0;                                   // 0 = disable, 1 = enables display buffer reordering for decoding H.264 bitstream

    oparam.mp4DeblkEnable = 0;                                  // 0 = disable, 1 = enable. For MPEG4 and H.263 (post-processing), MPEG-4 deblocking filtered output to host application                             
	oparam.chromaInterleave = 0;                                // NOTE: 0 = CbCr not interleaved, 1 = CbCr interleaved.                                
	oparam.mp4Class = 0;                                        // MEPG-4 type: 0 = MPEG-4; 1 = DivX 5.0 or higher; 2 = Xvid; 5 = DivX 4.0
    oparam.avcExtension = 0;                                    // Not used on mx6
	oparam.mjpg_thumbNailDecEnable = 0;                         // Not used on mx6
	oparam.mapType = 0;                                         // Map type for GDI inferface. 0 is a linear frame map. 1 is a frame tiled map. 2 is a filed tiled map
	oparam.tiled2LinearEnable = 0;                              // Tiled to linear map enable mode. Used to the post processing unit for display
	oparam.bitstreamMode = 0;                                   // 0: VPU sends interrupt for more bitstream (interrupt mode);1: VPU returns to the status right before PIC_RUN (rollback mode). 
	oparam.jpgLineBufferMode = 0;                               // 0 is a LineBuffer mode and 1 is a streaming mode
	oparam.psSaveBuffer = 0;                                    // For H.264, A start address which the decoder saves PS (SPS/PPS) RBSP. 8 byte-aligned
	oparam.psSaveBufferSize = 0;                                // PS Save Buffer size

    // Open JPEG decoder
	RetCode ret = vpu_DecOpen(&handle, &oparam);
	if (ret != RETCODE_SUCCESS) {
		LOGE(TAG, "vpu_DecOpen failed: %d", ret);
        vpu_dec_bs_free(jpegDec);
        free(jpegDec);
		return NULL;
	}

    jpegDec->config = *config;
	jpegDec->handle = handle;
    return jpegDec;
}

int vpu_dec_jpeg_close(JPEGDec* jpegDec) {

    // Close JPEG decoder
	RetCode ret = vpu_DecClose(jpegDec->handle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE) {
		vpu_SWReset(jpegDec->handle, 0);
		ret = vpu_DecClose(jpegDec->handle);
		if (ret != RETCODE_SUCCESS) {
			LOGE(TAG, "vpu_DecClose failed");
        }
	}

    // Free bitstream buffer
    vpu_dec_bs_free(jpegDec);
    free(jpegDec);

    return ret == RETCODE_SUCCESS ? 0 : -1;
}

int vpu_dec_jpeg_start(JPEGDec* jpegDec, const vpu_dec_in* in, vpu_dec_out* out) {

    // Fill bitstream buffer
    if (vpu_dec_bs_fill(jpegDec, in) == -1) {
        return -1;
    }

    // Allocate framebuffer if not existed
    if (jpegDec->totalfb == 0) {
        if (vpu_dec_fb_alloc(jpegDec) == -1) {
            return -1;
        }
    }

    // Start decoding one frame
    if (vpu_dec_start_frame(jpegDec) == -1) {
        return -1;
    }

    // Get decoding output
    if (vpu_dec_get_output(jpegDec, out) == -1) {
        return -1;
    }

    
    return 0;
}

