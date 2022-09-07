#include <map>
#include <string>
#include <ui/PixelFormat.h>
#include <ui/DisplayInfo.h>
#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "log.h"
#include "gl_utils.h"


using namespace std;
using namespace android;


#define LOG_TAG "GLUtils"


static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        LOGE("after %s() glError (0x%x)", op, error);
    }
}
struct Character {
    GLuint  textureID;
    GLuint  width;
    GLuint  height;
    GLint   bearingX;
    GLint   bearingY;
    GLint   advance;
};
struct CharacterTexture {
    CharacterTexture() {
        init();
    }
    ~CharacterTexture() {
        release();
    }
    const Character& operator[](char c)   {
        return mCharacters[c];
    }
private:
    bool init(const char* fontPath = "/system/fonts/NotoSerif-Regular.ttf") {
        if (FT_Init_FreeType(&mLibrary)) {
            LOGE("FT_Init_FreeType failed");
            return false;
        }
        if (FT_New_Face(mLibrary, fontPath, 0, &mFace)) {
            LOGE("FT_New_Face failed");
            release();
            return false;
        }
        if (FT_Set_Pixel_Sizes(mFace, 0, 64)) {
            LOGE("FT_Set_Pixel_Sizes failed");
            release();
            return false;
        }
        FT_GlyphSlot slot = mFace->glyph;
        for (char c=32; c < 127; ++c) {
            // Load character glyph
            if (FT_Load_Char(mFace, c, FT_LOAD_RENDER)) {
                LOGD("FT_Load_Char failed: aiisc_code=%02x", c);
                continue;
            }
            // Generate texture
            GLuint texture = 0;
            if (slot->bitmap.width > 0 && slot->bitmap.rows > 0) {
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    slot->bitmap.width,
                    slot->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    slot->bitmap.buffer
                );
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            // NOTES: advance is number of 1/64 pixels
            Character character = {
                texture, 
                slot->bitmap.width,
                slot->bitmap.rows,
                slot->bitmap_left, 
                slot->bitmap_top,
                (GLint)(slot->advance.x >> 6)
            };
            mCharacters.insert(make_pair(c, character));
        }    
        LOGI("%u character textures loaded", mCharacters.size());
        return true;
    }
    void release() {
        for (auto c: mCharacters) {
            glDeleteTextures(1, &c.second.textureID);
        }
        mCharacters.clear();
        if (mFace) {
            FT_Done_Face(mFace);
            mFace = nullptr;
        }
        if (mLibrary) {
            FT_Done_FreeType(mLibrary);
            mLibrary = nullptr;
        }
    }
      
private:
    map<char, Character> mCharacters;
    FT_Library mLibrary = nullptr;
    FT_Face mFace = nullptr;
};

class GLShader {
public:
    GLShader(const char* vertexSource, const char* fragmentSource) {
        mProgram = buildProgram(vertexSource, fragmentSource);
    }
    ~GLShader() {
        glDeleteShader(mVertexShader);
        glDeleteShader(mPixelShader);
        glDeleteProgram(mProgram);
    }
    void use() {
        glUseProgram(mProgram);
        checkGlError("glUseProgram");
    }
    GLint uniform(const char* name) {
        GLint location = glGetUniformLocation(mProgram, name);
        checkGlError("glGetUniformLocation");
        return location;
    }
private:
    GLuint buildProgram(const char* vertexSource, const char* fragmentSource) {
        mVertexShader = buildShader(GL_VERTEX_SHADER, vertexSource);
        if (!mVertexShader) {
            return 0;
        }
        mPixelShader = buildShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (!mPixelShader) {
            return 0;
        }
        GLuint program = glCreateProgram();
        checkGlError("glCreateProgram");
        if (program) {
            glAttachShader(program, mVertexShader);
            checkGlError("glAttachShader");
            glAttachShader(program, mPixelShader);
            checkGlError("glAttachShader");
            glLinkProgram(program);
            GLint linkStatus = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
            if (linkStatus != GL_TRUE) {
                GLint logLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
                if (logLength) {
                    char* log = new char[logLength];
                    glGetProgramInfoLog(program, logLength, nullptr, log);
                    LOGE("failed to link program:\n%s", log);
                    delete[] log;
                }
                glDeleteProgram(program);
                program = 0;
            }
        }
        return program;
    }
    GLuint buildShader(GLenum shaderType, const char* source) {
        GLuint shader = glCreateShader(shaderType);
        checkGlError("glCreateShader");
        if (shader) {
            glShaderSource(shader, 1, &source, nullptr);
            checkGlError("glShaderSource");
            glCompileShader(shader);
            checkGlError("glCompileShader");
            GLint compiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            checkGlError("glGetShaderiv");
            if (!compiled) {
                GLint logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
                checkGlError("glGetShaderiv");
                if (logLength > 0) {
                    char* log = new char[logLength];
                    glGetShaderInfoLog(shader, logLength, nullptr, log);
                    checkGlError("glGetShaderInfoLog");
                    LOGE("failed to compile shader %d:\n%s", shaderType, log);
                    delete[] log;
                    glDeleteShader(shader);
                    checkGlError("glDeleteShader");
                    shader = 0;
                }
            }
        }
        return shader;
    }
private:
    GLuint mProgram = 0;
    GLuint mVertexShader = 0;
    GLuint mPixelShader = 0;    
};

