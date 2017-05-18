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
#include <thread>
#include "precomp.hpp"
#include "opencv2/imvt.hpp"

namespace cv {
namespace imvt {

using namespace std;
using namespace cv;

struct RenderBuffer {

	// previous images/flows
	vector<UMat> preImageLs;
	vector<UMat> preImageRs;
	vector<UMat> preFlowLtoRs;
	vector<UMat> preFlowRtoLs;

	// current images
	vector<UMat> imageLs;
	vector<UMat> imageRs;

	// set by the first time
	vector<UMat> warps;

	static RenderBuffer& instance() {
		static RenderBuffer wrapper;
		return wrapper;
	}

	bool isInit() {
		return warps.size() > 0;
	}

	void init(const vector<UMat>& images) {
		preImageLs.assign(images.size(), UMat());
		preImageRs.assign(images.size(), UMat());
		preFlowLtoRs.assign(images.size(), UMat());
		preFlowRtoLs.assign(images.size(), UMat());
		imageLs.assign(images.size(), UMat());
		imageRs.assign(images.size(), UMat());

		Mat warp(images[0].rows, images[0].cols, CV_32FC3);
		for (int x = 0; x < warp.cols; ++x) {
			const float shift = float(x) / float(warp.cols);
			const float slabShift = float(x);
			for (int y = 0; y < warp.rows; ++y) {
				warp.at<Point3f>(y, x) = Point3f(slabShift, y, shift);
			};
		}
		warps.assign(images.size(), warp.getUMat(ACCESS_READ));
	}


	void release() {
		preImageLs.clear();
		preImageRs.clear();
		preFlowLtoRs.clear();
		preFlowRtoLs.clear();
		imageLs.clear();
		imageRs.clear();
		warps.clear();
	}

	static void renderChunk(RenderBuffer* c, int i, UMat* chunk) {
		// compute optical flows
		UMat flowLtoR;
		UMat flowRtoL;
		imvt::computeOpticalFlow(
			c->imageLs[i],
			c->imageRs[i],
			c->preFlowLtoRs[i],
			c->preImageLs[i],
			c->preImageRs[i],
			flowLtoR,
			imvt::DirectionHint::LEFT);
		imvt::computeOpticalFlow(
			c->imageRs[i],
			c->imageLs[i],
			c->preFlowRtoLs[i],
			c->preImageRs[i],
			c->preImageLs[i],
			flowRtoL,
			imvt::DirectionHint::RIGHT);

		// combine novel views
		*chunk = imvt::combineLazyNovelViews(
			c->warps[i],
			c->imageLs[i],
			c->imageRs[i],
			flowLtoR,
			flowRtoL);

		// save previous images/flows
		c->preImageLs[i] = c->imageLs[i];
		c->preImageRs[i] = c->imageRs[i];
		c->preFlowLtoRs[i] = flowLtoR;
		c->preFlowRtoLs[i] = flowRtoL;

		// wait for opencl completed
		ocl::Queue::getDefault().finish();

	}

	void renderChunks(const vector<UMat>& imgLs, const vector<UMat>& imgRs, vector<UMat>& chunks) {
		chunks.assign(imageLs.size(), UMat());
		vector<thread> threads;
		for (int i = 0; i < imageLs.size(); ++i) {
			imageLs[i] = imgLs[i];
			imageRs[i] = imgRs[i];
			threads.push_back(thread(renderChunk, this, i, &chunks[i]));
		}
		for (thread& t : threads) {
			t.join();
		}
	}
};


CV_EXPORTS_W void renderPanoramaChunks(
	const vector<UMat>& imageLs,
	const vector<UMat>& imageRs,
	vector<UMat>& chunks) {
	RenderBuffer& wrapper = RenderBuffer::instance();
	if (!wrapper.isInit()) {
		wrapper.init(imageLs);
	}
	wrapper.renderChunks(imageLs, imageRs, chunks);
}

CV_EXPORTS_W void releaseRenderBuffer() {
	RenderBuffer& wrapper = RenderBuffer::instance();
	wrapper.release();
}


} // namespace imvt
} // namespace cv


