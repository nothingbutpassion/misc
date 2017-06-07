#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "precomp.hpp"
#include "opencl_kernels_oclrenderpano.hpp"

namespace cv {
namespace ocl {
namespace imvt {

using namespace std;
using namespace cv;

typedef Vec4w Vec4z;

CV_EXPORTS_W  UMat oclGammaLUT() {
	Mat lut_gamma(1, 65536, CV_16U);
	for (int i = 0; i < 65536; i++) {
		lut_gamma.at<ushort>(0, i) = pow((float)(i / 65536.0), 2.2) * 65536.0; 
	}
	return lut_gamma.getUMat(ACCESS_READ);
}

CV_EXPORTS_W UMat oclAntiGammaLUT() {
	Mat lut_anti_gamma(1, 65536, CV_16U);
	for (int i = 0; i < 65536; i++) {
		lut_anti_gamma.at<ushort>(0, i) = pow((float)(i / 65536.0), 1 / 2.2) * 65536.0;
	}
	return lut_anti_gamma.getUMat(ACCESS_READ);
}

CV_EXPORTS_W void oclAntiGammaAdjust(const UMat& lut_anti_gamma, UMat& image) {
	CV_Assert(lut_anti_gamma.type() == CV_16UC1 && image.type() == CV_16UC4);
	ocl::Kernel k( "anti_gamma_lut_adjust", ocl::oclrenderpano::coloradjust_oclsrc);
	k.args(ocl::KernelArg::ReadOnlyNoSize(lut_anti_gamma),
		ocl::KernelArg::ReadWrite(image));
	size_t globalsize[] = { image.cols, image.rows };
	size_t localsize[] = { 16, 16 };
	k.run(2, globalsize, localsize, false);
}

CV_EXPORTS_W void oclGammaAdjust(const UMat& lut_gamma, UMat& image) {
	CV_Assert(lut_gamma.type() == CV_16UC1 && image.type() == CV_16UC4);
	ocl::Kernel k( "gamma_lut_adjust", ocl::oclrenderpano::coloradjust_oclsrc);
	k.args(ocl::KernelArg::ReadOnlyNoSize(lut_gamma),
		ocl::KernelArg::ReadWrite(image));
	size_t globalsize[] = { image.cols, image.rows };
	size_t localsize[] = { 16, 16 };
	k.run(2, globalsize, localsize, false);
}

CV_EXPORTS_W UMat oclAddBrightnessAndClampMulti(const UMat& image, const float value) {
	CV_Assert(image.type() == CV_16UC4);
	UMat adjustedImage(image.size(), CV_16UC4);
	ocl::Kernel k("add_brightness_and_clamp_multi", ocl::oclrenderpano::coloradjust_oclsrc);
	k.args(ocl::KernelArg::ReadOnlyNoSize(image),
		ocl::KernelArg::WriteOnly(adjustedImage),
		ocl::KernelArg::Constant(&value, sizeof(value)));
	size_t globalsize[] = { adjustedImage.cols, adjustedImage.rows };
	size_t localsize[] = { 16, 16 };
	k.run(2, globalsize, localsize, false);
	return adjustedImage;
}


float getAverageBrightness16(UMat mat) {
	//Vec4z colorVec = mean(mat);
	Vec4z colorVec = sum(mat)/(mat.rows*mat.cols);
	float b;
	b = (float(colorVec[0]) * 0.114 + float(colorVec[1]) * 0.587 + float(colorVec[2]) * 0.299);
	return b;
}

float calcBrightnessRatio16(UMat L, UMat R, float avg = 0.0, bool mean_color = false) {
	float l, r;

	//Vec4z colorVec = mean(L);
	//Vec4z colorVecR = mean(R);
	Vec4z colorVec = sum(L)/(L.rows*L.cols);
	Vec4z colorVecR = sum(R)/(R.rows*R.cols);

	
	if (mean_color) {
		l = (colorVec[0] + colorVec[1] + colorVec[2]) / 3;
		r = (colorVecR[0] + colorVecR[1] + colorVecR[2]) / 3;
	}
	else {
		l = (float(colorVec[0]) * 0.114 + float(colorVec[1]) * 0.587 + float(colorVec[2]) * 0.299);
		r = (float(colorVecR[0]) * 0.114 + float(colorVecR[1]) * 0.587 + float(colorVecR[2]) * 0.299);
	}

	if (avg == 0.0)
		return  l / r;

	if (l < 0.2*avg || r < 0.2*avg || l > 3 * avg || r > 3 * avg) {
		return 1.0;
	}

	float ratio = l / r;
	return ratio;
}

float calcBrightnessRatioByGrid(UMat L, UMat R, float& average, int type = 0) {
	float ratio = 1.0;
	int grid_width = L.rows / 6;
	int y_count = L.rows / grid_width;
	Rect target_rect;
	float b_avg = getAverageBrightness16(L);
	for (int y = 0; y < y_count; y++) {
		Rect grid(0, y*grid_width, L.cols, grid_width);
		UMat l_grid = L(grid);
		float b;
		if (type == 0) {
			b = getAverageBrightness16(l_grid);
		}
		else {
			//b = getAverageBrightness(l_grid);
		}
		if (b > b_avg) {
			target_rect = grid;
			break;
		}
	}

	UMat targetL = L(target_rect);
	UMat targetR = R(target_rect);
	if (type == 0) {
		ratio = calcBrightnessRatio16(targetL, targetR);
	} else {
		//ratio = calcBrightnessRatio(targetL, targetR);
	}
	return ratio;
}

void adjustImagesToStandardUsingMul(int standardSeq, vector<UMat>& images, vector<UMat>& adjusted, 
	vector<double>& colorP, vector<double>& adjustColor, int type, bool debug) {

	int standard = 0;
	vector<double> resort(images.size(), 0.0);
	resort[0] = 1.0;

	for (int i = 1; i < images.size(); i++) {
		resort[i] = colorP[i - 1] * resort[i - 1];
	}
	vector<double> sorted = vector<double>(resort);
	sort(sorted.begin(), sorted.end());

	if (standardSeq != -1) {
		for (int i = 0; i < images.size(); i++) {
			if (sorted[standardSeq] == resort[i]) {
				standard = i;
				break;
			}
		}
	}
	for (int i = 0; i < images.size(); i++) {
		if (standardSeq != -1) {
			adjustColor[i] = resort[i] / resort[standard];
		}
		else {
			adjustColor[i] = resort[i] / sorted[sorted.size() - 2];
		}
	}
	if (debug) {
		printf("%d standard is \n", standard);
	}
	for (int i = 0; i < images.size(); i++) {
		if (debug) {
			printf("adjustImagesToStandardUsingMul  %d %f \n", i, adjustColor[i]);
		}
		if (adjustColor[i] != 1.0) {
			adjusted[i] = oclAddBrightnessAndClampMulti(images[i], adjustColor[i]);
		}
		else {
			adjusted[i] = images[i];
		}
	}
}




struct GammaLUT {
	static GammaLUT& instance() {
		static GammaLUT gammaLUT;
		return gammaLUT;
	}

