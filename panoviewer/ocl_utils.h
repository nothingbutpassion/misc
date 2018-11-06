#pragma once

#include <string>
#include <vector>
#include <utility>

using std::string;
using std::vector;

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

//
// Init/Release OpenCL
//
void initOpenCL();
void releaseOpenCL();
// NOTES: must first call releaseOpenCL if setupOpenCL has been called
void attachOpenCL(cl_context context, cl_device_id device_id, cl_command_queue command_queue);

//
// Program & Kernel
//
cl_program buildProgram(const string& kernel_file, const string& build_options);
void releaseProgram(cl_program program);
cl_kernel createKernel(cl_program program, const string& kernel_function);
void releaseKernel(cl_kernel kernel);
void setKernelArgs(cl_kernel kernel, const vector<pair<size_t, const void*>>& args);
void runKernel(cl_kernel kernel, cl_uint dim, size_t* globalsize, size_t* localsize);

//
// Buffer related
//
cl_mem createBuffer(size_t size, void* host_ptr, cl_mem_flags flags);
void releaseBuffer(cl_mem buffer);
void readBuffer(cl_mem buffer, void* host_ptr, size_t size, size_t buffer_offset, bool blocking);
void writeBuffer(cl_mem buffer, const void* host_ptr, size_t size, size_t buffer_offset, bool blocking);
void copyBuffer(cl_mem src_buffer, cl_mem dst_buffer, size_t size, size_t src_offset, size_t dst_offset);
// NOTES: map_flags: CL_MAP_READ or CL_MAP_WRITE and CL_MAP_WRITE_INVALIDATE_REGION are mutually exclusive
void* mapBuffer(cl_mem buffer, cl_map_flags map_flags, size_t size, bool blocking, size_t buffer_offset);
void unmapBuffer(cl_mem buffer, void* ptr);

//
// Command queue
//
void flushQueue();
void finishQueue();

//
// SVM: supported from OpenCL 2.0
//

// NOTES: flags:
// CL_MEM_READ_WRITE or CL_MEM_WRITE_ONLY and CL_MEM_READ_ONLY are mutually exclusive.
// CL_MEM_SVM_FINE_GRAIN_BUFFER: fine-grained allocation.
// CL_MEM_SVM_ATOMICS: atomic support
void* svmAlloc(cl_svm_mem_flags flags, size_t size, cl_uint alignment);

// NOTES: make sure svm_ptr is no longer used by kernel or others
void svmFree(void* svm_ptr);
void svmSafeFree(void* svm_ptr);

// NOTES: map_flags: CL_MAP_READ or CL_MAP_WRITE and CL_MAP_WRITE_INVALIDATE_REGION are mutually exclusive
void svmMap(cl_map_flags map_flags, void *svm_ptr, size_t size, bool blocking);
void svmUnmap(void* svm_ptr);
void svmMemcpy(void* dst_ptr, const void* src_ptr, size_t size, bool blocking);
// NOTES: 
// 1) size must be a multiple of pattern_size.
// 2) The maximum value of pattern_size is the size of the largest integer or floating-point vector data type supported by the OpenCL device
void svmMemFill(void* svm_ptr, const void* pattern, size_t pattern_size, size_t size);
void setKernelArgSVMPointer(cl_kernel kernel, cl_uint arg_index, const void* arg_value);

