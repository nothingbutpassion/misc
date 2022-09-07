#include <stdio.h>
#include <string.h>
#include "clut.h"


namespace clut {

const char* openCLErrorString(int err) {
    switch(err) {
    case CL_DEVICE_NOT_FOUND:
        return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:
        return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:
        return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
        return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:
        return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:
        return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
        return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:
        return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:
        return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
        return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:
        return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:
        return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:
        return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
        return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case CL_INVALID_VALUE:
        return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:
        return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:
        return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:
        return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:
        return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:
        return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:
        return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:
        return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:
        return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
        return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:
        return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:
        return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:
        return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:
        return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:
        return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:
        return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:
        return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:
        return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL:
        return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:
        return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:
        return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:
        return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:
        return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:
        return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:
        return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:
        return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:
        return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:
        return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:
        return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:
        return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:
        return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE:
        return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:
        return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE:
        return "CL_INVALID_GLOBAL_WORK_SIZE";
        //case CL_INVALID_PROPERTY:
        //	return "CL_INVALID_PROPERTY";
        //case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR:
        //	return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
        //case CL_PLATFORM_NOT_FOUND_KHR:
        //	return "CL_PLATFORM_NOT_FOUND_KHR";
        //	//case CL_INVALID_PROPERTY_EXT:
        //	//    return "CL_INVALID_PROPERTY_EXT";
        //case CL_DEVICE_PARTITION_FAILED_EXT:
        //	return "CL_DEVICE_PARTITION_FAILED_EXT";
        //case CL_INVALID_PARTITION_COUNT_EXT:
        //	return "CL_INVALID_PARTITION_COUNT_EXT";
        //default:
        //	return "unknown error code";
    }
    static char buf[256];
    sprintf(buf, "%d", err);
    return buf;
}

struct Context {
    bool init;
    cl_platform_id platform;
    cl_device_id device;
    cl_context clContext;
    cl_command_queue clCmdQueue;
    size_t maxWorkGroupSize;
    cl_uint maxDimensions;
    size_t maxWorkItemSizes[16];
    cl_uint maxComputeUnits;
    bool doubleSupport;
    char devName[256];
    char extraOptions[256]; //extra options to recognize vendor specific fp64 extensions
    char binPath[256];
    Context() : 
        init(false), 
        platform(0), 
        device(0), 
        clContext(NULL), 
        clCmdQueue(NULL), 
        maxWorkGroupSize(0), 
        maxDimensions(0),
        maxComputeUnits(0),
        doubleSupport(false) {
        memset(maxWorkItemSizes, 0, sizeof(maxWorkItemSizes));
        memset(devName, 0, sizeof(devName));
        memset(extraOptions, 0, sizeof(extraOptions));
        memset(binPath, 0, sizeof(binPath));
        
    }
};

Context* getContext() {
    static Context ctx;
    return &ctx;
}


void openCLInit(cl_device_type deviceType) {
    
    cl_int status;
    Context* ctx = getContext();
    ctx->init = true;
    
    openCLSafeCall(clGetPlatformIDs(1, &ctx->platform, NULL));
    openCLSafeCall(clGetDeviceIDs(ctx->platform, deviceType, 1, &ctx->device, NULL));
    openCLSafeCall(clGetDeviceInfo(ctx->device, CL_DEVICE_NAME, sizeof(ctx->devName), ctx->devName, NULL));
   
    ctx->clContext = clCreateContext(NULL, 1, &ctx->device, NULL, NULL, &status);
    openCLVerifyCall(status);
    
    ctx->clCmdQueue = clCreateCommandQueue(ctx->clContext, ctx->device, CL_QUEUE_PROFILING_ENABLE, &status);
    openCLVerifyCall(status);

    openCLSafeCall(clGetDeviceInfo(ctx->device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), (void*)&ctx->maxWorkGroupSize, NULL));
    openCLSafeCall(clGetDeviceInfo(ctx->device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), (void*)&ctx->maxDimensions, NULL));
    openCLSafeCall(clGetDeviceInfo(ctx->device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(ctx->maxWorkItemSizes), (void*)ctx->maxWorkItemSizes, NULL));
    openCLSafeCall(clGetDeviceInfo(ctx->device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), (void*)&ctx->maxComputeUnits, NULL));
    openCLSafeCall(clGetDeviceInfo(ctx->device, CL_DEVICE_EXTENSIONS, sizeof(ctx->extraOptions), (void*)ctx->extraOptions, NULL));

    if(string(ctx->extraOptions).find("cl_khr_fp64") != string::npos) {
        sprintf(ctx->extraOptions, "-DDOUBLE_SUPPORT");
        ctx->doubleSupport = true;
    }
