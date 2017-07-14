/**
* Copyright (c) 2016-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE_render file in the root directory of this subproject. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#include <string>
#include <vector>
#include <iostream>
#include "precomp.hpp"
#include "opencl_kernels_oclrenderpano.hpp"
#include "opencv2/oclrenderpano/ocl_optflow.hpp"


namespace cv {
namespace ocl {
namespace imvt {

using namespace std;



static void oclCubicRemap(const UMat& src, UMat& dst, const UMat& mapx, const UMat& mapy) {
	CV_Assert(src.type() == CV_8UC4 || src.type() == CV_8UC3);
	CV_Assert(mapx.type() == CV_32FC1 && mapy.type() == CV_32FC1 || mapx.size() == mapy.size());
	UMat s = src;
	dst.create(mapx.size(), s.type());
	string srcType = s.type() == CV_8UC4 ? "_8UC4" : "_8UC3";
	string mapType = "_32FC1";
	string kernelName = string("cubic_remap") + srcType + mapType;
	ocl::Kernel k(kernelName.c_str(), ocl::oclrenderpano::remap_oclsrc);
	k.args(ocl::KernelArg::ReadOnly(src),
		ocl::KernelArg::WriteOnly(dst),
		ocl::KernelArg::ReadOnlyNoSize(mapx),
		ocl::KernelArg::ReadOnlyNoSize(mapy));
	size_t globalsize[] = { dst.cols, dst.rows };
	size_t localsize[] = { 16, 16 };
	k.run(2, globalsize, localsize, false);
}

CV_EXPORTS_W void oclProjection(
	const vector<UMat>& srcImages, 
	const vector<UMat>& xmap,
	const vector<UMat>& ymap,
	vector<UMat>& dstImages) {


	vector<UMat> outImages(srcImages.size());
	if (dstImages.size() != srcImages.size()) {
		dstImages = outImages;
	}

	for (int i = 0; i < srcImages.size(); ++i) {
		if (srcImages[i].type() == CV_8UC3) {
			UMat dstImage;
			oclCubicRemap(srcImages[i], dstImage, xmap[i], ymap[i]);
			cvtColor(dstImage, dstImages[i], COLOR_BGR2BGRA);
			//UMat tmp;
			//cvtColor(srcImages[i],  tmp, COLOR_BGR2BGRA);
			//oclCubicRemap(tmp, dstImages[i], xmap[i], ymap[i]);
		} else {
			oclCubicRemap(srcImages[i], dstImages[i], xmap[i], ymap[i]);
		}
	}
}


CV_EXPORTS_W void oclSmoothImage(UMat& pano, const UMat& previous, float thresh_hold, bool isPano) {
	if (pano.dims != previous.dims) {
		return;
	}
	if (previous.cols != 0) {
		CV_Assert(pano.type() == CV_8UC4 || pano.type() == previous.type());
		int is_pano = isPano ? 1 : 0;
		ocl::Kernel k("smooth_image", ocl::oclrenderpano::zcamutils_oclsrc);
		k.args(ocl::KernelArg::ReadWrite(pano),
			ocl::KernelArg::ReadOnlyNoSize(previous),
			ocl::KernelArg::Constant(&thresh_hold, sizeof(thresh_hold)),
			ocl::KernelArg::Constant(&is_pano, sizeof(is_pano)));
		size_t globalsize[] = { pano.cols, pano.rows };
		size_t localsize[] = { 16, 16 };
		k.run(2, globalsize, localsize, false);
	}
}

CV_EXPORTS_W void oclSharpImage(UMat& sphericalImage, float factor) {
	if (factor != 0.0) {
		UMat blured;
		oclGaussianBlur(sphericalImage, blured, Size(3, 3), 3);
		cv::addWeighted(sphericalImage, 1 + factor, blured, -1 * factor, 0, sphericalImage);
	}
}


CV_EXPORTS_W void oclStackHorizontal(const std::vector<UMat>& srcImages, UMat& dstImage) {
	int totalCols = 0;
	for (size_t i = 0; i < srcImages.size(); i++) {
		CV_Assert(srcImages[i].dims <= 2 && srcImages[i].rows == srcImages[0].rows && srcImages[i].type() == srcImages[0].type());
		totalCols += srcImages[i].cols;
	}

	int cols = 0;
	dstImage.create(srcImages[0].rows, totalCols, srcImages[0].type());
	for (size_t i = 0; i < srcImages.size(); i++) {
		UMat dpart = dstImage(Rect(cols, 0, srcImages[i].cols, srcImages[i].rows));
		srcImages[i].copyTo(dpart);
		cols += srcImages[i].cols;
	}
}



CV_EXPORTS_W void oclOffsetHorizontalWrap(const UMat& srcImage, float offset, UMat& dstImage) {
	// get warp mat
	UMat warpMat(srcImage.size(), CV_32FC2);
	ocl::Kernel k("offset_horizontal_wrap", ocl::oclrenderpano::zcamutils_oclsrc);
	k.args(ocl::KernelArg::WriteOnly(warpMat),
		ocl::KernelArg::Constant(&offset, sizeof(offset)));
	size_t globalsize[] = { warpMat.cols, warpMat.rows };
	size_t localsize[] = { 16, 16 };
	k.run(2, globalsize, localsize, false);

	// remap
	remap(
		srcImage,
		dstImage,
		warpMat,
		UMat(),
		INTER_NEAREST,
		BORDER_WRAP);
}


CV_EXPORTS_W void oclOffsetHorizontalWrap(UMat& image, float offset) {
	UMat dst;
	oclOffsetHorizontalWrap(image, offset, dst);
	image = dst;
}

static void olcRemoveChunkLine(UMat& chunk) {
	CV_Assert(chunk.type() == CV_8UC4);
	ocl::Kernel k("remove_chunk_line", ocl::oclrenderpano::zcamutils_oclsrc);
	k.args(ocl::KernelArg::ReadWrite(chunk));
	size_t globalsize[] = { chunk.rows };
	size_t localsize[] = { 64 };
	k.run(1, globalsize, localsize, false);
}

CV_EXPORTS_W void oclRemoveChunkLines(vector<UMat>& chunks) {
	for (int i = 0; i < chunks.size(); ++i) {
		olcRemoveChunkLine(chunks[i]);
	}
}


}	// namespace imvt
}	// namespace ocl
}	// namespace cv


