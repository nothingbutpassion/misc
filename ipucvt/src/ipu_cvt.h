 /**
  *******************************************************************************
  * 					  Continental Confidential
  * 				 Copyright (c) Continental AG. 2016
  *
  * 	 This software is furnished under license and may be used or
  * 	 copied only in accordance with the terms of such license.
  *******************************************************************************
  * @file	  ipu_cvt.h
  * @brief  The IPU converting library API interface
  *******************************************************************************
  */
#ifndef IPU_CVT_H
#define IPU_CVT_H
#ifdef __cplusplus
extern "C" {
#endif

// This Macro may be defined in <linux/ipu.h>
#ifndef __ASM_ARCH_IPU_H__

#include <stdint.h>

/**
  * @brief IPU pixel format definitions: Four-character-code (FOURCC)
  */
#define fourcc(a, b, c, d)\
	 (((uint32_t)(a)<<0)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))

/**
  * @name IPU Pixel Formats
  *
  * Pixel formats are defined with ASCII FOURCC code. The pixel format codes are the same used by V4L2 API.
  */
  
/*! @name Generic or Raw Data Formats */
/*! @{ */
#define IPU_PIX_FMT_GENERIC fourcc('I', 'P', 'U', '0')	/*!< IPU Generic Data */
#define IPU_PIX_FMT_GENERIC_32 fourcc('I', 'P', 'U', '1')	/*!< IPU Generic Data */
#define IPU_PIX_FMT_GENERIC_16 fourcc('I', 'P', 'U', '2')	/*!< IPU Generic Data */
#define IPU_PIX_FMT_LVDS666 fourcc('L', 'V', 'D', '6')	/*!< IPU Generic Data */
#define IPU_PIX_FMT_LVDS888 fourcc('L', 'V', 'D', '8')	/*!< IPU Generic Data */
/*! @} */
/*! @name RGB Formats */
/*! @{ */
#define IPU_PIX_FMT_RGB332  fourcc('R', 'G', 'B', '1')	/*!<  8  RGB-3-3-2    */
#define IPU_PIX_FMT_RGB555  fourcc('R', 'G', 'B', 'O')	/*!< 16  RGB-5-5-5    */
#define IPU_PIX_FMT_RGB565  fourcc('R', 'G', 'B', 'P')	/*!< 16  RGB-5-6-5   */
#define IPU_PIX_FMT_RGB666  fourcc('R', 'G', 'B', '6')	/*!< 18  RGB-6-6-6    */
#define IPU_PIX_FMT_BGR666  fourcc('B', 'G', 'R', '6')	/*!< 18  BGR-6-6-6    */
#define IPU_PIX_FMT_BGR24   fourcc('B', 'G', 'R', '3')	/*!< 24  BGR-8-8-8    */
#define IPU_PIX_FMT_RGB24   fourcc('R', 'G', 'B', '3')	/*!< 24  RGB-8-8-8    */
#define IPU_PIX_FMT_GBR24   fourcc('G', 'B', 'R', '3')	/*!< 24  GBR-8-8-8    */
#define IPU_PIX_FMT_BGR32   fourcc('B', 'G', 'R', '4')	/*!< 32  BGR-8-8-8-8  */
#define IPU_PIX_FMT_BGRA32  fourcc('B', 'G', 'R', 'A')	/*!< 32  BGR-8-8-8-8  */
#define IPU_PIX_FMT_RGB32   fourcc('R', 'G', 'B', '4')	/*!< 32  RGB-8-8-8-8  */
#define IPU_PIX_FMT_RGBA32  fourcc('R', 'G', 'B', 'A')	/*!< 32  RGB-8-8-8-8  */
#define IPU_PIX_FMT_ABGR32  fourcc('A', 'B', 'G', 'R')	/*!< 32  ABGR-8-8-8-8 */
/*! @} */
/*! @name YUV Interleaved Formats */
/*! @{ */
#define IPU_PIX_FMT_YUYV    fourcc('Y', 'U', 'Y', 'V')	/*!< 16 YUV 4:2:2 */
#define IPU_PIX_FMT_UYVY    fourcc('U', 'Y', 'V', 'Y')	/*!< 16 YUV 4:2:2 */
#define IPU_PIX_FMT_YVYU    fourcc('Y', 'V', 'Y', 'U')  /*!< 16 YVYU 4:2:2 */
#define IPU_PIX_FMT_VYUY    fourcc('V', 'Y', 'U', 'Y')  /*!< 16 VYYU 4:2:2 */
#define IPU_PIX_FMT_Y41P    fourcc('Y', '4', '1', 'P')	/*!< 12 YUV 4:1:1 */
#define IPU_PIX_FMT_YUV444  fourcc('Y', '4', '4', '4')	/*!< 24 YUV 4:4:4 */
#define IPU_PIX_FMT_VYU444  fourcc('V', '4', '4', '4')	/*!< 24 VYU 4:4:4 */
/* two planes -- one Y, one Cb + Cr interleaved  */
#define IPU_PIX_FMT_NV12    fourcc('N', 'V', '1', '2') 	/* 12  Y/CbCr 4:2:0  */
/* two planes -- 12  tiled Y/CbCr 4:2:0  */
#define IPU_PIX_FMT_TILED_NV12    fourcc('T', 'N', 'V', 'P')
#define IPU_PIX_FMT_TILED_NV12F   fourcc('T', 'N', 'V', 'F')
/*! @} */
/*! @name YUV Planar Formats */
/*! @{ */
#define IPU_PIX_FMT_GREY    fourcc('G', 'R', 'E', 'Y')	/*!< 8  Greyscale */
#define IPU_PIX_FMT_YVU410P fourcc('Y', 'V', 'U', '9')	/*!< 9  YVU 4:1:0 */
#define IPU_PIX_FMT_YUV410P fourcc('Y', 'U', 'V', '9')	/*!< 9  YUV 4:1:0 */
#define IPU_PIX_FMT_YVU420P fourcc('Y', 'V', '1', '2')	/*!< 12 YVU 4:2:0 */
#define IPU_PIX_FMT_YUV420P fourcc('I', '4', '2', '0')	/*!< 12 YUV 4:2:0 */
#define IPU_PIX_FMT_YUV420P2 fourcc('Y', 'U', '1', '2')	/*!< 12 YUV 4:2:0 */
#define IPU_PIX_FMT_YVU422P fourcc('Y', 'V', '1', '6')	/*!< 16 YVU 4:2:2 */
#define IPU_PIX_FMT_YUV422P fourcc('4', '2', '2', 'P')	/*!< 16 YUV 4:2:2 */
/* non-interleaved 4:4:4 */
#define IPU_PIX_FMT_YUV444P fourcc('4', '4', '4', 'P')	/*!< 24 YUV 4:4:4 */
/*! @} */


