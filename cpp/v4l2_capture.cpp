#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "v4l2_capture.h"


static inline const char* str(unsigned long ioctlCode) {
    switch (ioctlCode) {
        case VIDIOC_QUERYCAP:
            return "VIDIOC_QUERYCAP";
        case VIDIOC_G_FMT:
            return "VIDIOC_G_FMT";
        case VIDIOC_S_FMT:
            return "VIDIOC_S_FMT";
        case VIDIOC_G_PARM:
            return "VIDIOC_G_PARM";
        case VIDIOC_S_PARM:
            return "VIDIOC_S_PARM";
        case VIDIOC_STREAMON:
            return "VIDIOC_STREAMON";
        case VIDIOC_STREAMOFF:
            return "VIDIOC_STREAMOFF";
        case VIDIOC_QBUF:
            return "VIDIOC_QBUF";
        case VIDIOC_DQBUF:
            return "VIDIOC_DQBUF";
        case VIDIOC_REQBUFS:
            return "VIDIOC_REQBUFS";
        case VIDIOC_QUERYBUF:
            return "VIDIOC_QUERYBUF";
        case VIDIOC_ENUM_FMT:
            return "VIDIOC_ENUM_FMT";
        case VIDIOC_ENUM_FRAMESIZES:
            return "VIDIOC_ENUM_FRAMESIZES";
        case VIDIOC_ENUMINPUT:
            return "VIDIOC_ENUMINPUT";
        case VIDIOC_S_INPUT:
            return "VIDIOC_S_INPUT";
        case VIDIOC_G_INPUT:
            return "VIDIOC_G_INPUT";
    }
    return "UNKNOWN_IOCODE";
}

V4L2Capture::V4L2Capture() {
}

V4L2Capture::V4L2Capture(int index) {
    open(index);
}
V4L2Capture::V4L2Capture(const char* deviceName) {
    open(deviceName);
}
V4L2Capture::~V4L2Capture() {
    if (deviceHandle != -1) {
        try_set_streaming(false);
        try_release_buffers();
        ::close(deviceHandle);
        deviceHandle = -1;
    }
}
bool V4L2Capture::isOpened() {
    return opened;
}
bool V4L2Capture::open(int index) {
    char name[256];
    if (index < 0) {
        int deviceIndex = -1;
        for (int i=0; i < MAX_CAMERAS; ++i) {
            snprintf(name, sizeof(name), "/dev/video%d", i);
            if (open(name))
                return true;
        }
        if (deviceIndex < 0) {
            LOGE("no v4l2 device found\n");
            return false;
        }
    } else {
        snprintf(name, sizeof(name), "/dev/video%d", index);
    }
    return open(name);
}

bool V4L2Capture::open(const char* deviceName) {
    LOGI("try to open %s ...\n", deviceName);
    // O_RDWR is requried, use O_NONBLOCK flag so that we can do select/poll in latter time
    deviceHandle = ::open(deviceName, O_RDWR | O_NONBLOCK, 0);
    if (deviceHandle == -1) {
        LOGE("open %s failed: %s\n", deviceName, strerror(errno));
        return false;
    }
    opened = init_capture();
    if (!opened) {
        ::close(deviceHandle);
        deviceHandle = -1;
        return false;
    }
    LOGI("%s is opened\n", deviceName);
    return opened;
}

bool V4L2Capture::read(void** raw, uint32_t* length) {
    if (deviceHandle == -1) {
        LOGE("capture must be opened before calling read()\n");
        return false;
    }

    if (bufferIndex >= 0) {
        // Revert buffer to the queue
        if (!try_queue_buffer(bufferIndex))
            LOGE("failed to revert buffer %u to queue\n", bufferIndex);
        else
            bufferIndex = -1;
    }
    if (!try_dequeue_buffer()) {
        LOGE("unable to get buffer from queue\n");
        return false;
    }
    if (raw)
        *raw = buffers[bufferIndex].start;
    if (length)
        *length = buffers[bufferIndex].buffer.bytesused;
    LOGD("got frame: timestamp=%lu, buf=%p, len=%u, width=%u, height=%u, format=%c%c%c%c\n",
         (1000*buffers[bufferIndex].buffer.timestamp.tv_sec + buffers[bufferIndex].buffer.timestamp.tv_usec/1000),
         buffers[bufferIndex].start, buffers[bufferIndex].buffer.bytesused, width, height,
         pixelformat&0xff, (pixelformat>>8)&0xff, (pixelformat>>16)&0xff, (pixelformat>>24)&0xff);
    return true;
}

