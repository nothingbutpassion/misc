#pragma once

#include <EGL/egl.h>
#include <gui/SurfaceComposerClient.h>

class GLUtils {
public:
    bool init();
    void release();
    void setBackgroundImage(void* bgra, int width, int height);
    void setForegroundText(const char* text);
    void draw();
    void show();
    void hide();
private:
    void drawBackground();
    void drawForeground();
private:
    android::sp<android::SurfaceComposerClient> mSurfaceComposerClient;
    android::sp<android::SurfaceControl> mSurfaceControl;
    EGLDisplay mEGLDisplay = nullptr;
    EGLSurface mEGLSurface = nullptr;
    EGLSurface mEGLContext = nullptr;
    EGLint mSurfaceWidth = 0;
    EGLint mSurfaceHeight = 0;
};

