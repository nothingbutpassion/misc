#ifndef VPU_CODEC_H
#define VPU_CODEC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
  * @brief VPU codec format. Currently only support encoding/decoding YUV420p to/from H.264/JPEG.
  */
enum vpu_codec_format {
	VPU_FMT_H264 = 2,			// For H.264 encoder/decoder
	VPU_FMT_JPEG = 7,			// For JPEG encoder/decoder
	VPU_FMT_VP8 = 9				// Not supported in current version
};

/**
  * @brief VPU pixel format. Currently only support YUV420p.
  */
enum vpu_pixel_format {
	VPU_YUV420P
};

/**
  * @brief VPU codec frame. Used for H.264 codec. 
  */
enum vpu_codec_frame {
	VPU_I_FRAME,				// I frame
	VPU_P_FRAME,				// P frame
};



/**
  * @brief VPU encoder config info. This struct can be extended for future version.
  */
struct vpu_enc_config {
	vpu_codec_format encfmt;	// Encoder format: 2 - H.264, 7 - JPEG
	vpu_pixel_format pixfmt;	// Pixel formt of the raw image. Currently, only YUV420P is supported 
	size_t width;				// The raw image width
	size_t height;				// The raw image height

	// 
	// More detailed options for encoder.
	//							 
	uint32_t quality;			// For JPEG, it is 1~100 (1-worst, 100-best), 0 is auto.  For H.264, it is the bitrate (Unit: kbps), 0 is auto
	uint16_t gop_size;			// Group Of Picture size: 0 = only first picture is I, 1 = all I frames, 2 = IPIP, 3 = IPPIPP, and so on. Ignored for JPEG.
};

/**
  * @brief VPU encoder Input. This struct can be exented for future version.
  */
struct vpu_enc_in {
	char* buf;					// The raw (YUV) image buffer
	size_t buflen;				// The length of buf.
};

/**
  * @brief VPU encoder Output. This struct can be exented for future version.
  */
struct vpu_enc_out {
	char* buf;					// The encoded (H.264 or JPEG) image buffer
	size_t buflen;				// The length of buf.
	vpu_codec_frame	frame;		// The encoded image type: 0 - I frame, 1 -P frame.	
};



/**
  * @brief VPU decoder config. This struct can be extended for future version.
  * 
  */
struct vpu_dec_config {
	vpu_codec_format decfmt;		// Decoder format: 2 - H.264, 7 - JPEG
};

/**
  * @brief VPU decoder Input. This struct can be exented for future version.
  */
struct vpu_dec_in  {
	char* buf;					// The encoded (H.264 or JPEG) image buffer.
	size_t buflen;				// The length of buf.
};

/**
  * @brief VPU decoder Input. This struct can be exented for future version.
  */
struct vpu_dec_out {
	char* buf;					// The raw (YUV) image buffer
	size_t buflen;				// The length of buf
	size_t width;				// The raw image width
	size_t height;				// The raw image height
	vpu_pixel_format pixfmt;	// Pixel format of the raw image. 
};


/**
  * @brief Get VPU error message.
  *
  * @return  A pointer to a string that describes the error message of the VPU codec lib.
  *
  * @note The returned pointer may be overrided by latter VPU codec function calling. 
  *		 Application should copy the string from the returned pointer for latter reference.
  */
const char* vpu_error();


/**
  * @brief Initialize VPU
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_init();

/**
  * @brief Deinitialize VPU
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_deinit();


/**
  * @brief Open VPU encoder
  *
  * @param enc_config	The encoder config.
  *
  * @return A encoder handle if succeed, otherwise NULL is returned.
  */
void* vpu_enc_open(const vpu_enc_config* enc_config);

/**
  * @brief Close VPU encoder
  *
  * @param enc_handle	The encoder handle returned by vpu_enc_open()
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_enc_close(void* enc_handle);

/**
  * @brief Do VPU encoding work.
  *
  * @param enc_handle	The encoder handle returned by vpu_enc_open()
  * @param enc_in 	The encoder Input.
  * @param enc_out	The encoder Output. 
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_encode(void* enc_handle, const vpu_enc_in* enc_in, vpu_enc_out* enc_out);




/**
  * @brief Open VPU decoder.
  *
  * @param dec_config	The encoder config.
  *
  * @return A decoder handle if succeed, otherwise NULL is returned.
  */
void* vpu_dec_open(const vpu_dec_config* dec_config);

/**
  * @brief Close VPU decoder.
  *
  * @param dec_handle	The decoder handle returned by vpu_dec_open()
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_dec_close(void* dec_handle);

/**
  * @brief Do VPU encoding work.
  *
  * @param dec_handle	The decoder handle returned by vpu_dec_open().
  * @param dec_in		The encoder Input.
  * @param dec_out 	The encoder Output.
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int vpu_decode(void* dec_handle, const vpu_dec_in* dec_in, vpu_dec_out* dec_out);



#ifdef __cplusplus
}		// end extern "C"
#endif
#endif 	// end VPU_CODEC_H
