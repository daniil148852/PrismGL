#include "GLESToGLConverter.h"
#include "../util/Logger.h"
#include <dlfcn.h>
#include <cstring>

namespace PrismGL {

GLESToGLConverter::GLESToGLConverter()
    : initialized(false), majorVersion(0), minorVersion(0) {
}

GLESToGLConverter::~GLESToGLConverter() {
    shutdown();
}

bool GLESToGLConverter::initialize() {
    if (initialized) {
        return true;
    }

    detectGLESVersion();
    loadVendorInfo();

    initialized = true;
    LOGI("GLESToGLConverter: Initialized GL ES %d.%d", majorVersion, minorVersion);
    LOGI("GLESToGLConverter: Vendor: %s", vendor.c_str());
    LOGI("GLESToGLConverter: Renderer: %s", renderer.c_str());

    return true;
}

void GLESToGLConverter::shutdown() {
    if (!initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(cacheMutex);
    functionCache.clear();
    initialized = false;
}

void* GLESToGLConverter::getProcAddress(const char* name) {
    if (!initialized) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = functionCache.find(name);
    if (it != functionCache.end()) {
        return it->second;
    }

    void* address = loadFunction(name);
    functionCache[name] = address;
    return address;
}

bool GLESToGLConverter::isGLES3() const {
    return majorVersion >= 3;
}

bool GLESToGLConverter::isGLES32() const {
    return majorVersion >= 3 && minorVersion >= 2;
}

int GLESToGLConverter::getMajorVersion() const {
    return majorVersion;
}

int GLESToGLConverter::getMinorVersion() const {
    return minorVersion;
}

std::string GLESToGLConverter::getVendor() {
    return vendor;
}

std::string GLESToGLConverter::getRenderer() {
    return renderer;
}

std::string GLESToGLConverter::getVersion() {
    return version;
}

void GLESToGLConverter::detectGLESVersion() {
    majorVersion = 3;
    minorVersion = 0;

    const char* versionStr = (const char*)glGetString(GL_VERSION);
    if (versionStr) {
        version = versionStr;
        
        if (strncmp(versionStr, "OpenGL ES ", 10) == 0) {
            const char* ver = versionStr + 10;
            majorVersion = atoi(ver);
            const char* dot = strchr(ver, '.');
            if (dot) {
                minorVersion = atoi(dot + 1);
            }
        }
    } else {
        glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
        glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    }
}

void GLESToGLConverter::loadVendorInfo() {
    const char* v = (const char*)glGetString(GL_VENDOR);
    vendor = v ? v : "Unknown";

    const char* r = (const char*)glGetString(GL_RENDERER);
    renderer = r ? r : "Unknown";

    const char* ver = (const char*)glGetString(GL_VERSION);
    version = ver ? ver : "Unknown";
}

void* GLESToGLConverter::loadFunction(const char* name) {
    void* func = nullptr;

#ifdef ANDROID
    static void* glHandle = nullptr;
    if (!glHandle) {
        glHandle = dlopen("libGLESv3.so", RTLD_LAZY);
    }
    if (glHandle) {
        func = dlsym(glHandle, name);
    }
#endif

    if (!func) {
        func = eglGetProcAddress(name);
    }

    return func;
}

}
