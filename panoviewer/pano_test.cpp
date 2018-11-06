#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core/ocl.hpp"

#include "utils.h"

#ifdef USE_OPENCL
#include "ocl_utils.h"
#endif

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
		printf("\rcpu fps: %f", fps);
		imshow("plane", plane);
	} while (waitKey(10) == -1);
}

#ifdef USE_OPENCL
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
	} while (waitKey(10) == -1);
}
#endif

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("usage %s <pano-image>", argv[0]);
		return -1;
	}
#ifdef USE_OPENCL
	if (ocl::haveOpenCL()) {
		ocl::setUseOpenCL(true);
		if (ocl::useOpenCL())
			oclPanoViewer(argv[1]);
	}
#endif
	panoViewer(argv[1]);
	return 0;
}