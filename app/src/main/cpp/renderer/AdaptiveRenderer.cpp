#include "AdaptiveRenderer.h"
#include "../util/Logger.h"
#include <algorithm>

namespace PrismGL {

struct AdaptiveRenderer::Impl {
    RenderConfig config;
    QualityLevel currentLevel = QualityLevel::HIGH;
    float targetFrameTime = 16.67f;
    int frameTimeHistorySize = 60;
    std::vector<float> frameTimeHistory;
    int historyIndex = 0;
    int framesAboveTarget = 0;
    int framesBelowTarget = 0;
    bool autoAdjustEnabled = true;
};

AdaptiveRenderer::AdaptiveRenderer(const RenderConfig& config) : pImpl(std::make_unique<Impl>()) {
    pImpl->config = config;
    pImpl->targetFrameTime = 1000.0f / (float)config.targetFPS;
    pImpl->frameTimeHistory.resize(pImpl->frameTimeHistorySize, pImpl->targetFrameTime);
}

AdaptiveRenderer::~AdaptiveRenderer() = default;

void AdaptiveRenderer::adjustQuality(const GPUMetrics& metrics) {
    if (!pImpl->autoAdjustEnabled) return;
    
    pImpl->frameTimeHistory[pImpl->historyIndex] = metrics.frameTime;
    pImpl->historyIndex = (pImpl->historyIndex + 1) % pImpl->frameTimeHistorySize;
    
    float avgFrameTime = 0.0f;
    for (float ft : pImpl->frameTimeHistory) {
        avgFrameTime += ft;
    }
    avgFrameTime /= pImpl->frameTimeHistorySize;
    
    if (avgFrameTime > pImpl->targetFrameTime * 1.1f) {
        pImpl->framesAboveTarget++;
        pImpl->framesBelowTarget = 0;
        
        if (pImpl->framesAboveTarget > 30) {
            QualityLevel newLevel = evaluateQuality(metrics);
            if (newLevel < pImpl->currentLevel) {
                pImpl->currentLevel = newLevel;
                applyQualitySettings(newLevel);
                pImpl->framesAboveTarget = 0;
            }
        }
    } else if (avgFrameTime < pImpl->targetFrameTime * 0.8f) {
        pImpl->framesBelowTarget++;
        pImpl->framesAboveTarget = 0;
        
        if (pImpl->framesBelowTarget > 60) {
            if (pImpl->currentLevel < QualityLevel::ULTRA) {
                int level = (int)pImpl->currentLevel + 1;
                pImpl->currentLevel = (QualityLevel)level;
                applyQualitySettings(pImpl->currentLevel);
                pImpl->framesBelowTarget = 0;
            }
        }
    }
}

QualityLevel AdaptiveRenderer::evaluateQuality(const GPUMetrics& metrics) {
    if (metrics.frameTime < pImpl->targetFrameTime * 0.7f) {
        return QualityLevel::ULTRA;
    } else if (metrics.frameTime < pImpl->targetFrameTime * 0.9f) {
        return QualityLevel::HIGH;
    } else if (metrics.frameTime < pImpl->targetFrameTime * 1.2f) {
        return QualityLevel::MEDIUM;
    } else if (metrics.frameTime < pImpl->targetFrameTime * 1.5f) {
        return QualityLevel::LOW;
    }
    return QualityLevel::MINIMAL;
}

void AdaptiveRenderer::applyQualitySettings(QualityLevel level) {
    switch (level) {
        case QualityLevel::ULTRA:
            pImpl->config.maxResolutionScale = 2;
            break;
        case QualityLevel::HIGH:
            pImpl->config.maxResolutionScale = 2;
            break;
        case QualityLevel::MEDIUM:
            pImpl->config.maxResolutionScale = 1;
            break;
        case QualityLevel::LOW:
            pImpl->config.maxResolutionScale = 1;
            break;
        case QualityLevel::MINIMAL:
            pImpl->config.maxResolutionScale = 0;
            break;
    }
    LOGI("Quality level changed to: %d", (int)level);
}

void AdaptiveRenderer::setQualityLevel(QualityLevel level) {
    pImpl->currentLevel = level;
    applyQualitySettings(level);
}

QualityLevel AdaptiveRenderer::getQualityLevel() const {
    return pImpl->currentLevel;
}

void AdaptiveRenderer::setTargetFrameTime(float ms) {
    pImpl->targetFrameTime = ms;
}

float AdaptiveRenderer::getTargetFrameTime() const {
    return pImpl->targetFrameTime;
}

}
