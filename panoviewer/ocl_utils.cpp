#undef NDEBUG

#include <assert.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "ocl_utils.h"

using namespace std;


struct Context {
	static Context& instance() {
		static Context context;
		return context;
	}

	cl_device_id device_id = NULL;;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
};
//
// Init/Release OpenCL
//
void initOpenCL() {
	cl_int status;
	cl_platform_id platform_id;
	
	status = clGetPlatformIDs(1, &platform_id, NULL);
	assert(status == CL_SUCCESS);
	
	cl_device_id device_id;
	status = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
	assert(status == CL_SUCCESS);

	cl_context_properties context_properties[] = {
		CL_CONTEXT_PLATFORM, 
		(cl_context_properties)platform_id,
		0
	};
	cl_context context = clCreateContext(context_properties, 1, &device_id, NULL, NULL, &status);
	assert(status == CL_SUCCESS);

	// NOTES: clCreateCommandQueue is deprecated is OpenCL 2.0
	cl_command_queue command_queue = clCreateCommandQueueWithProperties(context, device_id, NULL, &status);
	assert(status == CL_SUCCESS);

	Context& c = Context::instance();
	c.device_id = device_id;
	c.context = context;
	c.command_queue = command_queue;
}
void releaseOpenCL() {
	cl_int status;
	Context& c = Context::instance();
	status = clReleaseCommandQueue(c.command_queue);
	assert(status == CL_SUCCESS);
	status = clReleaseContext(c.context);
	assert(status == CL_SUCCESS);
	status = clReleaseDevice(c.device_id);
	assert(status == CL_SUCCESS);
}
// NOTES: must first call releaseOpenCL if setupOpenCL has been called
void attachOpenCL(cl_context context, cl_device_id device_id, cl_command_queue command_queue) {
	Context& c = Context::instance();
	c.device_id = device_id;
	c.context = context;
	c.command_queue = command_queue;
}

//
// Program & Kernel
//
cl_program buildProgram(const string& kernel_file, const string& build_options) {
	cl_int status;
	std::ifstream ifs(kernel_file);
	std::string program_str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	const char* src = program_str.c_str();
	size_t src_size = program_str.size();
	Context& c = Context::instance();
	cl_program program = clCreateProgramWithSource(c.context, 1, &src, &src_size, &status);
	assert(status == CL_SUCCESS);

	status = clBuildProgram(program, 1, &c.device_id, build_options.c_str(), NULL, NULL);
	assert(status == CL_SUCCESS);
	return program;
}
void releaseProgram(cl_program program) {
	cl_int status = clReleaseProgram(program);
	assert(status == CL_SUCCESS);
}
cl_kernel createKernel(cl_program program, const string& kernel_function) {
	cl_int status;
	cl_kernel kernel = clCreateKernel(program, kernel_function.c_str(), &status);
	assert(status == CL_SUCCESS);
	return kernel;
}
void releaseKernel(cl_kernel kernel) {
	cl_int status = clReleaseKernel(kernel);
	assert(status == CL_SUCCESS);
}

void setKernelArgs(cl_kernel kernel, const vector<pair<size_t, const void*>>& args) {
	cl_int status;
	for (cl_uint i = 0; i < args.size(); ++i) {
		status = clSetKernelArg(kernel, i, args[i].first, args[i].second);
		assert(status == CL_SUCCESS);
	}
}
void runKernel(cl_kernel kernel, cl_uint dim, size_t* globalsize, size_t* localsize) {
	Context& c = Context::instance();
	cl_int status = clEnqueueNDRangeKernel(c.command_queue, kernel, dim, NULL, globalsize, localsize, 0, NULL, NULL);
	assert(status == CL_SUCCESS);
}

