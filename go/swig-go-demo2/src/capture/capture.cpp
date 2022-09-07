#include "capture.h"

using namespace cv;

Capture::Capture() {}

Capture::~Capture() {}

bool Capture::open(int device) {
	return cap.open(device);
}

bool Capture::read() {
	return cap.read(frame);
}

void Capture::edge() {
	cvtColor(frame, edges, CV_BGR2GRAY);
    GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
    Canny(edges, edges, 0, 30, 3);
}

void Capture::show(const char* wndName) {
    imshow(wndName, edges);	
}

void Capture::createWindow(const char* wndName) {
	namedWindow(wndName, CV_WINDOW_AUTOSIZE);
}

void Capture::destroyWindow(const char* wndName) {
	cv::destroyWindow(wndName);
}

int Capture::waitKey(int delay) {
	return cv::waitKey(delay);
}