static const char* gBackVertexShader =
    "#version 300 es\n"
    "layout(location = 0) in vec4 aPos;\n"
    "layout(location = 1) in vec2 aTexCoord;\n"
    "out vec2 texCoord;\n"
    "void main() {\n"
    "    gl_Position = aPos;\n"
    "    texCoord = aTexCoord;\n"
    "}\n";

static const char* gBackFragmentShader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D uTex;\n"
    "in vec2 texCoord;\n"
    "out vec4 outColor;\n"
    "void main() {\n"
    "    outColor = texture(uTex, texCoord).bgra;\n"
    "}\n";
 
static const char* gFrontVertexShader =
    "#version 300 es\n"
    "layout (location = 0) in vec4 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 texCoord;\n"
    "void main()\n"
    "{\n"
    "  gl_Position = aPos;\n"
    "  texCoord = aTexCoord;\n"
    "}";

static const char* gFrontFragmentShader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D uTex;\n"
    "in vec2 texCoord;\n"
    "out vec4 outColor;\n"
    "void main()\n"
    "{\n"
    "   outColor = vec4(1.0, 0, 0, texture(uTex, texCoord).r);\n"
    "}"; 


static GLShader* gBackShader = nullptr;
static GLuint gBackTexture = 0;
static GLuint gBackVAO = 0;
static GLuint gBackVBO = 0;
static GLuint gBackEBO = 0;

static GLShader* gFrontShader = nullptr;
static string gFrontText;
static GLint  gFrontWidth = 0;
static GLint  gFrontHeight = 0;
static GLint  gFrontBase = 0;
static GLuint gFrontVAO = 0;
static GLuint gFrontVBO = 0;

static CharacterTexture* gCharacterTexture = nullptr;




static void initGLES(int surfaceWidth, int surfaceHeight) {
    LOGI("GL_VERSION: %s", glGetString(GL_VERSION));
    LOGI("GL_VENDOR: %s", glGetString(GL_VENDOR));
    LOGI("GL_RENDERER: %s", glGetString(GL_RENDERER));
    LOGI("GL_EXTENSIONS: %s", glGetString(GL_EXTENSIONS));
    LOGI("Surface Size: %dx%d", surfaceWidth, surfaceHeight);

    // NOTES: 
    // It's very import to set pixel storage alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    checkGlError("glPixelStorei");
    
    // Set viewport
    LOGI("GL Viewport: (%d, %d, %d, %d)", 0, 0, surfaceWidth, surfaceHeight);
    glViewport(0, 0, surfaceWidth, surfaceHeight);
    checkGlError("glViewport");

    // Init global shaders & character textures
    gBackShader = new GLShader(gBackVertexShader, gBackFragmentShader);
    gFrontShader = new GLShader(gFrontVertexShader, gFrontFragmentShader);
    gCharacterTexture = new CharacterTexture();
        
    // Enable culling for optimization
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);    
}


