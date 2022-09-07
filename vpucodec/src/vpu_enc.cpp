#include <stdlib.h>
#include <string.h>
#include "vpu_enc.h"
#include "vpu_utils.h"
#include "vpu_fb.h"

#define TAG "VPUEnc"


int vpu_enc_check(const vpu_enc_config* config, const vpu_enc_in* in, vpu_enc_out* out) {
    // Check encoder config
    if (config) {
        if (config->encfmt != VPU_FMT_JPEG && config->encfmt != VPU_FMT_H264) {
            LOGE(TAG, "vpu_enc_config::encfmt is %d (not supported)", config->encfmt);
            return -1;
        }

        if (config->pixfmt!= VPU_YUV420P) {
            LOGE(TAG, "vpu_enc_config::pixfmt is %d (not supported)", config->pixfmt);
            return -1;
        }

        if (config->width == 0 || config->height == 0 ) {
            LOGE(TAG, "vpu_enc_config::width/height is %d/%d (invalid value)", config->width, config->height);
            return -1;
        }

        if (config->encfmt == VPU_FMT_H264 && 
            (config->width % 16 != 0 || config->height % 16 != 0)) {
            LOGE(TAG, "vpu_enc_config::width/height is %d/%d (not a multiple of 16)", config->width, config->height);
            return -1;  
        }

    }

    // Check encoder input
    if (in) {
        if (!in->buf) {
            LOGE(TAG, "vpu_enc_in::buf is NULL");
            return -1;
        }

        if (in->buflen == 0) {
            LOGE(TAG, "vpu_enc_in::buflen is 0");
            return -1;
        }
    }

    // Check encoder output
    if (out) {
        if (!out->buf) {
            LOGE(TAG, "vpu_enc_out::buf is NULL");
            return -1;
        }

        if (out->buflen == 0) {
            LOGE(TAG, "vpu_enc_out::buflen is 0");
            return -1;
        } 
    }
    
    return 0;
}



int vpu_enc_fb_alloc(VPUEnc* enc) {
    // Get initial info 
    EncInitialInfo initinfo = {0};
    RetCode	ret = vpu_EncGetInitialInfo(enc->handle, &initinfo);
	if (ret != RETCODE_SUCCESS) {
		LOGE(TAG, "vpu_EncGetInitialInfo failed: %d", ret);
		return -1;
	}
    
    // Calculate frame buffer info
	int minfbcount = initinfo.minFrameBufferCount;
    int extrafbcount = enc->config.encfmt == VPU_FMT_JPEG ? 0 : 2;
    int srcfbcount = 1;
    int stride = (enc->config.width + 15) & ~15;
	int height = (enc->config.height + 15) & ~15;
    enc->totalfb = minfbcount + extrafbcount + srcfbcount;
    enc->srcfb = enc->totalfb - 1;

    // Allocate frame buffer
    int err = 0;
    for (int i = 0; i < enc->totalfb; i++) {
        enc->fb[i].myIndex = i;
        err |= vpu_fb_alloc(&enc->fbmem[i], enc->config.pixfmt, stride, height, 0, &enc->fb[i]);
    }
    if (err) {
        vpu_enc_fb_free(enc);
        return -1;
    }

    // Register framebuffer
    PhysicalAddress subSampBaseA = enc->config.encfmt != VPU_FMT_JPEG ? enc->fb[minfbcount].bufY : 0;
    PhysicalAddress subSampBaseB = enc->config.encfmt != VPU_FMT_JPEG ? enc->fb[minfbcount + 1].bufY : 0;
    EncExtBufInfo extbufinfo = {0};
    ret = vpu_EncRegisterFrameBuffer(enc->handle, enc->fb, minfbcount, stride, stride,
        subSampBaseA, subSampBaseB, &extbufinfo);
    if (ret != RETCODE_SUCCESS) {
		LOGE(TAG, "vpu_EncRegisterFrameBuffer failed: %d", ret);
        vpu_enc_fb_free(enc);
		return -1;
    }

//    LOGI(TAG, "totalfb=%d, srcfb=%d, minfbcount=%d, extrafbcount=%d, stride=%d, height=%d",  
//        enc->totalfb, enc->srcfb, minfbcount, extrafbcount, stride, height);
    return 0;
}


void vpu_enc_fb_free(VPUEnc* enc) {
    for (int i = 0; i < enc->totalfb; i++) {
        vpu_fb_free(&enc->fbmem[i], &enc->fb[i]);
    }
    enc->totalfb = 0;
    enc->srcfb = 0;
}