bool V4L2Capture::set(int property, double value) {
    if (deviceHandle == -1) {
        LOGE("capture must be opened before calling set()\n");
        return false;
    }

    int32_t propertyValue = uint32_t(int(value+0.5));
    switch (property) {
        case CAP_PROP_BUFFERSIZE:
        {
            if (propertyValue < 1 || propertyValue > MAX_BUFFERS) {
                LOGE("buffer size must be from 1 to %d\n", MAX_BUFFERS);
                return false;
            }
            if (propertyValue == bufferSize)
                return true;
            int32_t oldValue = bufferSize;
            requestBufferSize = uint32_t(propertyValue);
            if (reset_capture() && (bufferSize == propertyValue))
                return true;
            if (bufferSize != oldValue) {
                requestBufferSize = oldValue;
                reset_capture();
            }
            LOGE("unable to set buffer size as %u\n", propertyValue);
            return false;
        }

        case GAP_PROP_FOURCC:
        {
            if (propertyValue == pixelformat)
                return true;
            int32_t oldValue = pixelformat;
            pixelformat = propertyValue;
            if (reset_capture() && (pixelformat == propertyValue))
                return true;
            if (pixelformat != oldValue) {
                pixelformat = oldValue;
                reset_capture();
            }
            LOGE("unable to set fourcc as %c%c%c%c\n",
                    propertyValue&0xff, (propertyValue>>8)&0xff, (propertyValue>>16)&0xff, (propertyValue>>24)&0xff);
            return false;
        }

        case CAP_PROP_FRAME_WIDTH:
        case CAP_PROP_FRAME_HEIGHT:
        {
            if (propertyValue < 1 || propertyValue > 1920 || propertyValue % 16 != 0) {
                LOGE("both width and height must be divided by 16 and not greater than 1920\n");
                return false;
            }
            bool isSettingWidth = (property == CAP_PROP_FRAME_WIDTH);
            if (isSettingWidth)
                requestWidth = propertyValue;
            else
                requestHeight = propertyValue;
            // two subsequent calls setting WIDTH and HEIGHT will change the video size
            if (0 == requestWidth || 0 == requestHeight)
                return true;
            if (width == requestWidth && height == requestHeight) {
                requestWidth = requestHeight = 0;
                return true;
            }
            uint32_t oldWidth = width;
            uint32_t oldHeight = height;
            width = requestWidth;
            height = requestHeight;
            if (reset_capture() && width == requestWidth && height == requestHeight) {
                requestWidth = requestHeight = 0;
                return true;
            }

            LOGE("unable to set frame size as %ux%u\n", requestWidth, requestHeight);
            requestWidth = requestHeight = 0;
            if (width != oldWidth || height != oldHeight) {
                width = oldWidth;
                height = oldHeight;
                reset_capture();
            }
            return false;
        }

        case CAP_PROP_FPS:
        {
            if (propertyValue < 1 || propertyValue > 60) {
                LOGE("fps must be from 1 to 60\n");
                return false;
            }
            if (propertyValue == fps)
                return true;
            int32_t oldValue = fps;
            fps = propertyValue;
            if (reset_capture() && fps == propertyValue)
                return true;
            if (fps != oldValue) {
                fps = oldValue;
                reset_capture();
            }
            LOGE("unable to set fps as %u\n",propertyValue);
            return false;
        }

        case CAP_PROP_CHANNEL:
        {
            if (propertyValue < 0 || propertyValue > 256) {
                LOGE("input channel must be from 0 to 256\n");
                return false;
            }
            if (propertyValue == inputChannel)
                return true;
            int32_t oldValue = inputChannel;
            inputChannel = propertyValue;
            if (reset_capture() && inputChannel == propertyValue)
                return true;
            if (inputChannel != oldValue) {
                inputChannel = oldValue;
                reset_capture();
            }
            LOGE("unable to set input channel as %u\n",propertyValue);
            return false;
        }
    }
    LOGE("unsupported seting property: %d\n", property);
    return false;
}
double V4L2Capture::get(int property) {
    if (deviceHandle == -1) {
        LOGE("capture must be opened before calling get()\n");
        return 0;
    }
    switch (property) {
        case CAP_PROP_BUFFERSIZE:
            return bufferSize;
        case GAP_PROP_FOURCC:
            return pixelformat;
        case CAP_PROP_FRAME_WIDTH:
            return width;
        case CAP_PROP_FRAME_HEIGHT:
            return height;
        case CAP_PROP_FPS:
            return fps;
        case CAP_PROP_CHANNEL:
            return inputChannel;
    }
    LOGE("unsupported getting property: %d\n", property);
    return 0;
}

