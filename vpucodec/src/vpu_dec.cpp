#include <unistd.h>
#include <string.h>
#include "vpu_dec.h"
#include "vpu_utils.h"
#include "vpu_fb.h"


#define TAG "VPUDec"



int vpu_dec_check(const vpu_dec_config* config, const vpu_dec_in* in, vpu_dec_out* out) {
    // Check decoder config
    if (config) {
        if (config->decfmt != VPU_FMT_JPEG && config->decfmt != VPU_FMT_H264) {
            LOGE(TAG, "vpu_dec_config::decfmt is %d (not supported)", config->decfmt);
            return -1;
        }
    }

    // Check decoder input
    if (in) {
        if (!in->buf) {
            LOGE(TAG, "vpu_dec_in::buf is NULL");
            return -1;
        }

        if (in->buflen == 0) {
            LOGE(TAG, "vpu_dec_in::buflen is 0");
            return -1;
        }
    }

    // Check decoder output
    if (out) {
        if (!out->buf) {
            LOGE(TAG, "vpu_dec_out::buf is NULL");
            return -1;
        }

        if (out->buflen == 0) {
            LOGE(TAG, "vpu_dec_out::buflen is 0");
            return -1;
        } 
    }

    return 0;
}



int vpu_dec_ps_alloc(VPUDec* dec) {
    // Obtain physical memory
    vpu_mem_desc mem = {0};
	mem.size = PS_SAVE_SIZE;
	int ret = IOGetPhyMem(&mem);
	if (ret != RETCODE_SUCCESS) {
		LOGE(TAG, "IOGetPhyMem failed: %d", ret);
		return -1;
	}

    dec->ps = mem;
    return 0;
}

void vpu_dec_ps_free(VPUDec* dec) {
    if (dec->ps.size > 0) {
        IOFreePhyMem(&dec->bitstream);
        memset(&dec->bitstream, 0, sizeof(vpu_mem_desc));
    }
}


int vpu_dec_bs_alloc(VPUDec* dec) {
    // Obtain physical memory
    vpu_mem_desc mem = {0};
	mem.size = STREAM_BUF_SIZE;
	int ret = IOGetPhyMem(&mem);
	if (ret != RETCODE_SUCCESS) {
		LOGE(TAG, "IOGetPhyMem failed: %d", ret);
		return -1;
	}

	// Obtain virtual address
	int virtaddr = IOGetVirtMem(&mem);
	if (virtaddr <= 0) {
		LOGE(TAG, "IOGetVirtMem failed: %d", virtaddr);
        IOFreePhyMem(&mem);
        return -1;
    }

    dec->bitstream = mem;
    return 0;
}

void vpu_dec_bs_free(VPUDec* dec) {
    if (dec->bitstream.size > 0) {
        IOFreeVirtMem(&dec->bitstream);
        IOFreePhyMem(&dec->bitstream);
        memset(&dec->bitstream, 0, sizeof(vpu_mem_desc));
    }
}


int vpu_dec_bs_fill(VPUDec* dec, const vpu_dec_in* dec_in) {
    // Get bitstream buffer    
    PhysicalAddress pa_read;
    PhysicalAddress pa_write;
    Uint32 size;
    RetCode ret = vpu_DecGetBitstreamBuffer(dec->handle, &pa_read, &pa_write, &size);
    if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_DecGetBitstreamBuffer failed: %d", ret);
        return -1;
    }
    LOGI(TAG, "BitstreamBuffer: bitstream=%lu, read=%u, write=%u, size=%u", dec->bitstream.phy_addr, pa_read, pa_write, size);

    // Fill bitstream buffer 
    unsigned long va_write = dec->bitstream.virt_uaddr + pa_write - dec->bitstream.phy_addr;
    if (dec_in->buflen > size) {
        LOGE(TAG, "input buffer length (%lu) greater than availabe bitstream space (%lu)", dec_in->buflen, size);
        return -1;
    }
    // NOTE: bitstream buffer operates as a ring-buffer
    unsigned long room = dec->bitstream.virt_uaddr + dec->bitstream.size - va_write;
    if (room < dec_in->buflen) {
        memcpy((void*)va_write, dec_in->buf, room);
        memcpy((void*)dec->bitstream.virt_uaddr, dec_in->buf + room, dec_in->buflen - room);
    } else {
        memcpy((void*)va_write, dec_in->buf, dec_in->buflen);
    }

    // Update bitstream buffer
    ret = vpu_DecUpdateBitstreamBuffer(dec->handle, dec_in->buflen);
    if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_DecUpdateBitstreamBuffer failed: %d", ret);
        return -1;
    }

    return 0;
}


