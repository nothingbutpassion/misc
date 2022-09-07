#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include <linux/videodev2.h>
#include <cstdint>

#define DEBUG_V4L2_CAPTURE
#ifdef  DEBUG_V4L2_CAPTURE
#include <cstdio>
#define LOGD(...)
#define LOGI(...)   ((void)fprintf(stdout, __VA_ARGS__))
#define LOGW(...)   ((void)fprintf(stderr, __VA_ARGS__))
#define LOGE(...)   ((void)fprintf(stderr, __VA_ARGS__))
#else
#define LOGD(...)
#define LOGI(...)
#define LOGW(...)
#define LOGE(...)
#endif

// NOTES:
// Usage: v4l2_fourcc('Y','U','Y','V') is equal to V4L2_PIX_FMT_YUYV that defined in linux/videodev2.h
// linux/videodev2.h should have include this macro
#ifndef v4l2_fourcc
#define v4l2_fourcc(a,b,c,d) ((uint32_t) (a) | ((uint32_t) (b) << 8) | ((uint32_t) (c) << 16) | ((uint32_t) (d) << 2
#endif

constexpr uint32_t MAX_CAMERAS      = 16;
constexpr uint32_t MAX_BUFFERS      = 8;
constexpr uint32_t DEFAULT_BUFFERS  = 4;
constexpr uint32_t DEFAULT_WIDTH    = 640;
constexpr uint32_t DEFAULT_HEIGHT   = 480;
constexpr uint32_t DEFAULT_FPS      = 30;

class V4L2Capture {
public:
    enum {
        GAP_PROP_FOURCC,
        CAP_PROP_FRAME_WIDTH,
        CAP_PROP_FRAME_HEIGHT,
        CAP_PROP_FPS,
        CAP_PROP_BUFFERSIZE,
        CAP_PROP_CHANNEL
    };

    V4L2Capture();
    V4L2Capture(int index);
    V4L2Capture(const char* deviceName);
    ~V4L2Capture();
    bool isOpened();
    bool open(int index=-1);
    bool open(const char* deviceName);
    bool read(void** raw, uint32_t* length);
    bool set(int property, double value);
    double get(int property);

private:
    bool init_capture();
    bool reset_capture();
    bool try_capability();
    bool try_ioctl(unsigned long ioctlCode, void* parameter) const;
    bool try_enum_formats();
    bool try_emum_inputs();
    bool try_set_input();
    bool try_set_format();
    bool try_set_fps();
    bool try_set_streaming(bool on);
    bool try_request_buffers(uint32_t bufferSize);
    bool try_create_buffers();
    bool try_release_buffers();
    bool try_queue_buffer(uint32_t index);
    bool try_dequeue_buffer();

private:
    bool opened = false;
    int deviceHandle = -1;
    uint32_t inputChannel = 0;
    uint32_t pixelformat = 0;
    uint32_t width  = DEFAULT_WIDTH;
    uint32_t height = DEFAULT_HEIGHT;
    uint32_t fps = DEFAULT_FPS;
    uint32_t bufferSize = 0;
    int32_t bufferIndex = -1;
    struct Buffer {
        void *  start = 0;
        size_t  length = 0;
        // This is dequeued buffer. It used for to put it back in the queue.
        // The buffer is valid only if bufferIndex >= 0
        v4l2_buffer buffer = v4l2_buffer();
    } buffers[MAX_BUFFERS];
    // NOTES:
    // requestWidth/Height is only used in set()
    // requestBufferSize is used in open() and set()
    uint32_t requestWidth = 0;
    uint32_t requestHeight = 0;
    uint32_t requestBufferSize = DEFAULT_BUFFERS;

};

#endif // V4L2_CAPTURE_H