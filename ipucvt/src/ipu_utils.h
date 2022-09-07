/**
 *******************************************************************************
 *                       Continental Confidential
 *                  Copyright (c) Continental AG. 2016
 *
 *      This software is furnished under license and may be used or
 *      copied only in accordance with the terms of such license.
 *******************************************************************************
 * @file     ipu_utils.h
 * @brief   The utils classes header file
 *******************************************************************************
 */
#ifndef IPU_UTILS_H
#define IPU_UTILS_H

#include <linux/ipu.h>
#include <map>
#include "ipu_cvt.h"



#define IPU_ERROR_BUFLEN 	64
#define IPU_TASK_TIMEOUT 	1000


struct IPUError {
	static void set(const char *fmt, ...);
	static void reset();
	static const char* get();
private:
	static char errBuffer[IPU_ERROR_BUFLEN];
};


struct IPUConverter {

	IPUConverter();
	~IPUConverter();
	
	int open();
	int close();
	int convert(const ipu_in* in, ipu_out* out);
	void* alloc(uint32_t size);
	int free(void* buf);
	
private:

	// DAM memory
	struct DMAMem {
		dma_addr_t	addr;				// DMA memory address
		uint32_t	size;				// DMA memory size
	};

	std::map<void*, DMAMem>	 allocs;	// All  allocated buffers
	int 		fd;						// IPU File Desciptor
	ipu_task 	task;					// IPU task
};


#endif // end IPU_UTILS_H