int vpu_enc_fb_fill(VPUEnc* enc, const vpu_enc_in* enc_in) {
    // NOTE: pixfmt is always VPU_YUV420P which is MODE420
    int format = enc->config.pixfmt;
    int divX = (format == MODE420 || format == MODE422) ? 2 : 1;
	int divY = (format == MODE420 || format == MODE224) ? 2 : 1;

	int y_size = enc->config.width * enc->config.height;
	int c_size = y_size / divX / divY;
	int img_size = y_size + c_size * 2;

    FrameBuffer* fb = &enc->fb[enc->srcfb];
    vpu_mem_desc* mem = &enc->fbmem[enc->srcfb];
	unsigned long y_addr = fb->bufY  + mem->virt_uaddr - mem->phy_addr;
	unsigned long u_addr = fb->bufCb + mem->virt_uaddr - mem->phy_addr;
	unsigned long v_addr = fb->bufCr + mem->virt_uaddr - mem->phy_addr;

    if (enc_in->buflen < img_size) {
        LOGE(TAG, "input buffer length is not enough");
        return -1;
    }

	if (enc_in->buflen == mem->size) {
        memcpy((void *)y_addr, enc_in->buf, enc_in->buflen);
	} else {
        memcpy((void *)y_addr, enc_in->buf, y_size);
        // NOTES: if NOTE: EncOpenParam::chromaInterleave is 1 when vpu_EncOpen, we should use
        //            memcpy((void *)u_addr, enc_in->buf + y_size, c_size * 2); 
        memcpy((void *)u_addr, enc_in->buf + y_size, c_size);
        memcpy((void *)v_addr, enc_in->buf + y_size + c_size, c_size);
	}
	return 0;
}



int vpu_enc_bs_alloc(VPUEnc* enc) {
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

    enc->bitstream = mem;
    return 0;
}

void vpu_enc_bs_free(VPUEnc* enc) {
    if (enc->bitstream.size > 0) {
        IOFreeVirtMem(&enc->bitstream);
        IOFreePhyMem(&enc->bitstream);
        memset(&enc->bitstream, 0, sizeof(vpu_mem_desc));
    }
}


int vpu_enc_fill_header(VPUEnc* enc, vpu_enc_out* enc_out, int* filled) {
    
    if (enc->config.encfmt == VPU_FMT_H264) {
        // Fill H264 SPS_RBSP header
        EncHeaderParam header_param = {0};
        int filled_size = 0;
        header_param.headerType = SPS_RBSP;
        vpu_EncGiveCommand(enc->handle, ENC_PUT_AVC_HEADER, &header_param);
        unsigned buf  = enc->bitstream.virt_uaddr + header_param.buf - enc->bitstream.phy_addr;
        memcpy(enc_out->buf, (void*)buf, header_param.size);
        filled_size += header_param.size;
        LOGD(TAG, "generate h264 SPS_RBSP header: %d bytes", header_param.size);

        // File H264 PPS_RBSP header
        header_param.headerType = PPS_RBSP;
        vpu_EncGiveCommand(enc->handle, ENC_PUT_AVC_HEADER, &header_param);
        buf = enc->bitstream.virt_uaddr + header_param.buf - enc->bitstream.phy_addr;
        memcpy(enc_out->buf + filled_size, (void*)buf, header_param.size);
        filled_size += header_param.size;
        LOGD(TAG, "generate h264 PPS_RBSP header: %d bytes", header_param.size);
        
        if (filled) {
            *filled = filled_size;
        }
    } else if (enc->config.encfmt == VPU_FMT_JPEG) {
        // Get JPEG header
        EncParamSet paramset = {0};
        paramset.size = STREAM_BUF_SIZE;
        paramset.pParaSet = (Uint8*)malloc(STREAM_BUF_SIZE);
        if (!paramset.pParaSet) {
            LOGE(TAG, "malloc failed");
            return -1;
        }         	
        vpu_EncGiveCommand(enc->handle, ENC_GET_JPEG_HEADER, &paramset);
        

        // Fill JPEG header
        memcpy(enc_out->buf, (void*)paramset.pParaSet, paramset.size);
        free(paramset.pParaSet);
        if (filled) {
            *filled = paramset.size;
            LOGD(TAG, "generate JPEG header: %d bytes", *filled);
        }
    }
	return 0;
}


int vpu_enc_fill_body(VPUEnc* enc, vpu_enc_out* enc_out, int offset) {

    EncOutputInfo outinfo = {0};
    RetCode ret = vpu_EncGetOutputInfo(enc->handle, &outinfo);
    if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_EncGetOutputInfo failed: %d", ret);
        return -1;
    }

    // Fill vpu_enc_out
    unsigned long bitstream = outinfo.bitstreamBuffer + enc->bitstream.virt_uaddr - enc->bitstream.phy_addr;
    enc_out->frame = outinfo.picType == 0 ? VPU_I_FRAME : VPU_P_FRAME;
    if (enc->config.encfmt == VPU_FMT_H264 && enc_out->frame != VPU_I_FRAME) {
        memcpy(enc_out->buf, (void*)bitstream, outinfo.bitstreamSize);
        enc_out->buflen = outinfo.bitstreamSize;
    } else {
        memcpy(enc_out->buf + offset, (void*)bitstream, outinfo.bitstreamSize);
        enc_out->buflen = offset + outinfo.bitstreamSize;
    }
    
    LOGD(TAG, "EncOutputInfo: bitstreamSize=%u, picType=%d", outinfo.bitstreamSize, outinfo.picType);
    return 0;
}


int vpu_enc_start_frame(VPUEnc* enc) {
    // Start one frame
    EncParam  enc_param = {0};
	enc_param.sourceFrame = &enc->fb[enc->srcfb];
	enc_param.quantParam = 23;
	enc_param.forceIPicture = 0;
	enc_param.skipPicture = 0;
	enc_param.enableAutoSkip = 1;
	enc_param.encLeftOffset = 0;
	enc_param.encTopOffset = 0;
    RetCode ret = vpu_EncStartOneFrame(enc->handle, &enc_param);
    if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_EncStartOneFrame failed: %d", ret);
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
            return -1;
        }
    }
    return 0;
}



