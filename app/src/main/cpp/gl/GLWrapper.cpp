#include "GLWrapper.h"
#include "../util/Logger.h"
#include <dlfcn.h>

namespace PrismGL {

GLWrapper::GLWrapper() : display(EGL_NO_DISPLAY), config(nullptr), 
    surface(EGL_NO_SURFACE), context(EGL_NO_CONTEXT), 
    initialized(false), viewportWidth(0), viewportHeight(0), viewportScale(1.0f) {}

GLWrapper::~GLWrapper() {
    shutdown();
}

bool GLWrapper::initialize() {
    if (initialized) return true;
    return initEGL();
}

bool GLWrapper::initEGL() {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }
    
    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
        LOGE("Failed to initialize EGL");
        return false;
    }
    LOGI("EGL version: %d.%d", major, minor);
    
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    
    EGLint numConfigs;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("Failed to choose EGL config");
        return false;
    }
    
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return false;
    }
    
    EGLint pbufferAttribs[] = {
        EGL_WIDTH, 512,
        EGL_HEIGHT, 512,
        EGL_NONE
    };
    
    surface = eglCreatePbufferSurface(display, config, pbufferAttribs);
    if (surface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL pbuffer surface");
        return false;
    }
    
    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("Failed to make EGL context current");
        return false;
    }
    
    initialized = true;
    LOGI("GLES3 context created successfully");
    LOGI("GL Vendor: %s", glGetString(GL_VENDOR));
    LOGI("GL Renderer: %s", glGetString(GL_RENDERER));
    LOGI("GL Version: %s", glGetString(GL_VERSION));
    
    return true;
}

void GLWrapper::shutdown() {
    if (!initialized) return;
    
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    
    display = EGL_NO_DISPLAY;
    surface = EGL_NO_SURFACE;
    context = EGL_NO_CONTEXT;
    initialized = false;
}

void GLWrapper::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GLWrapper::setViewport(int x, int y, int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    glViewport(x, y, width, height);
}

void GLWrapper::setViewportScale(float scale) {
    viewportScale = scale;
    if (viewportWidth > 0 && viewportHeight > 0) {
        glViewport(0, 0, 
            (GLsizei)(viewportWidth * scale), 
            (GLsizei)(viewportHeight * scale));
    }
}

bool GLWrapper::isGLES3Supported() const {
    return initialized;
}

bool GLWrapper::isGLES32Supported() const {
    if (!initialized) return false;
    const char* version = (const char*)glGetString(GL_VERSION);
    return version && version[0] >= '3' && version[2] >= '2';
}

bool GLWrapper::isVBOSupported() const {
    return initialized;
}

bool GLWrapper::isVAOSupported() const {
    return initialized;
}

bool GLWrapper::isInstancingSupported() const {
    return initialized;
}

const char* GLWrapper::getVendor() const {
    return initialized ? (const char*)glGetString(GL_VENDOR) : "";
}

const char* GLWrapper::getRenderer() const {
    return initialized ? (const char*)glGetString(GL_RENDERER) : "";
}

const char* GLWrapper::getVersion() const {
    return initialized ? (const char*)glGetString(GL_VERSION) : "";
}

void GLWrapper::enable(int cap) {
    glEnable(cap);
}

void GLWrapper::disable(int cap) {
    glDisable(cap);
}

void GLWrapper::setDepthFunc(int func) {
    glDepthFunc(func);
}

void* GLWrapper::loadGLESFunction(const char* name) {
    return (void*)eglGetProcAddress(name);
}

}
