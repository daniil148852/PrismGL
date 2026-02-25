#pragma once

#include <GLES3/gl3.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace PrismGL {

struct GLFunction {
    void* address = nullptr;
    const char* name = nullptr;
};

class GLESToGLConverter {
public:
    GLESToGLConverter();
    ~GLESToGLConverter();

    bool initialize();
    void shutdown();

    void* getProcAddress(const char* name);
    
    bool isGLES3() const;
    bool isGLES32() const;
    int getMajorVersion() const;
    int getMinorVersion() const;

    std::string getVendor();
    std::string getRenderer();
    std::string getVersion();

private:
    bool initialized;
    int majorVersion;
    int minorVersion;
    std::string vendor;
    std::string renderer;
    std::string version;
    
    std::unordered_map<std::string, void*> functionCache;
    std::mutex cacheMutex;

    void detectGLESVersion();
    void loadVendorInfo();
    void* loadFunction(const char* name);
};

}
