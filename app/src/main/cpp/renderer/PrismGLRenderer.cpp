#include "PrismGLRenderer.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "DrawCallOptimizer.h"
#include "AdaptiveRenderer.h"
#include "../gl/GLWrapper.h"
#include "../gl/VulkanBridge.h"
#include "../util/Logger.h"
#include "../util/FileCache.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <vector>
#include <chrono>

namespace PrismGL {

struct Renderer::Impl {
    RenderConfig config;
    bool initialized = false;
    RenderBackend currentBackend = RenderBackend::GLES3;
    GPUMetrics metrics{};
    
    std::unique_ptr<GLWrapper> glWrapper;
    std::unique_ptr<VulkanBridge> vulkanBridge;
    std::unique_ptr<ShaderManager> shaderManager;
    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<DrawCallOptimizer> drawOptimizer;
    std::unique_ptr<AdaptiveRenderer> adaptiveRenderer;
    std::unique_ptr<FileCache> shaderCache;
    
    float resolutionScale = 1.0f;
    int targetFPS = 60;
    std::string deviceProfile;
    
    std::chrono::time_point<std::chrono::high_resolution_clock> frameStart;
    int frameCount = 0;
    float fpsAccumulator = 0.0f;
    
    std::string detectDeviceProfile() {
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        if (!renderer) return "generic";
        
        std::string device(renderer);
        if (device.find("Adreno") != std::string::npos) {
            if (device.find("6") != std::string::npos || device.find("7") != std::string::npos)
                return "adreno_high";
            return "adreno_mid";
        }
        if (device.find("Mali") != std::string::npos) {
            if (device.find("G7") != std::string::npos || device.find("G8") != std::string::npos)
                return "mali_high";
            return "mali_mid";
        }
        if (device.find("PowerVR") != std::string::npos) return "powervr";
        if (device.find("Intel") != std::string::npos) return "intel";
        return "generic";
    }
    
    RenderBackend selectBestBackend() {
        if (config.preferredBackend != RenderBackend::AUTO) {
            return config.preferredBackend;
        }
        
        if (vulkanBridge && vulkanBridge->isAvailable()) {
            return RenderBackend::VULKAN_ANGLE;
        }
        
        return RenderBackend::GLES3;
    }
};

Renderer::Renderer() : pImpl(std::make_unique<Impl>()) {}

Renderer::~Renderer() = default;

bool Renderer::initialize(const RenderConfig& config) {
    pImpl->config = config;
    
    Logger::init("PrismGL");
    LOGI("Initializing PrismGL Renderer...");
    
    pImpl->glWrapper = std::make_unique<GLWrapper>();
    if (!pImpl->glWrapper->initialize()) {
        LOGE("Failed to initialize GLWrapper");
        return false;
    }
    
    pImpl->vulkanBridge = std::make_unique<VulkanBridge>();
    pImpl->vulkanBridge->initialize();
    
    pImpl->currentBackend = pImpl->selectBestBackend();
    LOGI("Selected backend: %d", (int)pImpl->currentBackend);
    
    pImpl->deviceProfile = pImpl->detectDeviceProfile();
    LOGI("Detected device profile: %s", pImpl->deviceProfile.c_str());
    
    pImpl->shaderManager = std::make_unique<ShaderManager>(config.shaderCacheEnabled);
    pImpl->textureManager = std::make_unique<TextureManager>(config.asyncTextureLoading);
    pImpl->drawOptimizer = std::make_unique<DrawCallOptimizer>();
    pImpl->adaptiveRenderer = std::make_unique<AdaptiveRenderer>(config);
    
    pImpl->shaderCache = std::make_unique<FileCache>("/sdcard/PrismGL/shaders");
    pImpl->shaderManager->setCache(pImpl->shaderCache.get());
    
    pImpl->targetFPS = config.targetFPS;
    pImpl->resolutionScale = (float)config.maxResolutionScale;
    
    pImpl->initialized = true;
    LOGI("PrismGL initialized successfully");
    return true;
}

void Renderer::shutdown() {
    if (!pImpl->initialized) return;
    
    pImpl->shaderManager.reset();
    pImpl->textureManager.reset();
    pImpl->drawOptimizer.reset();
    pImpl->adaptiveRenderer.reset();
    pImpl->glWrapper.reset();
    pImpl->vulkanBridge.reset();
    
    pImpl->initialized = false;
    LOGI("PrismGL shutdown complete");
}

bool Renderer::isInitialized() const {
    return pImpl->initialized;
}

void Renderer::beginFrame() {
    pImpl->frameStart = std::chrono::high_resolution_clock::now();
    pImpl->glWrapper->clear();
}

void Renderer::endFrame() {
    auto frameEnd = std::chrono::high_resolution_clock::now();
    float frameTime = std::chrono::duration<float, std::milli>(frameEnd - pImpl->frameStart).count();
    
    pImpl->metrics.frameTime = frameTime;
    pImpl->frameCount++;
    pImpl->fpsAccumulator += frameTime;
    
    if (pImpl->fpsAccumulator >= 1000.0f) {
        pImpl->metrics.drawCallsPerSecond = (float)pImpl->frameCount * 1000.0f / pImpl->fpsAccumulator;
        pImpl->frameCount = 0;
        pImpl->fpsAccumulator = 0.0f;
    }
    
    pImpl->adaptiveRenderer->adjustQuality(pImpl->metrics);
}

void Renderer::setResolutionScale(float scale) {
    pImpl->resolutionScale = std::clamp(scale, 
        (float)pImpl->config.minResolutionScale, 
        (float)pImpl->config.maxResolutionScale);
    pImpl->glWrapper->setViewportScale(pImpl->resolutionScale);
}

float Renderer::getResolutionScale() const {
    return pImpl->resolutionScale;
}

void Renderer::setTargetFPS(int fps) {
    pImpl->targetFPS = std::clamp(fps, 30, 144);
}

int Renderer::getTargetFPS() const {
    return pImpl->targetFPS;
}

void Renderer::enableShaderCache(bool enable) {
    pImpl->config.shaderCacheEnabled = enable;
    if (pImpl->shaderManager) {
        pImpl->shaderManager->enableCache(enable);
    }
}

void Renderer::clearShaderCache() {
    if (pImpl->shaderCache) {
        pImpl->shaderCache->clear();
    }
    if (pImpl->shaderManager) {
        pImpl->shaderManager->clearCache();
    }
}

const GPUMetrics& Renderer::getMetrics() const {
    return pImpl->metrics;
}

RenderBackend Renderer::getCurrentBackend() const {
    return pImpl->currentBackend;
}

std::string Renderer::getDeviceProfile() const {
    return pImpl->deviceProfile;
}

void* Renderer::getProcAddress(const char* name) {
    if (pImpl->currentBackend == RenderBackend::VULKAN_ANGLE || 
        pImpl->currentBackend == RenderBackend::VULKAN_ZINK) {
        return pImpl->vulkanBridge->getProcAddress(name);
    }
    return (void*)eglGetProcAddress(name);
}

}
