#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include "v4l2_capture.h"

using namespace std;

// -d /dev/video0 -s 1280x640 -f RGBA -r 15 -o output -c 100
// -h

void showUsage(const char* app) {
    LOGE("Usage: %s [-d <device-name>]  [-s <video-size>] [-f <pixel-format>] [-r <fps>] [-b <buffers>] [-i <input-channel>] [-o <out-file>] [-c <out-frames>] \n"
         "Example: %s -d /dev/video0 -s 640x480 -f RGB3 -r 30  -b 2 -i 0 -o cap.rgb -c 11\n", app, app);
}

int main(int argc, char* argv[])
{
    int opt;
    const char* app = argv[0];
    const char* device = nullptr;
    const char* size = nullptr;
    const char* format = nullptr;
    const char* rate = nullptr;
    const char* buffers = nullptr;
    const char* inChannel = nullptr;
    const char* outFile = nullptr;
    const char* outFrames = nullptr;
    while ((opt=getopt(argc, argv, "hd:s:f:r:b:i:o:c:")) != -1) {
        switch (opt) {
            case 'h':
                showUsage(app);
                return 0;
            case 'd':
                device = optarg;
                break;
            case 's':
                size = optarg;
                break;
            case 'f':
                format = optarg;
                break;
            case 'r':
                rate = optarg;
                break;
            case 'b':
                buffers = optarg;
                break;
            case 'i':
                inChannel = optarg;
                break;
            case 'o':
                outFile = optarg;
                break;
            case 'c':
                outFrames = optarg;
                break;
            default:
                showUsage(app);
                return -1;
        }
    }

    V4L2Capture capture;
    bool ok;
    if (device)
        ok = capture.open(device);
    else
        ok = capture.open();
    if (!ok) {
        LOGE("failed to open video capture\n");
        return -1;
    }
    int32_t channel = (int32_t) capture.get(V4L2Capture::CAP_PROP_CHANNEL);
    int32_t pixelformat = (int32_t) capture.get(V4L2Capture::GAP_PROP_FOURCC);
    int32_t width = (int32_t) capture.get(V4L2Capture::CAP_PROP_FRAME_WIDTH);
    int32_t height = (int32_t) capture.get(V4L2Capture::CAP_PROP_FRAME_HEIGHT);
    int32_t bufferSize = (int32_t) capture.get(V4L2Capture::CAP_PROP_BUFFERSIZE);
    int32_t fps = (int32_t) capture.get(V4L2Capture::CAP_PROP_FPS);
    LOGI("default properties: input=%u, format=%c%c%c%c, width=%u, height=%u, buffers=%u, fps=%u\n",
         channel, pixelformat&0xff, (pixelformat>>8)&0xff, (pixelformat>>16)&0xff, (pixelformat>>24)&0xff,
         width, height, bufferSize, fps);

    if (size) {
        string s(size);
        size_t found = s.find('x');
        if (found != string::npos) {
            width = stoi(s.substr(0, found));
            height = stoi(s.substr(found+1));
            capture.set(V4L2Capture::CAP_PROP_FRAME_WIDTH, width);
            capture.set(V4L2Capture::CAP_PROP_FRAME_HEIGHT, height);
        }
    }
    if (format) {
        string f(format);
        if (f.size() == 4) {
            pixelformat = v4l2_fourcc(f[0],f[1],f[2],f[3]);
            capture.set(V4L2Capture::GAP_PROP_FOURCC, pixelformat);
        }
    }
    if (rate) {
        string r(rate);
        fps = stoi(r);
        capture.set(V4L2Capture::CAP_PROP_FPS, fps);
    }
    if (buffers) {
        string b(buffers);
        bufferSize = stoi(b);
        capture.set(V4L2Capture::CAP_PROP_BUFFERSIZE, bufferSize);
    }
    if (inChannel) {
        string i(inChannel);
        channel = stoi(i);
        capture.set(V4L2Capture::CAP_PROP_CHANNEL, channel);
    }

    if (size || format || rate || buffers || inChannel) {
        channel = (int32_t) capture.get(V4L2Capture::CAP_PROP_CHANNEL);
        pixelformat = (int32_t) capture.get(V4L2Capture::GAP_PROP_FOURCC);
        width = (int32_t) capture.get(V4L2Capture::CAP_PROP_FRAME_WIDTH);
        height = (int32_t) capture.get(V4L2Capture::CAP_PROP_FRAME_HEIGHT);
        bufferSize = (int32_t) capture.get(V4L2Capture::CAP_PROP_BUFFERSIZE);
        fps = (int32_t) capture.get(V4L2Capture::CAP_PROP_FPS);
        LOGI("now properties: input=%u, format=%c%c%c%c, width=%u, height=%u, buffers=%u, fps=%u\n",
             channel, pixelformat&0xff, (pixelformat>>8)&0xff, (pixelformat>>16)&0xff, (pixelformat>>24)&0xff,
             width, height, bufferSize, fps);
    }
    if (outFile) {
        int outFrames = argc > 2 ? atoi(argv[2]) : 111;
        FILE* fp = fopen(outFile, "wb");
        if (!fp) {
            LOGE("can't open %s\n", outFile);
            return -1;
        }
        for (int i=0; i < outFrames; ++i) {
            void* frame = nullptr;
            uint32_t lenght = 0;
            if (!capture.read(&frame, &lenght)) {
                LOGE("read video frame error\n");
                break;
            }
            LOGI("captured video frame %d\n", i);
            fwrite(frame, 1, lenght, fp);
        }
        fclose(fp);
    }
    return 0;
}