//
// Buffer related
//
cl_mem createBuffer(size_t size, void* host_ptr, cl_mem_flags flags) {
	Context& c = Context::instance();
	cl_int status;
	cl_mem mem = clCreateBuffer(c.context, flags, size, host_ptr, &status);
	assert(status == CL_SUCCESS);
	return mem;
}
void releaseBuffer(cl_mem buffer) {
	cl_int status = clReleaseMemObject(buffer);
	assert(status == CL_SUCCESS);
}
void readBuffer(cl_mem buffer, void* host_ptr, size_t size, size_t buffer_offset, bool blocking) {
	Context& c = Context::instance();
	cl_int status = clEnqueueReadBuffer(c.command_queue, buffer, (blocking ? CL_TRUE : CL_FALSE), 
		buffer_offset, size, host_ptr, 0, NULL, NULL);
	assert(status == CL_SUCCESS);
}
void writeBuffer(cl_mem buffer, const void* host_ptr, size_t size, size_t buffer_offset, bool blocking) {
	Context& c = Context::instance();
	cl_int status = clEnqueueWriteBuffer(c.command_queue, buffer, (blocking ? CL_TRUE : CL_FALSE), 
		buffer_offset, size, host_ptr, 0, NULL, NULL);
	assert(status == CL_SUCCESS);
}
void copyBuffer(cl_mem src_buffer, cl_mem dst_buffer, size_t size, size_t src_offset, size_t dst_offset) {
	Context& c = Context::instance();
	cl_int status = clEnqueueCopyBuffer(c.command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, 0, NULL, NULL);
	assert(status == CL_SUCCESS);
}
// NOTES: map_flags: CL_MAP_READ or CL_MAP_WRITE and CL_MAP_WRITE_INVALIDATE_REGION are mutually exclusive
void* mapBuffer(cl_mem buffer, cl_map_flags map_flags, size_t size, bool blocking, size_t buffer_offset) {
	cl_int status;
	Context& c = Context::instance();
	void* ptr = clEnqueueMapBuffer(c.command_queue, buffer, (blocking ? CL_TRUE : CL_FALSE), 
		map_flags, buffer_offset, size, 0, NULL, NULL, &status);
	return ptr;
}
void unmapBuffer(cl_mem buffer, void* ptr) {
	Context& c = Context::instance();
	cl_int status = clEnqueueUnmapMemObject(c.command_queue, buffer, ptr, 0, NULL, NULL);
	assert(status == CL_SUCCESS);
}

//
// Command queue
//
void flushQueue() {
	Context& c = Context::instance();
	cl_int status = clFlush(c.command_queue);
	assert(status == CL_SUCCESS);
}
void finishQueue() {
	Context& c = Context::instance();
	cl_int status = clFinish(c.command_queue);
	assert(status == CL_SUCCESS);
}

//
// SVM: supported from OpenCL 2.0
//

// NOTES: flags:
// CL_MEM_READ_WRITE or CL_MEM_WRITE_ONLY and CL_MEM_READ_ONLY are mutually exclusive.
// CL_MEM_SVM_FINE_GRAIN_BUFFER: fine-grained allocation.
// CL_MEM_SVM_ATOMICS: atomic support
void* svmAlloc(cl_svm_mem_flags flags, size_t size, cl_uint alignment) {
	Context& c = Context::instance();
	void* svm_ptr = clSVMAlloc(c.context, flags, size, alignment);
	assert(svm_ptr != NULL);
	return svm_ptr;
}

// NOTES: make sure svm_ptr is no longer used by kernel or others
void svmFree(void* svm_ptr) {
	Context& c = Context::instance();
	clSVMFree(c.context, svm_ptr);
}
void svmSafeFree(void* svm_ptr) {
	Context& c = Context::instance();
	clEnqueueSVMFree(c.command_queue, 1, &svm_ptr, NULL, NULL, 0, NULL, NULL);
}

// NOTES: map_flags: CL_MAP_READ or CL_MAP_WRITE and CL_MAP_WRITE_INVALIDATE_REGION are mutually exclusive
void svmMap(cl_map_flags map_flags, void *svm_ptr, size_t size, bool blocking) {
	Context& c = Context::instance();
	cl_int status = clEnqueueSVMMap(c.command_queue, (blocking ? CL_TRUE : CL_FALSE), map_flags, svm_ptr, size, 0, NULL, NULL);
	assert(status == CL_SUCCESS);
}
void svmUnmap(void* svm_ptr) {
	Context& c = Context::instance();
	cl_int status = clEnqueueSVMUnmap(c.command_queue, svm_ptr, 0, NULL, NULL);
	assert(status == CL_SUCCESS);
}
void svmMemcpy(void* dst_ptr, const void* src_ptr, size_t size, bool blocking) {
	Context& c = Context::instance();
	cl_int status = clEnqueueSVMMemcpy(c.command_queue, (blocking ? CL_TRUE : CL_FALSE), dst_ptr, src_ptr, size, 0, NULL, NULL);
	assert(status == CL_SUCCESS);
}
// NOTES: 
// 1) size must be a multiple of pattern_size.
// 2) The maximum value of pattern_size is the size of the largest integer or floating-point vector data type supported by the OpenCL device
void svmMemFill(void* svm_ptr, const void* pattern, size_t pattern_size, size_t size) {
	Context& c = Context::instance();
	cl_int status = clEnqueueSVMMemFill(c.command_queue, svm_ptr, pattern, pattern_size, size, 0, NULL, NULL);
	assert(status == CL_SUCCESS);
}
void setKernelArgSVMPointer(cl_kernel kernel, cl_uint arg_index, const void* arg_value) {
	cl_int status = clSetKernelArgSVMPointer(kernel, arg_index, arg_value);
	assert(status == CL_SUCCESS);
}