bool V4L2Capture::init_capture() {
    if (!try_capability())
        return false;
    if (!try_emum_inputs())
        return false;
    if (!try_set_input())
        return false;
    if (!try_enum_formats())
        return false;
    if (!try_set_format())
        return false;
    if (!try_set_fps())
        return false;
    if (!try_request_buffers(requestBufferSize))
        return false;
    if (!try_create_buffers()) {
        try_request_buffers(0);
        return false;
    }
    // We should queue buffers before streaming.
    for (uint32_t i=0; i < bufferSize; ++i)
        try_queue_buffer(i);
    if (!try_set_streaming(true)) {
        try_release_buffers();
        return false;
    }
    return true;
}

bool V4L2Capture::reset_capture() {
    try_set_streaming(false);
    try_release_buffers();
    return init_capture();
}

bool V4L2Capture::try_capability() {
    v4l2_capability capability = v4l2_capability();
    if (!try_ioctl(VIDIOC_QUERYCAP, &capability)) {
        LOGE("unable to query capability.\n");
        return false;
    }
    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("unable to capture video memory.\n");
        return false;
    }
    return true;
}

bool V4L2Capture::try_ioctl(unsigned long ioctlCode, void* parameter) const {
    while (-1 == ioctl(deviceHandle, ioctlCode, parameter)) {
        if (!(errno == EBUSY || errno == EAGAIN)) {
            if (errno == EINVAL &&  (ioctlCode == VIDIOC_ENUM_FMT || ioctlCode == VIDIOC_ENUM_FRAMESIZES || ioctlCode == VIDIOC_ENUMINPUT))
                LOGD("ioctl: %s  failed, %s\n", str(ioctlCode), "invalid index");
            else
                LOGE("ioctl: %s  failed\n", str(ioctlCode));
            return false;
        }
        // Poll device
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(deviceHandle, &fds);
        // Timeout is 10s
        struct timeval tv;
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        int result = select(deviceHandle + 1, &fds, NULL, NULL, &tv);
        if (0 == result) {
            LOGE("select timeout\n");
            return false;
        }
        if (-1 == result && EINTR != errno) {
            LOGE("select failed: %s\n", strerror(errno));
            return false;
        }
    }
    LOGD("ioctl %s ok\n", str(ioctlCode));
    return true;
}