	void init() {
		lut_gamma = oclGammaLUT();
		lut_anti_gamma = oclAntiGammaLUT();
	}

	void release() {
		lut_gamma = UMat();
		lut_anti_gamma = UMat();
	}

	UMat gammaTable() {
		if (lut_gamma.empty()) {
			lut_gamma = oclGammaLUT();
		}
		return lut_gamma;
	}

	UMat antiGammaTable() {
		if (lut_anti_gamma.empty()) {
			lut_anti_gamma = oclAntiGammaLUT();
		}
		return lut_anti_gamma;
	}

private:
	UMat lut_gamma;
	UMat lut_anti_gamma;
};


CV_EXPORTS_W void oclInitGammaLUT() {
	GammaLUT::instance().init();
}

CV_EXPORTS_W void oclReleaseGammaLUT() {
	GammaLUT::instance().release();
}

CV_EXPORTS_W bool oclPreColorAdjustByGamma(
	vector<UMat>& spheres, 
	int standard, 
	float project_width_degree,
	float adjust_ratio, 
	bool save_debug, 
	bool mean_color, 
	bool is_hdr) {

	int numCams = spheres.size();
	static vector<double> colorP = vector<double>(numCams);
	static vector<float>  color = vector<float>(numCams);
	static vector<double> adjustColor = vector<double>(numCams);

	static bool initialized_gamma_table = false;
	static vector<float> previousColorP;
	if (!initialized_gamma_table) {
		for (int i = 0; i < numCams; i++) {
			previousColorP.push_back(0.0);
		}
		initialized_gamma_table = true;
	}

	// apply anti-gamma adjust
	vector<UMat> gammaMats = vector<UMat>(numCams);
	UMat lut_anti_gamma = GammaLUT::instance().antiGammaTable();
	for (int i = 0; i < numCams; i++) {
		spheres[i].convertTo(gammaMats[i], CV_16UC4);
		gammaMats[i] = gammaMats[i].mul(256);
		oclAntiGammaAdjust(lut_anti_gamma, gammaMats[i]);
	}

	// get left/right overlap image bigthness ratio
	if (true) {
		int sampleHight = spheres[0].rows;
		int sampleHight_start = spheres[0].rows * 0;
		float overlapAngleDegrees = (project_width_degree * float(numCams) - 360.0) / float(numCams);
		int camImageWidth = spheres[0].cols;
		int overlapImageWidth = float(camImageWidth) * (overlapAngleDegrees / project_width_degree);

		bool need_adjust = false;
		int shift = overlapImageWidth / 10 * 3;
		int sample_width = overlapImageWidth / 10 * 4;
		int shift_parallox = 0;
		Rect left(spheres[0].cols - overlapImageWidth + shift + shift_parallox, sampleHight_start, sample_width, sampleHight);
		Rect right(shift, sampleHight_start, sample_width, sampleHight);

		for (int i = 0; i < numCams; i++) {
			UMat overlapImageL = gammaMats[i](left);
			UMat overlapImageR = gammaMats[(i + 1) % numCams](right);

			colorP[i] = calcBrightnessRatioByGrid(overlapImageL, overlapImageR, color[i]);
			if (colorP[i] < 0.3 || colorP[i] > 2.0) {
				colorP[i] = 1.0;
			}

			if (abs(previousColorP[i] - colorP[i]) > 0.01 && previousColorP[i] != 0.0) {
				colorP[i] = previousColorP[i] * 0.5 + colorP[i] * 0.5;
			}

			previousColorP[i] = colorP[i];

			float ratio = abs(1 - colorP[i]);
			if (ratio > adjust_ratio) {
				need_adjust = true;
			}
		}
		if (!need_adjust) {
			return need_adjust;
		}

		if (standard == -1) {
			is_hdr = true;
		}
	}

	// adjust image using muliple method
	if (!is_hdr) {
		adjustImagesToStandardUsingMul(standard, gammaMats, gammaMats, colorP, adjustColor, 0, save_debug);
	}
	else {
		adjustImagesToStandardUsingMul(-1, gammaMats, gammaMats, colorP, adjustColor, 0, save_debug);
	}

	// apply anti-gamma adjust
	UMat lut_gamma = GammaLUT::instance().gammaTable();
	for (int i = 0; i < numCams; i++) {
		oclGammaAdjust(lut_gamma, gammaMats[i]);
		gammaMats[i] = gammaMats[i].mul(1.0 / 256);
		gammaMats[i].convertTo(spheres[i], CV_8UC4);
	}
	return true;
}


}	// namespace imvt
}	// namespace ocl
}	// namespace cv
