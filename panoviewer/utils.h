#pragma once

#include "opencv2/core.hpp"

using cv::Mat;
using cv::UMat;
void sphere_projection(const Mat& pano, Mat& plane, double theta, double phi, double fov_x, double fov_y);
void sphere_projection(const UMat& pano, UMat& plane, double theta, double phi, double fov_x, double fov_y);