bool V4L2Capture::try_enum_formats() {
    bool status = false;
    v4l2_fmtdesc fmtdesc = v4l2_fmtdesc();
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (try_ioctl(VIDIOC_ENUM_FMT, &fmtdesc) && fmtdesc.index < 256) {
        status = true;
        LOGI("pixelformat: %c%c%c%c, description: %s\n",
                fmtdesc.pixelformat & 0xff,
                (fmtdesc.pixelformat >> 8) & 0xff,
                (fmtdesc.pixelformat >> 16) & 0xff,
                (fmtdesc.pixelformat >> 24) & 0xff,
                fmtdesc.description);
        // get all available frame size of specified pixel format
        v4l2_frmsizeenum frmsize = v4l2_frmsizeenum();
        frmsize.index = 0;
        frmsize.pixel_format = fmtdesc.pixelformat;
        while (try_ioctl(VIDIOC_ENUM_FRAMESIZES, &frmsize) && frmsize.index < 256) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                LOGI("discrete: width=%u, height=%u\n", frmsize.discrete.width, frmsize.discrete.height);
                // get next available frame size
                frmsize.index++;
            } else if (frmsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
                LOGI("continuous: min_width=%u, min_height=%u, max_width=%u, max_height=%u\n",
                       frmsize.stepwise.min_width, frmsize.stepwise.min_height, frmsize.stepwise.max_width, frmsize.stepwise.max_height);
                break;
            } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                LOGI("stepwise: min_width=%u, min_height=%u, max_width=%u, max_height=%u, step_width=%u, step_height=%u\n",
                       frmsize.stepwise.min_width, frmsize.stepwise.min_height,
                       frmsize.stepwise.max_width, frmsize.stepwise.max_height,
                       frmsize.stepwise.step_width, frmsize.stepwise.step_height);
                break;
            }
        }
        // get next available pixel format
        fmtdesc.index++;
    }
    return  status;
}

bool V4L2Capture::try_emum_inputs() {
    bool status = false;
    v4l2_input input = v4l2_input();
    input.index = 0;
    while (try_ioctl(VIDIOC_ENUMINPUT, &input) && input.index < 256) {
        status = true;
        LOGI("input： %u\n", input.index);
        input.index++;
    }
    return status;
}

bool V4L2Capture::try_set_input() {
    v4l2_input input = v4l2_input();
    input.index = inputChannel;
    try_ioctl(VIDIOC_S_INPUT, &input);
    return try_ioctl(VIDIOC_G_INPUT, &inputChannel);
}

bool V4L2Capture::try_set_format() {
    __u32 try_order[] = {
            pixelformat,
            V4L2_PIX_FMT_RGB24,
            V4L2_PIX_FMT_YUV420,
            V4L2_PIX_FMT_YUYV,
            V4L2_PIX_FMT_UYVY,
            V4L2_PIX_FMT_NV12,
            V4L2_PIX_FMT_NV21,
            V4L2_PIX_FMT_JPEG,
    };
    for (size_t i = 0; i < sizeof(try_order) / sizeof(__u32); i++) {
        if (0 == try_order[i])
            continue;
        v4l2_format format = v4l2_format();
        format.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.pixelformat = try_order[i];
        format.fmt.pix.field       = V4L2_FIELD_ANY;
        format.fmt.pix.width       = width;
        format.fmt.pix.height      = height;
        if (try_ioctl(VIDIOC_S_FMT, &format) && try_ioctl(VIDIOC_G_FMT, &format)) {
            width = format.fmt.pix.width;
            height = format.fmt.pix.height;
            pixelformat = format.fmt.pix.pixelformat;
            LOGI("formats: width=%u, height=%u, pixelformat=%c%c%c%c\n",
                 width, height, pixelformat&0xff, (pixelformat>>8)&0xff, (pixelformat>>16)&0xff, (pixelformat>>24)&0xff);
            return true;
        }
    }
    LOGW("no proper formats, use default format\n");
    v4l2_format format = v4l2_format();
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (try_ioctl(VIDIOC_G_FMT, &format)) {
        width = format.fmt.pix.width;
        height = format.fmt.pix.height;
        pixelformat = format.fmt.pix.pixelformat;
        LOGI("format: width=%u, height=%u, pixelformat=%c%c%c%c\n",
             width, height, pixelformat&0xff, (pixelformat>>8)&0xff, (pixelformat>>16)&0xff, (pixelformat>>24)&0xff);
        return true;
    }
    return false;
}