#ifdef DEBUG_CLUT
    LOG("OpenCL initialize infos:\n");
    LOG("platform=%d, device=%d, clContext=%p, clCmdQueue=%p, maxWorkGroupSize=%d, maxComputeUnits=%d, maxDimensions=%d\n",  
           (int)ctx->platform, (int)ctx->device, ctx->clContext, ctx->clCmdQueue, ctx->maxWorkGroupSize, ctx->maxComputeUnits, ctx->maxDimensions);
    for(int i=0; i < ctx->maxDimensions; i++) {
        LOG("maxWorkItemSizes[%d]=%d, ", i, ctx->maxWorkItemSizes[i]); 
    }
    LOG("extraOptions=%s\n", ctx->extraOptions);
#endif
}
           

void openCLDestroy() {
    Context* ctx = getContext();
    if (ctx->init) {
        openCLSafeCall(clReleaseCommandQueue(ctx->clCmdQueue));
        openCLSafeCall(clReleaseContext(ctx->clContext));
    }
}


void openCLReleaseMemObject(cl_mem mem) {
    openCLSafeCall(clReleaseMemObject(mem));
}

cl_mem openCLCreateBuffer(size_t size, cl_mem_flags flags, void* host) {
    cl_int status;
    Context* ctx = getContext();
    cl_mem buffer = clCreateBuffer(ctx->clContext, flags, size, host, &status);
    openCLVerifyCall(status);
    return buffer;
}

void openCLReadBuffer(cl_mem buffer, void* host, size_t size)
{
    Context* ctx = getContext();
    openCLSafeCall(clEnqueueReadBuffer(ctx->clCmdQueue, buffer, CL_TRUE, 0, size, host, 0, NULL, NULL));
}

void openCLWriteBuffer(cl_mem buffer, const void* host, size_t size)
{
    Context* ctx = getContext();
    openCLSafeCall(clEnqueueWriteBuffer(ctx->clCmdQueue, buffer, CL_TRUE, 0, size, host, 0, NULL, NULL));
}

cl_program openCLCreateProgram(const char* source, size_t size, const char* buildOptions)
{
    cl_int status;
    Context* ctx = getContext();
    
    cl_program program = clCreateProgramWithSource(ctx->clContext, 1, &source, &size, &status);
    openCLVerifyCall(status);
    
    status = clBuildProgram(program, 1, &ctx->device, buildOptions, NULL, NULL);
    if (status != CL_SUCCESS) {
        char log[1024] = {0};
		clGetProgramBuildInfo(program, ctx->device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
		LOGE("Building logs: \n%s\n", log);
		clReleaseProgram(program);
    }
    openCLVerifyCall(status);
    return program;
}


cl_program openCLCreateProgram(const char* sourceFile, const char* buildOptions) {
    cl_program program = NULL;
    FILE* fp = fopen(sourceFile, "r");
    if (!fp) {
        LOGE("Can't open %s\n", sourceFile);
        return program;
    }
    
    fseek(fp, 0L, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    
    char* source = new char[size];
    size = fread(source, 1, size, fp);
    fclose(fp);
    
    program = openCLCreateProgram(source, size, buildOptions);
    
    delete source;
    return program;
}


void openCLReleaseProgram(cl_program program) {
    openCLSafeCall(clReleaseProgram(program));
}

cl_kernel openCLCreateKernel(cl_program program, const char* kernelName) {
    cl_int status;
    cl_kernel kernel = clCreateKernel(program, kernelName, &status);
    openCLVerifyCall(status);
    return kernel;
}

void openCLReleaseKernel(cl_kernel kernel) {
    return openCLSafeCall(clReleaseKernel(kernel));
}

void openCLSetKernelArgs(cl_kernel kernel, const vector< pair<size_t, const void*> >& args) {
    for(size_t i = 0; i < args.size(); i ++) {
        openCLSafeCall(clSetKernelArg(kernel, i, args[i].first, args[i].second));
    }
}

void openCLExecuteKernel(cl_kernel kernel, size_t globalThreads[3], size_t localThreads[3]) {
    Context* ctx = getContext();
#ifndef DEBUG_CLUT
    openCLSafeCall(clEnqueueNDRangeKernel(ctx->clCmdQueue, kernel, 3, NULL, globalThreads, localThreads, 0, NULL, NULL));
#else
    cl_event event = NULL;
    cl_ulong startTime;
    cl_ulong endTime;
    cl_ulong queueTime;
    openCLSafeCall(clEnqueueNDRangeKernel(ctx->clCmdQueue, kernel, 3, NULL, globalThreads, localThreads, 0, NULL, &event));
    openCLSafeCall(clWaitForEvents(1, &event));
    openCLSafeCall(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &startTime, 0));
    openCLSafeCall(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &endTime, 0));
    openCLSafeCall(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queueTime, 0));                       
    clReleaseEvent(event);                          
    LOG("Kernel execute infos:  executeTime=%fms, totalTime=%fms\n", 
        double(endTime-startTime)/1000000, double(endTime - queueTime)/1000000);                               
#endif
}

} // namespace clut
