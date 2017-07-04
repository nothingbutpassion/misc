/**
* Copyright (c) 2016-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE_render file in the root directory of this subproject. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#include <stdio.h>

#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>

#include "precomp.hpp"
#include "opencv2/core/opencl/runtime/opencl_core.hpp"
#include "opencv2/core/opencl/runtime/opencl_core_wrappers.hpp"

#include "opencv2/oclrenderpano/ocl_optflow.hpp"
#include "opencv2/oclrenderpano/ocl_novelview.hpp"
#include "opencv2/oclrenderpano/ocl_coloradjust.hpp"

#define LOGD printf
//#define LOGD(...)

namespace cv {
namespace ocl {
namespace imvt {

using namespace std;

template <class T>
class SafeQueue {
public:
    SafeQueue() : q(), m(), c() {}
    ~SafeQueue() {}

void enqueue(T t) {
    std::lock_guard<std::mutex> lock(m);
    q.push(t);
    c.notify_one();
}

T dequeue() {
    std::unique_lock<std::mutex> lock(m);
    while (q.empty()) {
        c.wait(lock);
    }
    T val = q.front();
    q.pop();
    return val;
}

private:
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;
};


struct RenderTask {
    int index;
    const UMat* imageL;
    const UMat* imageR;
    UMat* chunk;
    float motionThreshold;
};

struct RenderContext {

	SafeQueue<RenderTask> inQueue;
	SafeQueue<RenderTask> outQueue;
    vector<thread> threads;
    
	// previous images/flows
	vector<UMat> preImageLs;
	vector<UMat> preImageRs;
	vector<UMat> preFlowLtoRs;
	vector<UMat> preFlowRtoLs;

	// set by the first time
	vector<UMat> warps;

	static RenderContext& instance() {
		static RenderContext context;
		return context;
	}

	bool isInit() {
		return preImageLs.size() > 0;
	}

	void init(const vector<UMat>& images) {
		preImageLs.assign(images.size(), UMat());
		preImageRs.assign(images.size(), UMat());
		preFlowLtoRs.assign(images.size(), UMat());
		preFlowRtoLs.assign(images.size(), UMat());

		Mat warp(images[0].rows, images[0].cols, CV_32FC3);
		for (int x = 0; x < warp.cols; ++x) {
			const float shift = float(x) / float(warp.cols);
			const float slabShift = float(x);
			for (int y = 0; y < warp.rows; ++y) {
				warp.at<Point3f>(y, x) = Point3f(slabShift, y, shift);
			};
		}
		warps.assign(images.size(), UMat());
		for (int i = 0; i < warps.size(); ++i) {
			warp.copyTo(warps[i]);
		}
	}

	void release() {
		preImageLs.clear();
		preImageRs.clear();
		preFlowLtoRs.clear();
		preFlowRtoLs.clear();
		warps.clear();
	}

    void startThreads(int numThreads = 4) {
        for (int i=0; i < numThreads; i++) {
            threads.push_back(thread(renderChunkThread, this));
        }
    }

    void stopThreads() {
		for (thread& t : threads) {
			RenderTask stopTask = { -1 };
			inQueue.enqueue(stopTask);
		}
        for (thread& t : threads) {
            t.join();
        }
		threads.clear();
    }

	static void renderChunkThread(RenderContext* c) {

		// initialize OpenCL and SVM if necessary
		ocl::useOpenCL();
		if (ocl::haveSVM()) {
			ocl::Context::getDefault().useSVM();
		}

		LOGD("render thread %u is started\n", this_thread::get_id());
        
		while (1) {
			// get render task from input queue
			LOGD("render thread %u is waiting input task\n", this_thread::get_id());
			RenderTask t = c->inQueue.dequeue();

			LOGD("render thread %u got input task: %d\n", this_thread::get_id(), t.index);
			// exit this thread if stop task received
			if (t.index == -1) {
				break;
			}

			// compute optical flows
			UMat flowLtoR;
			UMat flowRtoL;
			oclComputeOpticalFlow(
				*(t.imageL),
				*(t.imageR),
				c->preFlowLtoRs[t.index],
				c->preImageLs[t.index],
				c->preImageRs[t.index],
				flowLtoR,
				DirectionHint::LEFT,
				t.motionThreshold);
			oclComputeOpticalFlow(
				*(t.imageR),
				*(t.imageL),
				c->preFlowRtoLs[t.index],
				c->preImageRs[t.index],
				c->preImageLs[t.index],
				flowRtoL,
				DirectionHint::RIGHT,
				t.motionThreshold);

			// combine novel views
			oclCombineNovelViews(
				c->warps[t.index],
				*(t.imageL),
				*(t.imageR),
				flowLtoR,
				flowRtoL,
				*(t.chunk));

			// save previous images/flows
			c->preImageLs[t.index] = t.imageL->clone();
			c->preImageRs[t.index] = t.imageR->clone();
			c->preFlowLtoRs[t.index] = flowLtoR.clone();
			c->preFlowRtoLs[t.index] = flowRtoL.clone();

			// wait for opencl completed
			ocl::Queue::getDefault().finish();
			LOGD("render thread %u finished input task: %d\n", this_thread::get_id(), t.index);
 
			// put render result to output queue
			c->outQueue.enqueue(t);
			LOGD("render thread %u enqueued output result: %d\n", this_thread::get_id(), t.index);
        }

		ocl::Queue::getDefault().~Queue();
		LOGD("render thread %u is exited\n", this_thread::get_id());
	}

	void renderChunks(const vector<UMat>& imgLs, const vector<UMat>& imgRs, vector<UMat>& chunks, float motionThreshold) {
		// no render threads
		if (threads.size() == 0) {
			return;
		}

		// put render tasks to input queue
		if (chunks.size() != imgLs.size()) {
			chunks.assign(imgLs.size(), UMat());
		}
		for (int index = 0; index < imgLs.size(); ++index) {
			RenderTask task = { index, &imgLs[index], &imgRs[index], &chunks[index], motionThreshold };
			inQueue.enqueue(task);
		}

		// get chunks from output queue
		for (int index = 0; index < imgLs.size(); ++index) {
			RenderTask out = outQueue.dequeue();
			chunks[out.index] = *(out.chunk);
		}
	}
};


static void setBufferPoolSize(size_t size) {
	MatAllocator* allocator = ocl::getOpenCLAllocator();
	BufferPoolController* controller = allocator->getBufferPoolController("OCL");
	if (controller) {
		controller->setMaxReservedSize(size);
	}
	controller = allocator->getBufferPoolController("HOST_ALLOC");
	if (controller) {
		controller->setMaxReservedSize(size);
	}
	controller = allocator->getBufferPoolController("SVM");
	if (controller) {
		controller->setMaxReservedSize(size);
	}
}

static void releaseBufferPool() {
	MatAllocator* allocator = ocl::getOpenCLAllocator();
	BufferPoolController* controller = allocator->getBufferPoolController("OCL");
	if (controller) {
		controller->freeAllReservedBuffers();
	}
	controller = allocator->getBufferPoolController("HOST_ALLOC");
	if (controller) {
		controller->freeAllReservedBuffers();
	}
	controller = allocator->getBufferPoolController("SVM");
	if (controller) {
		controller->freeAllReservedBuffers();
	}
}


CV_EXPORTS_W void oclRenderStereoPanoramaChunks(
	const vector<UMat>& imageLs,
	const vector<UMat>& imageRs,
	vector<UMat>& chunks,
	float motionThreshold) {
	RenderContext& context = RenderContext::instance();
	if (!context.isInit()) {
		context.init(imageLs);
		context.startThreads();
	}
	context.renderChunks(imageLs, imageRs, chunks, motionThreshold);
}

CV_EXPORTS_W void oclClearPreviousFrames() {
	RenderContext& context = RenderContext::instance();
	context.stopThreads();
	context.release();
}


static bool oclSelectDevice(string& device) {
	// no platforms
	vector<ocl::PlatformInfo> platformInfos;
	ocl::getPlatfomsInfo(platformInfos);
	if (platformInfos.size() == 0) {
		return false;
	}

	// select GPU devices
	vector<ocl::Device> devices;
	for (ocl::PlatformInfo& platform : platformInfos) {
		for (int i = 0; i < platform.deviceNumber(); i++) {
			ocl::Device device;
			platform.getDevice(device, i);
			if (device.available()
				&& device.type() == CL_DEVICE_TYPE_GPU
				&& (device.globalMemSize() >> 30) >= 4) {
				devices.push_back(device);
			}
		}
	}
	if (devices.size() == 0) {
		return false;
	}

	// select the device that has max global mem size
	string deviceName = devices[0].name();
	size_t maxMemSize = devices[0].globalMemSize();
	for (int i = 1; i < devices.size(); ++i) {
		if (devices[i].globalMemSize() > maxMemSize) {
			deviceName = devices[i].name();
			maxMemSize = devices[i].globalMemSize();
		}
	}
	device = ":GPU:" + deviceName;
}



CV_EXPORTS_W bool oclDeviceAvailable() {
	string device;
	return oclSelectDevice(device);
}


CV_EXPORTS_W bool oclInitialize() {
	string device;
	if (!oclSelectDevice(device)) {
		return false;
	}

	char value[64];
	if (!getenv("OPENCV_OPENCL_DEVICE")) {
		snprintf(value, sizeof(value), "OPENCV_OPENCL_DEVICE=%s", device.c_str());
		putenv(value);
	}
	
	/* @deprecated
	size_t bufferPoolSize = ocl::Device::getDefault().globalMemSize() >> 20;
	if (!getenv("OPENCV_OPENCL_BUFFERPOOL_LIMIT")) {
		snprintf(value, sizeof(value), "OPENCV_OPENCL_BUFFERPOOL_LIMIT=%luMB", bufferPoolSize);
		putenv(value);
	}
	if (!getenv("OPENCV_OPENCL_HOST_PTR_BUFFERPOOL_LIMIT")) {
		snprintf(value, sizeof(value), "OPENCV_OPENCL_HOST_PTR_BUFFERPOOL_LIMIT=%luMB", bufferPoolSize);
		putenv(value);
	}
	if (!getenv("OPENCV_OPENCL_SVM_BUFFERPOOL_LIMIT")) {
		snprintf(value, sizeof(value), "OPENCV_OPENCL_SVM_BUFFERPOOL_LIMIT=%luMB", bufferPoolSize);
		putenv(value);
	}
	*/
	size_t maxBufferPoolSize = ocl::Device::getDefault().globalMemSize();
	setBufferPoolSize(maxBufferPoolSize);

	if (ocl::haveOpenCL()) {
		ocl::setUseOpenCL(true);
		if (ocl::haveSVM()) {
			ocl::Context::getDefault().useSVM();
		}
	}
	if (ocl::useOpenCL()) {
		oclInitGammaLUT();
        return true;
    }
    return false;
}

CV_EXPORTS_W void oclRelease() {
	RenderContext& context = RenderContext::instance();
	context.stopThreads();
	context.release();
	oclReleaseGammaLUT();
	releaseBufferPool();
}


}	// namespace imvt
}	// namespace ocl
}	// namespace cv