/**
  * @brief IPU rotate mode
  */
enum ipu_rotate_mode {         
	IPU_ROTATE_NONE = 0,			// NO rotation
	IPU_ROTATE_VERT_FLIP,			// Vertical flip
	IPU_ROTATE_HORIZ_FLIP,			// Horizental flip
    IPU_ROTATE_180,					// Rotate 180 degrees
    IPU_ROTATE_90_RIGHT,			// Rotate 90 degrees to right
    IPU_ROTATE_90_RIGHT_VFLIP,		// Rotate 90 degrees to right, and Vertical flip
    IPU_ROTATE_90_RIGHT_HFLIP,		// Rotate 90 degrees to right, and Horizental flip
    IPU_ROTATE_90_LEFT,				// Rotate 90 degrees to left
};


#endif	// end  __ASM_ARCH_IPU_H__


/**
  * @brief  IPU input/output cropping rect.
  */
struct ipu_rect {
	uint32_t x;			// The X coordinate of cropping rect
	uint32_t y;			// The Y coordinate of cropping rect
	uint32_t w;			// Width of cropping rect
	uint32_t h;			// Height of cropping rect
};


/**
  * @brief IPU converting Input. This struct can be exented for future version.
  */
struct ipu_in {
	void*	 buf;				// Input buffer, should be set as the return value of ipu_alloc()
	uint32_t buflen;			// Input buffer length
	uint32_t format;			// Input pixel format, specified by IPU_PIX_FMT_*
	uint32_t width;				// Input picture width
	uint32_t height;			// Input picture height
	ipu_rect crop;				// Input picture cropping rect
};


/**
  * @brief IPU converting Output. This struct can be exented for future version.
  */
struct ipu_out {
	void*	 buf;				// Output buffer, should be set as the return value of ipu_alloc()
	uint32_t buflen;			// Output buffer length
	uint32_t format;			// Output pixel format, specified by IPU_PIX_FMT_*
	uint32_t width;				// Output picture width
	uint32_t height;			// Output picture height
	ipu_rect crop;				// Output picture cropping rect
	uint8_t  rotate;			// Output rotation mode, specified by IPU_ROTATE_*
};


/**
  * @brief Get IPU error message.
  *
  * @return A pointer to a string that describes the error message of the ipu_*().
  *
  * @note The returned pointer may be overrided by latter ipu_*() calling. 
  *		 Application should copy the string from the returned pointer for latter reference.
  */
const char* ipu_error();


/**
  * @brief Open IPU device.
  *
  * @return A IPU device handle if succeed, otherwise NULL is returned.
  */
void* ipu_open();


/**
  * @brief Close IPU device.
  *
  * @param handle The IPU device handle returned by ipu_open().
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */	
int ipu_close(void* handle);


/**
  * @brief Allocate IPU buffer
  *
  * @param handle  The IPU device handle returned by ipu_open().
  * @param size	 The IPU buffer size required.
  *
  * @return A pointer to the allocated buffer if succeed, otherwise NULL is returned.
  *
  * @note  The allocated buffer should be released by ipu_free() if no longer used. 
  */
void* ipu_alloc(void* handle, uint32_t size);


/**
  * @brief Free IPU buffer
  *
  * @param handle	 The IPU device handle returned by ipu_open().
  * @param buf	 The IPU buffer returned by ipu_alloc().   
  *
  * @return 0 if succeed, otherwise -1 is returned.
  *
  * @note If handle is valid and buf is NULL, 0 is retured. 
  */
int ipu_free(void* handle, void* buf);


/**
  * @brief Do IPU converting
  *
  * @param handle The IPU device handle returned by ipu_open().
  * @param in 	The input params for IPU converting
  * @param out 	The output params for IPU converting
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int ipu_convert(void* handle, const ipu_in* in, ipu_out* out);



#ifdef __cplusplus
}		// end extern "C"
#endif
#endif 	// end IPU_CVT_H