bool V4L2Capture::try_set_fps() {
    v4l2_streamparm streamparm = v4l2_streamparm();
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamparm.parm.capture.timeperframe.numerator = 1;
    streamparm.parm.capture.timeperframe.denominator = fps;
    if (!try_ioctl(VIDIOC_S_PARM, &streamparm) || !try_ioctl(VIDIOC_G_PARM, &streamparm))
        return false;
    fps = streamparm.parm.capture.timeperframe.denominator;
    LOGI("fps: %u\n", fps);
    return true;
}

bool V4L2Capture::try_set_streaming(bool on) {
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (!try_ioctl(on ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type))
        return false;
    LOGI("streaming: %s\n", on ? "on":"off");
    return true;
}

bool V4L2Capture::try_request_buffers(uint32_t nums) {
    bufferSize = nums;
    // Release the buffers already allocated
    if (0 == bufferSize) {
        v4l2_requestbuffers req = v4l2_requestbuffers();
        req.count = bufferSize;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        if (!try_ioctl(VIDIOC_REQBUFS, &req)) {
            if (EINVAL == errno) {
                LOGE("unable to support memory mapping\n");
            }
            return false;
        }
        bufferSize = req.count;
        LOGI("buffers: %u\n", bufferSize);
        return true;
    }
    // Request allocating buffers
    while (bufferSize > 0) {
        v4l2_requestbuffers req = v4l2_requestbuffers();
        req.count = bufferSize;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        if (!try_ioctl(VIDIOC_REQBUFS, &req)) {
            if (EINVAL == errno) {
                LOGE("device does not support memory mapping\n");
            }
            return false;
        }
        if (req.count >= bufferSize) {
            bufferSize = req.count;
            break;
        }
        bufferSize -= 1;
        LOGE("insufficient buffer memory, decrease buffers\n");
    }
    if (bufferSize < 1) {
        LOGE("insufficient buffer memory\n");
        return false;
    }
    LOGI("buffers: %u\n", bufferSize);
    return true;
}

bool V4L2Capture::try_create_buffers() {
    for (uint32_t i=0; i < bufferSize; ++i) {
        v4l2_buffer buf = v4l2_buffer();
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (!try_ioctl(VIDIOC_QUERYBUF, &buf)) {
            return false;
        }
        buffers[i].start =
                ::mmap(NULL,        // start anywhere
                     buf.length,
                     PROT_READ,  // required
                     MAP_SHARED, // recommended
                     deviceHandle, buf.m.offset);
        if (MAP_FAILED == buffers[i].start) {
            LOGE("mmap failed: %s\n", strerror(errno));
            return false;
        }
        buffers[i].length = buf.length;
        buffers[i].buffer = buf;
        LOGD("buffer: index=%u, offset=%u, length=%u\n", buf.index, buf.m.offset, buf.length);
    }
    return true;
}

bool V4L2Capture::try_release_buffers() {
    for (int i=0; i < bufferSize; ++i) {
        if (-1 == ::munmap(buffers[i].start, buffers[i].length)) {
            LOGE("munmap failed: %s\n", strerror(errno));
        }
    }
    bufferIndex = -1;
    // Applications can call ioctl VIDIOC_REQBUFS again to change the number of buffers,
    // however this cannot succeed when any buffers are still mapped. A count value of zero
    // frees all buffers, after aborting or finishing any DMA in progress, an implicit VIDIOC_STREAMOFF.
    return  try_request_buffers(0);
}

bool V4L2Capture::try_queue_buffer(uint32_t index) {
    if (!try_ioctl(VIDIOC_QBUF, &buffers[index].buffer))
        return false;
    LOGD("queue buffer: %u\n", index);
    return true;
}

bool V4L2Capture::try_dequeue_buffer() {
    v4l2_buffer buf = v4l2_buffer();
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (!try_ioctl(VIDIOC_DQBUF, &buf))
        return false;

    assert(buf.index < bufferSize);
    assert(buffers[buf.index].length == buf.length);

    // We shouldn't use this buffer in the queue while not retrieve frame from it.
    buffers[buf.index].buffer = buf;
    bufferIndex = buf.index;
    return true;
}