void releaseGLES() {
    delete gBackShader;
    delete gFrontShader;
    delete gCharacterTexture;
    gBackShader = nullptr;
    gFrontShader = nullptr;
    gCharacterTexture = nullptr;
}


bool GLUtils::init() {
    // Apply for a surface (or native window) from SurfaceFlinger
    DisplayInfo dinfo;
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain));
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
    if (status != NO_ERROR) {
        LOGE("SurfaceComposerClient::getDisplayInfo error: %d", status);
        return false;
    }    
    LOGI("DisplayInfo: width=%u, height=%u, orientation=%u", dinfo.w, dinfo.h, dinfo.orientation);
    sp<SurfaceComposerClient> composerClient = new SurfaceComposerClient();
    uint32_t w = dinfo.w; 
    uint32_t h = dinfo.h;
    if (dinfo.orientation == DISPLAY_ORIENTATION_90 || dinfo.orientation == DISPLAY_ORIENTATION_270) {
        w = dinfo.h;
        h = dinfo.w;
    }
    sp<SurfaceControl> control = composerClient->createSurface(String8("rvm"), w, h, PIXEL_FORMAT_RGB_888);
    SurfaceComposerClient::Transaction()
        .setLayer(control, 0x40000001)
        .apply();
        
    // Initialize EGL context
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major, minor;
    if (eglInitialize(display, &major, &minor) == EGL_FALSE) {
       LOGE("eglInitialize failed");
       return false; 
    }
    LOGI("EGL Version=%d.%d", major, minor);
    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE,           8,
        EGL_GREEN_SIZE,         8,
        EGL_BLUE_SIZE,          8,
        EGL_NONE
    };
    EGLConfig config;
    EGLint numConfigs;
    if (eglChooseConfig(display, attribs, &config, 1, &numConfigs) == EGL_FALSE) {
       LOGE("eglChooseConfig failed");
       return false; 
    }
    EGLSurface surface = eglCreateWindowSurface(display, config, control->getSurface().get(), nullptr);
    const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGE("eglMakeCurrent failed!");
        return false;
    }
    
    // Save native window and EGL context
    mSurfaceComposerClient = composerClient;
    mSurfaceControl = control;
    mEGLDisplay = display;
    mEGLSurface = surface;
    mEGLContext = context;

    // Init GLES
    eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_WIDTH, &mSurfaceWidth);
    eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_HEIGHT, &mSurfaceHeight);
    initGLES(mSurfaceWidth, mSurfaceHeight);
    return true;
}

void GLUtils::release() {
    releaseGLES();
    eglMakeCurrent(mEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(mEGLDisplay, mEGLSurface);
    eglDestroyContext(mEGLDisplay, mEGLContext);
    eglTerminate(mEGLDisplay);
    mSurfaceWidth = 0;
    mSurfaceHeight = 0;
    mSurfaceControl.clear();
    mSurfaceComposerClient.clear();
}


void GLUtils::setBackgroundImage(void* bgra, int width, int height) {
    if (!gBackTexture) {
        // Create background texture
        glGenTextures(1, &gBackTexture);
        checkGlError("glGenTextures");
        glBindTexture(GL_TEXTURE_2D, gBackTexture);
        checkGlError("glBindTexture");
        glGenerateMipmap(GL_TEXTURE_2D);
        checkGlError("glGenerateMipmap");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        checkGlError("glTexImage2D");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        checkGlError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        checkGlError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        checkGlError("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        checkGlError("glTexParameteri");
        // Generate background vertex coords
        EGLint surfaceWidth, surfaceHeight;
        eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_WIDTH, &surfaceWidth);
        eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_HEIGHT, &surfaceHeight);
        float xr = float(width)/surfaceWidth;
        float yr = float(height)/surfaceHeight;
        GLfloat backVertices[] = {
            // vertex coords [-1,1]          // texture coords [0,1]
            -1.0f*xr,   1.0f*yr,  0.0f,      0.0f, 0.0f,
            -1.0f*xr,  -1.0f*yr,  0.0f,      0.0f, 1.0f,
             1.0f*xr,  -1.0f*yr,  0.0f,      1.0f, 1.0f,
             1.0f*xr,   1.0f*yr,  0.0f,      1.0f, 0.0f
        };
        GLushort backIndices[] = { 0, 1, 2, 0, 2, 3 };
        // Generate the back vertex array object, then bind it, set vertex buffer, then configure vertex attributes.
        glGenVertexArrays(1, &gBackVAO);
        glGenBuffers(1, &gBackVBO);
        glGenBuffers(1, &gBackEBO);
        glBindVertexArray(gBackVAO);
        glBindBuffer(GL_ARRAY_BUFFER, gBackVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(backVertices), backVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gBackEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(backIndices), backIndices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));  
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);        
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBackTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bgra);    
}


