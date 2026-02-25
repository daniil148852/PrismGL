#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdint>

namespace PrismGL {

class GLWrapper {
public:
    GLWrapper();
    ~GLWrapper();
    
    bool initialize();
    void shutdown();
    
    void clear();
    void setViewport(int x, int y, int width, int height);
    void setViewportScale(float scale);
    
    bool isGLES3Supported() const;
    bool isGLES32Supported() const;
    bool isVBOSupported() const;
    bool isVAOSupported() const;
    bool isInstancingSupported() const;
    
    const char* getVendor() const;
    const char* getRenderer() const;
    const char* getVersion() const;
    
    void enable(int cap);
    void disable(int cap);
    void setDepthFunc(int func);
    
private:
    EGLDisplay display;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;
    bool initialized;
    int viewportWidth;
    int viewportHeight;
    float viewportScale;
    
    bool initEGL();
    void* loadGLESFunction(const char* name);
};

}
