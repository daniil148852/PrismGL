#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace PrismGL {

enum class RenderBackend {
    GLES3,
    VULKAN_ANGLE,
    VULKAN_ZINK,
    AUTO
};

struct RenderConfig {
    int targetFPS = 60;
    int maxResolutionScale = 2;
    int minResolutionScale = 1;
    bool shaderCacheEnabled = true;
    bool asyncTextureLoading = true;
    bool bindlessTextures = false;
    bool debugMode = false;
    RenderBackend preferredBackend = RenderBackend::AUTO;
    std::string deviceProfile;
};

struct GPUMetrics {
    float frameTime;
    float drawCallsPerSecond;
    int trianglesPerFrame;
    int texturesBound;
    int shaderSwitches;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize(const RenderConfig& config);
    void shutdown();
    bool isInitialized() const;

    void beginFrame();
    void endFrame();
    
    void setResolutionScale(float scale);
    float getResolutionScale() const;
    
    void setTargetFPS(int fps);
    int getTargetFPS() const;

    void enableShaderCache(bool enable);
    void clearShaderCache();

    const GPUMetrics& getMetrics() const;
    
    RenderBackend getCurrentBackend() const;
    std::string getDeviceProfile() const;

    void* getProcAddress(const char* name);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}
