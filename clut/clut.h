#ifndef __CLUT_H__
#define __CLUT_H__

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

//#define DEBUG_CLUT

#ifdef DEBUG_CLUT
#define LOG(msg ...)	fprintf(stdout, msg)
#define LOGE(msg ...)	fprintf(stderr, msg)
#else
#define LOG(msg ...)
#define LOGE(msg ...)
#endif

using std::string;
using std::vector;
using std::pair;

namespace clut {

inline string format(const char* fmt, ... ) {
    char buf[1 << 16];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    return string(buf);
}

class Exception : public std::exception
{
public:
    Exception() : code(0), line(0) {}
    Exception(int _code, const string& _err, const string& _func, const string& _file, int _line) 
        : code(_code), err(_err), func(_func), file(_file), line(_line) {
        formatMessage();
    }
    virtual ~Exception() throw() {}
    virtual const char* what() const throw() { return msg.c_str(); }
    
    void formatMessage() {
        if (func.size() > 0) {
            msg = format("%s:%d: error: (%d) %s in function %s\n", file.c_str(), line, code, err.c_str(), func.c_str());
        } else {
            msg = format("%s:%d: error: (%d) %s\n", file.c_str(), line, code, err.c_str());
        }
    }
private:
    string msg; 	// the formatted error message
    int code;       // error code
    string err;     // error description
    string func;    // function name. Available only when the compiler supports __func__ macro
    string file;    // source file name where the error has occured
    int line;       // line number in the source file where the error has occured
};

const char* openCLErrorString(int err);

#define openCLSafeCall(expr)  safeCall(expr, __FILE__, __LINE__, __func__)
#define openCLVerifyCall(res) safeCall(res, __FILE__, __LINE__, __func__)

inline void safeCall(int err, const char* file, int line, const char* func) {
    if (CL_SUCCESS != err) {
        throw Exception(err, openCLErrorString(err), func, file, line);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenCL wrapper functions
///////////////////////////////////////////////////////////////////////////////////////////////////

void openCLInit(cl_device_type type = CL_DEVICE_TYPE_GPU);
void openCLDestroy();

void openCLReleaseMemObject(cl_mem mem);

cl_mem openCLCreateBuffer(size_t size, cl_mem_flags flags = CL_MEM_READ_WRITE, void* host = NULL);
void openCLReadBuffer(cl_mem buffer, void* host, size_t size);
void openCLWriteBuffer(cl_mem buffer, const void* host, size_t size);

cl_program openCLCreateProgram(const char* source, size_t size, const char* buildOptions);
cl_program openCLCreateProgram(const char* sourceFile, const char* buildOptions);
void openCLReleaseProgram(cl_program program);

cl_kernel openCLCreateKernel(cl_program program, const char* kernelName);
void openCLReleaseKernel(cl_kernel kernel);

void openCLSetKernelArgs(cl_kernel kernel, const vector< pair<size_t, const void*> >& args);
void openCLExecuteKernel(cl_kernel kernel, size_t globalThreads[3], size_t localThreads[3]);

} // namespace clut

#endif // #define __CLUT_H__