int vpu_dec_fb_alloc(VPUDec* dec) {
	// Parse bitstream and get width/height/framerate etc.
    DecInitialInfo initinfo = {0};
	vpu_DecSetEscSeqInit(dec->handle, 1);
	RetCode ret = vpu_DecGetInitialInfo(dec->handle, &initinfo);
	vpu_DecSetEscSeqInit(dec->handle, 0);
	if (ret != RETCODE_SUCCESS) {
		LOGE(TAG, "vpu_DecGetInitialInfo failed: %d, InitialInfo error: %d",
            ret, initinfo.errorcode);
		return -1;
	}

	if (!initinfo.streamInfoObtained) {
        LOGE(TAG, "can't obtain stream info");
        return -1; 
    }
    if (!initinfo.picWidth || !initinfo.picHeight) {
        LOGE(TAG, "invalid DecInitialInfo: picWidth=%d, picHeight=%d", initinfo.picWidth, initinfo.picHeight);
		return -1;
    }

    LOGD(TAG, "DecInitialInfo: picWidth=%d, picHeight=%d, frameRateRes=%lu, frameRateDiv=%lu, minFrameBufferCount=%u",
        initinfo.picWidth, initinfo.picHeight, initinfo.frameRateRes, initinfo.frameRateDiv, initinfo.minFrameBufferCount);

	//
	// We suggest to add two more buffers than minFrameBufferCount:
	//
	// vpu_DecClrDispFlag is used to control framebuffer whether can be
	// used for decoder again. One framebuffer dequeue from IPU is delayed
	// for performance improvement and one framebuffer is delayed for
	// display flag clear.
	//
	// Performance is better when more buffers are used if IPU performance
	// is bottleneck.
	//
	// Two more buffers may be needed for interlace stream from IPU DVI view
	//
    dec->totalfb = initinfo.minFrameBufferCount + 2;
    dec->width = (initinfo.picWidth + 15) & ~15;
    dec->height = (initinfo.picHeight + 15) & ~15;
    int err = 0;
    int mvCol = dec->config.decfmt == VPU_FMT_JPEG ? 0 : 1;
    for (int i = 0; i < dec->totalfb; i++) {
        dec->fb[i].myIndex = i;
        err |= vpu_fb_alloc(&dec->fbmem[i], MODE420, dec->width, dec->height, mvCol, &dec->fb[i]);
    }
    if (err) {
        vpu_dec_fb_free(dec);
        return -1;
    }

	// User needs to fill max suported macro block value of frame as following
	DecBufInfo bufinfo = {0};
	bufinfo.maxDecFrmInfo.maxMbX = dec->width / 16;
	bufinfo.maxDecFrmInfo.maxMbY = dec->height / 16;
	bufinfo.maxDecFrmInfo.maxMbNum = dec->width * dec->height / 256;

	// For H.264, we can overwrite initial delay calculated from syntax.
	// delay can be 0,1,... (in unit of frames)
	// Set to -1 or do not call this command if you don't want to overwrite it.
	// Take care not to set initial delay lower than reorder depth of the clip, 
	// otherwise, display will be out of order.
	int delay = -1;
	vpu_DecGiveCommand(dec->handle, DEC_SET_FRAME_DELAY, &delay);
	ret = vpu_DecRegisterFrameBuffer(dec->handle, dec->fb, dec->totalfb, dec->width, &bufinfo);
	if (ret != RETCODE_SUCCESS) {
		LOGE(TAG, "vpu_DecRegisterFrameBuffer failed: %d", ret);
		return -1;
	}

    return 0;
}

void vpu_dec_fb_free(VPUDec* dec) {
    for (int i = 0; i < dec->totalfb; i++) {
        vpu_fb_free(&dec->fbmem[i], &dec->fb[i]);
    }
    dec->totalfb = 0;
    dec->width = 0; 
    dec->height = 0;
}


