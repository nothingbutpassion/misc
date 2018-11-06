#include <iostream>
#include "ocl_utils.h"

using namespace std;

// test SVM features
void openCLSVM() {
	initOpenCL();
	void* svm_ptr = svmAlloc(CL_MEM_READ_WRITE, 32);

	cl_uchar host_ptr[32] = { 0 };
	for (int i = 0; i < 32; i++) {
		host_ptr[i] = '0' + i;
	}
	svmMemcpy(svm_ptr, host_ptr, 32);

	cl_uchar pattern = 'Z';
	svmMemFill(svm_ptr, &pattern, sizeof(cl_uchar), 16);

	svmMap(CL_MAP_READ, svm_ptr, 32);
	cl_uchar* svm = (cl_uchar*)svm_ptr;
	for (int i = 0; i < 32; i++) {
		cout << svm[i] << endl;
	}
	svmUnmap(svm_ptr);
	// NOTES: finishQueue() must be called before svmFree(), or use svmSafeFree() instead.
	finishQueue();

	svmFree(svm_ptr);
	releaseOpenCL();
}


// invoke method: openCLSum("sum.cl");
void openCLSum(const string& kernel_file) {
	initOpenCL();

	cl_program program = buildProgram(kernel_file);
	cl_kernel kernel = createKernel(program, "sum");

	constexpr int src_size = 256;
	constexpr int sum_size = 4;
	double src[src_size] = { 0 };
	double sum[sum_size] = { 0 };
	for (int i = 0; i < src_size; ++i) {
		src[i] = i + 1;
	}

	// NOTES: alternative:
	//cl_mem src_buffer = createBuffer(sizeof(src));
	//writeBuffer(src_buffer, src, sizeof(src));
	cl_mem src_buffer = createBuffer(sizeof(src), src, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE);
	cl_mem sum_buffer = createBuffer(sizeof(sum));

	size_t globalsize[] = { src_size };
	size_t localsize[] = { src_size / sum_size };
	vector<pair<size_t, const void*>> args = {
		make_pair(sizeof(src_buffer), (const void*)&src_buffer),
		make_pair(sizeof(sum_buffer), (const void*)&sum_buffer),
		make_pair(sizeof(double)*localsize[0], (const void*)NULL)
	};
	setKernelArgs(kernel, args);
	runKernel(kernel, 1, globalsize, localsize);

	// NOTES: alternative:
	//double* s = (double*)mapBuffer(sum_buffer, CL_MAP_READ, sizeof(sum));
	//for (int i = 0; i < sum_size; i++) {
	//	std::cout << s[i] << std::endl;
	//}
	//unmapBuffer(sum_buffer, s);
	readBuffer(sum_buffer, sum, sizeof(sum));
	for (int i = 0; i < sum_size; i++) {
		cout << sum[i] << endl;
	}

	releaseBuffer(src_buffer);
	releaseBuffer(sum_buffer);
	releaseKernel(kernel);
	releaseProgram(program);

	releaseOpenCL();
}