#include <Windows.h>
#include <math.h>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/ocl.hpp"

#include "utils.h"
#include "ocl_utils.h"

using namespace std;
using namespace cv;

#define PI 3.14159265358979323846

Point3d cross(Point3d p1, Point3d p2) {
	return Point3d(p1.y*p2.z - p1.z*p2.y, p1.z*p2.x - p1.x*p2.z, p1.x*p2.y - p1.y*p2.x);
}
double dot(Point3d p1, Point3d p2) {
	return p1.x*p2.x + p1.y*p2.y + p1.z*p2.z;
}
Point2d orthogonal_to_sphere(Point3d p) {
	double r = sqrt(dot(p, p));
	double theta = acos(p.z / r);
	double phi = atan2(p.y, p.x);
	phi = phi < 0 ? phi + 2 * PI : phi;
	return Point2d(phi, theta);
}
Point2d plane_to_sphere(double theta, double phi, Point2d p) {
	Point3d n = Point3d(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
	Point3d y = Point3d(sin(theta + PI / 2)*cos(phi), sin(theta + PI / 2)*sin(phi), cos(theta + PI / 2));
	Point3d x = cross(n, y);
	return orthogonal_to_sphere(n + x*p.x + y*p.y);
}
Point2d plane_to_sphere_map(double theta, double phi, Point2d fov, Size src_size, Size dst_size, Point2d dst_p) {
	double x_size = 2 * tan(fov.x / 2);
	double y_size = 2 * tan(fov.y / 2);
	double x = x_size * dst_p.x / dst_size.width;
	double y = y_size * dst_p.y / dst_size.height;
	x -= x_size / 2;
	y -= y_size / 2;
	Point2d pano_angle = plane_to_sphere(theta, phi, Point2d(x, y));
	return Point2d(src_size.width*pano_angle.x / (2 * PI), src_size.height*pano_angle.y / PI);
}

void sphere_projection(const Mat& pano, Mat& plane, double theta, double phi, double fov_x, double fov_y) {
	Mat map(plane.size(), CV_32FC2);
	for (int y = 0; y < map.rows; ++y) {
		for (int x = 0; x < map.cols; ++x) {
			Point2d p = plane_to_sphere_map(theta, phi, Point2d(fov_x, fov_y), pano.size(), plane.size(), Point2d(x, y));
			map.at<Point2f>(y, x) = Point2f(p.x, p.y);
		}
	}
	remap(pano, plane, map, Mat(), CV_INTER_CUBIC, BORDER_REPLICATE);
}

string get_executable_dir() {
	HMODULE hModule = GetModuleHandle(NULL);
	char path[MAX_PATH];
	GetModuleFileName(hModule, path, MAX_PATH);
	string pathstr(path);
	int pos = pathstr.rfind("\\");
	if (pos != string::npos) {
		return pathstr.substr(0, pos);
	}
	return string();
}

// NOTES: should call initOpenCL() first and releaseOpenCL when no longer used
void sphere_projection(const UMat& pano, UMat& plane, double theta, double phi, double fov_x, double fov_y) {
	
	attachOpenCL(
		(cl_context)ocl::Context::getDefault().ptr(), 
		(cl_device_id)ocl::Device::getDefault().ptr(),
		(cl_command_queue)ocl::Queue::getDefault().ptr());

	// NOTES: kernel/program should be released
	//static cl_program program = buildProgram("C:\\Users\\Administrator\\Downloads\\MyTesting\\PanoTest\\plane_to_sphere_map.cl");

	static cl_program program = buildProgram(get_executable_dir() + "\\plane_to_sphere_map.cl");
	static cl_kernel kernel = createKernel(program, "plane_to_sphere_map");

	UMat map(plane.size(), CV_32FC2);
	cl_mem map_buffer = (cl_mem)map.handle(ACCESS_WRITE);
	cl_int map_step = map.step;
	cl_int map_offset = map.offset;
	cl_int map_rows = map.rows;
	cl_int map_clos = map.cols;

	cl_int pano_rows = pano.rows;
	cl_int pano_clos = pano.cols;

	cl_float angle_theta = theta;
	cl_float angle_phi = phi;
	cl_float angle_fov_x = fov_x;
	cl_float angle_fov_y = fov_y;

	vector<pair<size_t, const void*>> args = {
		make_pair(sizeof(map_buffer),	(const void*)&map_buffer),
		make_pair(sizeof(map_step),		(const void*)&map_step),
		make_pair(sizeof(map_offset),	(const void*)&map_offset),
		make_pair(sizeof(map_rows),		(const void*)&map_rows),
		make_pair(sizeof(map_clos),		(const void*)&map_clos),

		make_pair(sizeof(pano_rows),	(const void*)&pano_rows),
		make_pair(sizeof(pano_clos),	(const void*)&pano_clos),

		make_pair(sizeof(angle_theta),	(const void*)&angle_theta),
		make_pair(sizeof(angle_phi),	(const void*)&angle_phi),
		make_pair(sizeof(angle_fov_x),	(const void*)&angle_fov_x),
		make_pair(sizeof(angle_fov_y),	(const void*)&angle_fov_y)
	};
	setKernelArgs(kernel, args);
	size_t globalsize[] = { ((map.cols + 15) >> 4) << 4, ((map.rows + 15) >> 4) << 4 };
	size_t localsize[] = { 16, 16 };
	runKernel(kernel, 2, globalsize, localsize);

	//releaseKernel(kernel);
	//releaseProgram(program);

	remap(pano, plane, map, UMat(), CV_INTER_CUBIC, BORDER_REPLICATE);
	ocl::finish();
}