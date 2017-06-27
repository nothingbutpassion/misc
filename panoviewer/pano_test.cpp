#include <iostream>
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/ocl.hpp"

#include "utils.h"
#include "ocl_utils.h"

using namespace std;
using namespace cv;

#define PI 3.14159265358979323846

struct ProjectArgument {
	double theta = PI / 2;
	double phi = PI / 3;
	double fov_x = 12 * PI / 16;
	double fov_y = 9 * PI / 16;
};
static void on_mouse(int event, int x, int y, int flags, void* userdata) {
	ProjectArgument* pa = static_cast<ProjectArgument*>(userdata);
	static bool hit = false;
	static int last_x;
	static int last_y;
	switch (event) {
	case EVENT_LBUTTONDOWN:
		last_x = x;
		last_y = y;
		hit = true;
		break;
	case EVENT_MOUSEMOVE:
		if (hit) {
			pa->phi += (x - last_x) / 128.0;
			pa->theta += (y - last_y) / 256.0;
			last_x = x;
			last_y = y;
		}
		break;
	case EVENT_LBUTTONUP:
		pa->phi += (x - last_x) / 128.0;
		pa->theta += (y - last_y) / 256.0;
		hit = false;
		break;
	default:
		break;
	}
}

void panoViewer(const string& pano_file) {
	ProjectArgument pa;
	namedWindow("plane");
	setMouseCallback("plane", on_mouse, &pa);

	Mat pano = imread(pano_file);
	Mat plane(pano.rows / 2, pano.cols / 3, pano.type());
	do {
		double start = cv::getCPUTickCount();
		sphere_projection(pano, plane, pa.theta, pa.phi, pa.fov_x, pa.fov_y);
		double fps = cv::getTickFrequency() / (cv::getCPUTickCount() - start);
		cout << "cpu fps: " << fps << endl;
		imshow("plane", plane);
	} while (waitKey(10) == 255);
}

void oclPanoViewer(const string& pano_file) {
	ProjectArgument pa;
	namedWindow("plane");
	setMouseCallback("plane", on_mouse, &pa);
	UMat pano = imread(pano_file).getUMat(ACCESS_READ);
	UMat plane(pano.rows / 2, pano.cols / 3, pano.type());
	do {
		double start = cv::getCPUTickCount();
		sphere_projection(pano, plane, pa.theta, pa.phi, pa.fov_x, pa.fov_y);
		double fps = cv::getTickFrequency()/ (cv::getCPUTickCount() - start);
		cout << "opencl fps: " << fps << endl;
		imshow("plane", plane);
	} while (waitKey(10) == 255);
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

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("usage %s <pano-image>", argv[0]);
		return -1;
	}

	if (ocl::haveOpenCL()) {
		oclPanoViewer(argv[1]);
	}else {
		panoViewer(argv[1]);
	}
	return 0;
}