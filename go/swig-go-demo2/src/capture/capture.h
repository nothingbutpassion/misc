#include "opencv2/opencv.hpp"

class Capture 
{
public:
	Capture();
	~Capture();
	bool open(int device);
	bool read();
	void edge();
	void show(const char* windowName);
	void createWindow(const char* windowName);
	void destroyWindow(const char* windowName);
	int waitKey(int delay);
private:
	cv::Mat frame;
	cv::Mat edges;
	cv::VideoCapture cap;
};



