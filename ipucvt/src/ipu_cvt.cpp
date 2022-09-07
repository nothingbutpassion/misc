/**
 *******************************************************************************
 *                       Continental Confidential
 *                  Copyright (c) Continental AG. 2016
 *
 *      This software is furnished under license and may be used or
 *      copied only in accordance with the terms of such license.
 *******************************************************************************
 * @file     ipu_cvt.cpp
 * @brief   The IPU converting library API implementation file
 *******************************************************************************
 */
#include <stddef.h>
#include "ipu_utils.h"
#include "ipu_cvt.h"


/**
  * @brief Get IPU error message.
  *
  * @return A pointer to a string that describes the error message of previous ipu_*() calling.
  *
  * @note The returned pointer may be overrided by latter ipu_*() calling. 
  *		 Application should copy the string from the returned pointer for latter reference.
  */
const char* ipu_error() {
    return IPUError::get();
}


/**
  * @brief Open IPU device.
  *
  * @return A IPU device handle if succeed, otherwise NULL is returned.
  */
void* ipu_open() {
    IPUError::reset();
    IPUConverter cvt;
    if (cvt.open() == -1) {
        return NULL;
    }
    return new IPUConverter(cvt);
}


/**
  * @brief Close IPU device.
  *
  * @param handle The IPU device handle returned by ipu_open().
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */	
int ipu_close(void* handle) {
    IPUError::reset();
    if (!handle) {
        IPUError::set("IPU handle is NULL");
        return -1;
    }
    
    IPUConverter* cvt = (IPUConverter*)handle;
    return cvt->close();
}


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
void* ipu_alloc(void* handle, uint32_t size) {
    IPUError::reset();
    if (!handle || !size) {
        IPUError::set("IPU handle is NULL, and/or buffer size is 0");
        return NULL;
    }

    IPUConverter* cvt = (IPUConverter*)handle;
    return cvt->alloc(size);

}


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
int ipu_free(void* handle, void* buf) {
    IPUError::reset();
    if (!handle) {
        IPUError::set("IPU handle is NULL");
        return -1;
    }

    if (!buf) {
        return 0;
    }

    IPUConverter* cvt = (IPUConverter*)handle;
    return cvt->free(buf);
}


/**
  * @brief Do IPU convertion
  *
  * @param handle The IPU device handle returned by ipu_open().
  * @param in 	The input params for IPU convertion
  * @param out 	The output params for IPU convertion
  *
  * @return 0 if succeed, otherwise -1 is returned.
  */
int ipu_convert(void* handle, const ipu_in* in, ipu_out* out) {
    IPUError::reset();
    if (!handle || !in || !out) {
        IPUError::set("IPU handle, and/or input, and/or output is NULL");
        return -1;
    }
    
    IPUConverter* cvt = (IPUConverter*)handle;
    return cvt->convert(in, out);
}

