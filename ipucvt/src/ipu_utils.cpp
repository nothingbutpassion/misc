/**
 *******************************************************************************
 *                       Continental Confidential
 *                  Copyright (c) Continental AG. 2016
 *
 *      This software is furnished under license and may be used or
 *      copied only in accordance with the terms of such license.
 *******************************************************************************
 * @file     ipu_utils.cpp
 * @brief   The utils classes implementation file
 *******************************************************************************
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <map>

#include "ipu_utils.h"


char IPUError::errBuffer[IPU_ERROR_BUFLEN] = {'\0'};


void IPUError::set(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(errBuffer, sizeof(errBuffer), fmt, ap);
    va_end(ap);
}

void IPUError::reset() {
    errBuffer[0]='\0';
}

const char* IPUError::get() {
    return errBuffer;
}


IPUConverter::IPUConverter() : fd(-1) {
    memset(&task, 0, sizeof(task));
    task.timeout = IPU_TASK_TIMEOUT;
    task.priority = IPU_TASK_PRIORITY_HIGH;
}

IPUConverter::~IPUConverter() {}


int IPUConverter::open() {
    fd = ::open("/dev/mxc_ipu", O_RDWR);
    if (fd == -1) {
        IPUError::set("open /dev/mxc_ipu failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int IPUConverter::close() {
    int ret = 0;
    for (std::map<void*, DMAMem>::iterator it = allocs.begin(); it != allocs.end(); ++it) {
        void* buf = it->first;
        DMAMem& dmaMem = it->second;
        if (munmap(buf, dmaMem.size) < 0) {
            IPUError::set("munmap failed: %s", strerror(errno));
            ret = -1;
        }
        if (ioctl(fd, IPU_FREE, &dmaMem.addr) < 0) {
            IPUError::set("ioctl IPU_FREE failed: %s", strerror(errno));
            ret = -1;
        }   
    }
    allocs.clear();

    if (::close(fd) < 0) {
       IPUError::set("close /dev/mxc_ipu failed: %s", strerror(errno));
       ret = -1;
    }
    return ret;
}

int IPUConverter::convert(const ipu_in* in, ipu_out* out) {
    std::map<void*, DMAMem>::iterator inAlloc = allocs.find(in->buf);
    std::map<void*, DMAMem>::iterator outAlloc = allocs.find(out->buf);
    if (inAlloc == allocs.end() || outAlloc == allocs.end()) {
        IPUError::set("Can't find DAM alloc for input and/or output buffer");
        return -1;
    }
    
    task.input.paddr = inAlloc->second.addr;
    task.input.format = in->format;
    task.input.width = in->width;
    task.input.height = in->height;
    task.input.crop.pos.x = in->crop.x;
    task.input.crop.pos.y = in->crop.y;
    task.input.crop.w = in->crop.w;
    task.input.crop.h = in->crop.h;
    task.output.paddr = outAlloc->second.addr;
    task.output.format = out->format;
    task.output.width = out->width;
    task.output.height = out->height;
    task.output.rotate = out->rotate;
    task.output.crop.pos.x = out->crop.x;
    task.output.crop.pos.y = out->crop.y;
    task.output.crop.w = out->crop.w;
    task.output.crop.h = out->crop.h;
    int ret = ioctl(fd, IPU_CHECK_TASK, &task);
	if (ret != IPU_CHECK_OK) {
        IPUError::set("ioctl IPU_CHECK_TASK failed: %d", ret);
        return -1;
    }

	ret = ioctl(fd, IPU_QUEUE_TASK, &task);
	if (ret < 0) {
		IPUError::set("ioctl IPU_QUEUE_TASK failed: %d", ret);
		return -1;
	}
    
    return 0;
}

void* IPUConverter::alloc(uint32_t size) {

    DMAMem dmaMem = {(dma_addr_t)size, size};
    if (ioctl(fd, IPU_ALLOC, &dmaMem.addr) < 0) {
		IPUError::set("ioctl IPU_ALLOC failed: %s", strerror(errno));
		return NULL;
	}
    
	void* buf = mmap(0, dmaMem.size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, dmaMem.addr);
	if (!buf) {
		IPUError::set("mmap failed: %s", strerror(errno));
		ioctl(fd, IPU_FREE, &dmaMem.addr);
        return NULL;
	}

    allocs[buf] = dmaMem;
    return buf;
}

int IPUConverter::free(void* buf) {
    std::map<void*, DMAMem>::iterator it = allocs.find(buf);
    if (it == allocs.end()) {
        IPUError::set("Can't find DMA alloc for the released buffer");
        return -1;
    }

    DMAMem& dmaMem = it->second;
    if (munmap(buf, dmaMem.size) < 0) {
        IPUError::set("munmap failed: %s", strerror(errno));
        return -1;
    }
    if (ioctl(fd, IPU_FREE, &dmaMem.addr) < 0) {
        IPUError::set("ioctl IPU_FREE failed: %s", strerror(errno));
        return -1;
    }

    allocs.erase(it);
    return 0;
}