int vpu_dec_start_frame(VPUDec* dec) {
    // Inform VPU decoder bitstream input is end
    RetCode ret = vpu_DecUpdateBitstreamBuffer(dec->handle, STREAM_END_SIZE);
    if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_DecUpdateBitstreamBuffer failed: %d", ret);
        return -1;
    }

    // For JPEG decoder
	if (dec->config.decfmt== VPU_FMT_JPEG) {
		//
		//  VPU is setting the rotation angle by counter-clockwise.
		//  We convert it to clockwise, which is consistent with V4L2
		//  rotation angle strategy.
		//
        int rot_angle = 0;
		vpu_DecGiveCommand(dec->handle, SET_ROTATION_ANGLE, &rot_angle);

		int mirror = 0;
		vpu_DecGiveCommand(dec->handle, SET_MIRROR_DIRECTION, &mirror);

	    int rot_stride = dec->width;
		vpu_DecGiveCommand(dec->handle, SET_ROTATOR_STRIDE, &rot_stride);
	}

    // Set rotator output
    vpu_DecGiveCommand(dec->handle, SET_ROTATOR_OUTPUT, &dec->fb[0]);

    // Start decoding one frame
    DecOutputInfo outinfo = {0};
    DecParam decparam = {0};
	decparam.dispReorderBuf = 0;
	decparam.skipframeMode = 0;
	decparam.skipframeNum = 0;
    decparam.prescanEnable = 0;                             // not used in the i.MX 6.
    decparam.prescanMode = 0;                               // not used in the i.MX 6.
	decparam.iframeSearchEnable = 0;                        // once enabled, prescanEnable, prescanMode and skipframeMode options are ignored.
    decparam.phyJpgChunkBase = 0;                           // NOTE: physical memory address of input bitstream buffer for Jpg.
    decparam.virtJpgChunkBase = 0;                          // NOTE: virtual memory address of input bitstream buffer for Jpg.
    ret = vpu_DecStartOneFrame(dec->handle, &decparam);
    if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_DecStartOneFrame failed: %d", ret);
        // NOTES: vpu_DecGetOutputInfo must be called after vpu_DecStartOneFrame (even if failed)
        vpu_DecGetOutputInfo(dec->handle, &outinfo);
        return -1;
    }

    // Wait for VPU processing
    while (vpu_IsBusy()) {
        if (vpu_WaitForInt(200) == 0) {
            break;
        }
        LOGW(TAG, "VPU is still busy, wait 300ms again");
        if (vpu_WaitForInt(300) != 0) {
            LOGE(TAG, "VPU process timeout");
            // NOTES: vpu_DecGetOutputInfo must be called after vpu_DecStartOneFrame (even if failed)
            vpu_DecGetOutputInfo(dec->handle, &outinfo);
            return -1;
        }
    }
    return 0;
}


int vpu_dec_get_output(VPUDec* dec, vpu_dec_out* dec_out) {
    // Get decoder output info
    DecOutputInfo outinfo = {0};
    RetCode ret = vpu_DecGetOutputInfo(dec->handle, &outinfo);
    if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_DecGetOutputInfo failed: %d", ret);
        return -1;
    }

    LOGI(TAG, "DecOutputInfo: decodingSuccess=%d, indexFrameDisplay=%d, indexFrameDecoded=%d, "
        "decPicWidth/decPicHeight=%d/%d, decPicCrop::left/top/right/bottom=%u/%u/%u/%u",
        outinfo.decodingSuccess, outinfo.indexFrameDisplay, outinfo.indexFrameDecoded,
        outinfo.decPicWidth, outinfo.decPicHeight, 
        outinfo.decPicCrop.left, outinfo.decPicCrop.top, 
        outinfo.decPicCrop.right, outinfo.decPicCrop.bottom);

    // In 8 instances test, we found some instance(s) may not get a chance to be scheduled
	// until timeout, so we yield schedule each frame explicitly.
	// This may be kernel dependant and may be removed on customer platform */
	usleep(0);

    // We only use I or P frame, indexFrameDecoded should be >= 0 if succeed
    if (outinfo.decodingSuccess != 1 
        || outinfo.indexFrameDecoded < 0 
        || outinfo.indexFrameDecoded >= dec->totalfb) {
        LOGE(TAG, "decoding failed");
        return -1;  
    }

    // Maybe outinfo.decPicCrop need to be checked for H.264         
    int size = outinfo.decPicWidth * outinfo.decPicHeight * 3 / 2;
    if (dec_out->buflen < size) {
        LOGE(TAG, "output buffer length is not avaliable");
        return -1;
    }

    // Fill vpu_dec_out struct
    unsigned long buf = dec->fb[outinfo.indexFrameDecoded].bufY - dec->fbmem[outinfo.indexFrameDecoded].phy_addr 
        + dec->fbmem[outinfo.indexFrameDecoded].virt_uaddr;
    memcpy(dec_out->buf, (void*)buf, size);
    dec_out->buflen = size;
    dec_out->width = outinfo.decPicWidth;
    dec_out->height = outinfo.decPicHeight;
    dec_out->pixfmt = VPU_YUV420P;

    // Clear decoder display flag
    if (dec->config.decfmt != VPU_FMT_JPEG && outinfo.indexFrameDisplay >= 0) {
        ret = vpu_DecClrDispFlag(dec->handle, outinfo.indexFrameDisplay);
        if (ret != RETCODE_SUCCESS) {
            LOGE(TAG, "vpu_DecClrDispFlag failed: %d", ret);
            return -1;
        }
    }
    
    return 0;
}


