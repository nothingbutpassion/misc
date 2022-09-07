
extern "C" {
#include <vpu_lib.h>
}

#include "vpu_utils.h"
#include "vpu_codec.h"
#include "vpu_enc_jpeg.h"
#include "vpu_enc_h264.h"
#include "vpu_dec_jpeg.h"
#include "vpu_dec_h264.h"


#define TAG "VPUCodec"


/**
  * @brief Get VPU error message.
  *
  * @return  A pointer to a string that describes the error message of the VPU codec lib.
  *
  * @note The returned pointer may be overrided by latter VPU codec function calling. 
  *		 Application should copy the string from the returned pointer for latter reference.
  */
const char* vpu_error() {
    return vpu_get_error();
}


/**
  * @brief Initialize VPU
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_init() {
    RetCode ret = vpu_Init(NULL);
    if (ret != RETCODE_SUCCESS) {
        LOGE(TAG, "vpu_Init failed: %d", ret);
        return -1;
    }
    
    vpu_set_error("vpu_Init succeed");
    return 0;
}

/**
  * @brief Deinitialize VPU
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_deinit() {
    vpu_UnInit();
    vpu_set_error("vpu_deinit succeed");
    return 0;
}


/**
  * @brief Open VPU encoder
  *
  * @param enc_config	The encoder config.
  *
  * @return A encoder handle if succeed, otherwise NULL is returned.
  */
void* vpu_enc_open(const vpu_enc_config* enc_config) {
    void* enc_handle = NULL;
    if (!enc_config) {
        LOGE(TAG, "encoder config is NULL");
        return enc_handle;
    }

    if (vpu_enc_check(enc_config, NULL, NULL) == -1) {
        return enc_handle;
    }
    
    if (enc_config->encfmt == VPU_FMT_JPEG) {
        enc_handle = vpu_enc_jpeg_open(enc_config);
    } else if (enc_config->encfmt == VPU_FMT_H264) {
        enc_handle = vpu_enc_h264_open(enc_config);
    }

    if (enc_handle) {
        vpu_set_error("vpu_enc_open succeed");
    }
    return enc_handle;
}

/**
  * @brief Close VPU encoder
  *
  * @param enc_handle	The encoder handle returned by vpu_enc_open()
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_enc_close(void* enc_handle) {
    int err = -1;
    if (!enc_handle) {
        LOGE(TAG, "encoder handle is NULL");
        return err;
    }

    VPUEnc* enc = (VPUEnc*)enc_handle;
    if (enc->config.encfmt == VPU_FMT_JPEG) {
        err = vpu_enc_jpeg_close((JPEGEnc*)enc);
    } else if (enc->config.encfmt == VPU_FMT_H264) {
        err = vpu_enc_h264_close((H264Enc*)enc);
    }

    if (err == 0) {
        vpu_set_error("vpu_enc_close succeed");
    }
    return err;   
}

/**
  * @brief Do VPU encoding work.
  *
  * @param enc_handle	The encoder handle returned by vpu_enc_open()
  * @param enc_in 	The encoder Input.
  * @param enc_out	The encoder Output. 
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_encode(void* enc_handle, const vpu_enc_in* enc_in, vpu_enc_out* enc_out) {
    int err = -1;
    if (!enc_handle || !enc_in || !enc_out) {
        LOGE(TAG, "encoder handle, and/or encoder input, and/or encoder output is NULL");
        return err;
    }

    if (vpu_enc_check(NULL, enc_in, enc_out) == -1) {
        return err;
    }
    
    VPUEnc* enc = (VPUEnc*)enc_handle;
    if (enc->config.encfmt == VPU_FMT_JPEG) {
        err = vpu_enc_jpeg_start((JPEGEnc*)enc, enc_in, enc_out);
    }   else if (enc->config.encfmt == VPU_FMT_H264) {
        err = vpu_enc_h264_start((H264Enc*)enc, enc_in, enc_out);
    }

    if (!err) {
        vpu_set_error("vpu_enc_open succeed");
    }

    return err;
}




/**
  * @brief Open VPU decoder.
  *
  * @param dec_config	The encoder config.
  *
  * @return A decoder handle if succeed, otherwise NULL is returned.
  */
void* vpu_dec_open(const vpu_dec_config* dec_config) {
    void* dec_handle = NULL;
    if (!dec_config) {
        LOGE(TAG, "decoder config is null");
        return dec_handle;
    }

    if (vpu_dec_check(dec_config, NULL, NULL) == -1) {
        return dec_handle;
    }

    if (dec_config->decfmt == VPU_FMT_JPEG) {
        dec_handle = vpu_dec_jpeg_open(dec_config);
    } else if (dec_config->decfmt == VPU_FMT_H264) {
        dec_handle = vpu_dec_h264_open(dec_config);
    }

    if (dec_handle) {
        vpu_set_error("vpu_dec_open succeed");  
    }
    return dec_handle;
}

/**
  * @brief Close VPU decoder.
  *
  * @param dec_handle	The decoder handle returned by vpu_dec_open()
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_dec_close(void* dec_handle) {
    int err = -1;
    if (!dec_handle) {
        LOGE(TAG, "decoder handle is null");
        return err;
    }

    VPUDec* dec = (VPUDec*)dec_handle;
    if (dec->config.decfmt == VPU_FMT_JPEG) {
        err = vpu_dec_jpeg_close((JPEGDec*)dec);
    } else if (dec->config.decfmt == VPU_FMT_H264) {
        err = vpu_dec_h264_close((H264Dec*)dec);
    }

    if (!err) {
        vpu_set_error("vpu_dec_close succeed");
    }
    return err;
}

/**
  * @brief Do VPU encoding work.
  *
  * @param dec_handle	The decoder handle returned by vpu_dec_open().
  * @param dec_in		The encoder Input.
  * @param dec_out 	The encoder Output.
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_decode(void* dec_handle, const vpu_dec_in* dec_in, vpu_dec_out* dec_out) {
    int err = -1;
    if (!dec_handle) {
        LOGE(TAG, "decoder handle, and/or encoder input, and/or encoder output is NULL");
        return err;
    }

    if (vpu_dec_check(NULL, dec_in, dec_out) == -1) {
        return err;
    }

    VPUDec* dec = (VPUDec*)dec_handle;
    if (dec->config.decfmt == VPU_FMT_JPEG) {
        err = vpu_dec_jpeg_start((JPEGDec*)dec, dec_in, dec_out);
    }   else if (dec->config.decfmt == VPU_FMT_H264) {
        err = vpu_dec_h264_start((H264Dec*)dec, dec_in, dec_out);
    }
    
    if (!err) {
        vpu_set_error("vpu_dec_close succeed");
    }
    return err;
}


