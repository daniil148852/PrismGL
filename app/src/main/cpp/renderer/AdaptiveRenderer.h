#pragma once

#include "PrismGLRenderer.h"
#include <vector>

namespace PrismGL {

enum class QualityLevel {
    ULTRA,
    HIGH,
    MEDIUM,
    LOW,
    MINIMAL
};

class AdaptiveRenderer {
public:
    explicit AdaptiveRenderer(const RenderConfig& config);
    ~AdaptiveRenderer();
    
    void adjustQuality(const GPUMetrics& metrics);
    
    void setQualityLevel(QualityLevel level);
    QualityLevel getQualityLevel() const;
    
    void setTargetFrameTime(float ms);
    float getTargetFrameTime() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    QualityLevel evaluateQuality(const GPUMetrics& metrics);
    void applyQualitySettings(QualityLevel level);
};

}
