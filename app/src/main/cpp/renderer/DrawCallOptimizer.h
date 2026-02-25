#pragma once

#include <vector>
#include <cstdint>
#include <unordered_map>

namespace PrismGL {

struct DrawCommand {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
    uint32_t vao;
    uint32_t indexBuffer;
    uint32_t indexType;
    uint32_t primitiveType;
};

struct BatchGroup {
    std::vector<DrawCommand> commands;
    uint32_t materialId;
    uint32_t textureId;
};

class DrawCallOptimizer {
public:
    DrawCallOptimizer();
    ~DrawCallOptimizer();
    
    void beginBatch();
    void addDrawCall(const DrawCommand& cmd);
    void endBatch();
    
    void executeBatched();
    
    void enableInstancing(bool enable);
    void enableBindlessTextures(bool enable);
    
    void sortByMaterial();
    void sortByTexture();
    
    int getBatchCount() const;
    int getDrawCallCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}