void GLUtils::setForegroundText(const char* text) {
    // Generate frontground vertex coords
    if (!gFrontVAO) {
        glGenVertexArrays(1, &gFrontVAO);
        glGenBuffers(1, &gFrontVBO);
        glBindVertexArray(gFrontVAO);
        glBindBuffer(GL_ARRAY_BUFFER, gFrontVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*6*5, nullptr, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }
    if (gFrontText != text) {
        gFrontText = text;
        int lower = 0;
        int upper = 0;
        int textWidth = 0;
        for (auto c: gFrontText) {
            const Character& ch = (*gCharacterTexture)[c];
            textWidth += ch.advance;
            if (lower < ch.height - ch.bearingY)
                lower = ch.height - ch.bearingY;
            if (upper < ch.bearingY)
                upper = ch.bearingY;
            //LOGD("Character: char=\'%c\', tex=%u, size=%dx%d, bearing=%dx%d, advance=%d", 
            //        c, ch.textureID, ch.width, ch.height, ch.bearingX, ch.bearingY, ch.advance);
        }
        gFrontWidth = textWidth;
        gFrontHeight = lower + upper;
        gFrontBase = lower;
        LOGD("Foreground: text=\"%s\", size=%dx%d, baseline=%d", 
            text, gFrontWidth, gFrontHeight, gFrontBase);
    }
}

void GLUtils::drawBackground() {
    gBackShader->use();
    glUniform1i(gBackShader->uniform("uTex"), 0);
    glBindVertexArray(gBackVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

}

void GLUtils::drawForeground() {
    gFrontShader->use();
    glUniform1i(gFrontShader->uniform("uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(gFrontVAO);

    float xr = 1.0f/mSurfaceWidth;
    float yr = 1.0f/mSurfaceHeight;
    float x = -gFrontWidth/2.0f;
    float y = -gFrontHeight/2.0f;
    //float y = -mSurfaceHeight/2.0f;
    for (auto c: gFrontText) {
        const Character& ch = (*gCharacterTexture)[c];
        if (ch.textureID > 0) {
            GLfloat xpos = xr*(x + ch.bearingX);
            GLfloat ypos = yr*(y - ch.height + ch.bearingY);
            GLfloat w = xr*ch.width;
            GLfloat h = yr*ch.height;
            GLfloat vertices[] = {
                // vertex coords [-1,1]         // texture coords [0,1]
                xpos,     ypos + h,   0.0f,     0.0f, 0.0f,            
                xpos,     ypos,       0.0f,     0.0f, 1.0f,
                xpos + w, ypos,       0.0f,     1.0f, 1.0f,
                xpos,     ypos + h,   0.0f,     0.0f, 0.0f,
                xpos + w, ypos,       0.0f,     1.0f, 1.0f,
                xpos + w, ypos + h,   0.0f,     1.0f, 0.0f          
            }; 
            glBindTexture(GL_TEXTURE_2D, ch.textureID);
            glBindBuffer(GL_ARRAY_BUFFER, gFrontVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        x += ch.advance;
    }
}


void GLUtils::draw() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw background
    if (gBackVAO)
        drawBackground();

    // Draw foreground
    if (gFrontVAO)
        drawForeground();
    
    // Commit to show
    eglSwapBuffers(mEGLDisplay, mEGLSurface);
}

void GLUtils::show() {
    if (mSurfaceControl) {
        SurfaceComposerClient::Transaction()
            .show(mSurfaceControl)
            .apply();
    }

}

void GLUtils::hide() {
    if (mSurfaceControl) {
        SurfaceComposerClient::Transaction()
            .hide(mSurfaceControl)
            .apply();
    }
}





















