#include "DrawCallOptimizer.h"
#include "../util/Logger.h"
#include <GLES3/gl3.h>
#include <algorithm>

namespace PrismGL {

struct DrawCallOptimizer::Impl {
    std::vector<BatchGroup> batches;
    std::vector<DrawCommand> pendingCommands;
    bool instancingEnabled = true;
    bool bindlessTexturesEnabled = false;
    uint32_t currentVAO = 0;
    uint32_t currentIndexBuffer = 0;
};

DrawCallOptimizer::DrawCallOptimizer() : pImpl(std::make_unique<Impl>()) {}

DrawCallOptimizer::~DrawCallOptimizer() = default;

void DrawCallOptimizer::beginBatch() {
    pImpl->pendingCommands.clear();
}

void DrawCallOptimizer::addDrawCall(const DrawCommand& cmd) {
    pImpl->pendingCommands.push_back(cmd);
}

void DrawCallOptimizer::endBatch() {
    if (pImpl->pendingCommands.empty()) return;
    
    BatchGroup batch;
    batch.commands = pImpl->pendingCommands;
    batch.materialId = pImpl->currentVAO;
    batch.textureId = pImpl->currentIndexBuffer;
    
    pImpl->batches.push_back(std::move(batch));
    pImpl->pendingCommands.clear();
}

void DrawCallOptimizer::executeBatched() {
    for (const auto& batch : pImpl->batches) {
        for (const auto& cmd : batch.commands) {
            if (cmd.vao) {
                glBindVertexArray(cmd.vao);
            }
            if (cmd.indexBuffer) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd.indexBuffer);
            }
            
            if (cmd.instanceCount > 1 && pImpl->instancingEnabled) {
                glDrawElementsInstanced(
                    cmd.primitiveType,
                    cmd.vertexCount,
                    cmd.indexType,
                    (void*)(int64_t)cmd.firstVertex,
                    cmd.instanceCount
                );
            } else if (cmd.indexBuffer) {
                glDrawElements(
                    cmd.primitiveType,
                    cmd.vertexCount,
                    cmd.indexType,
                    (void*)(int64_t)cmd.firstVertex
                );
            } else {
                glDrawArrays(cmd.primitiveType, cmd.firstVertex, cmd.vertexCount);
            }
        }
    }
    
    pImpl->batches.clear();
}

void DrawCallOptimizer::enableInstancing(bool enable) {
    pImpl->instancingEnabled = enable;
}

void DrawCallOptimizer::enableBindlessTextures(bool enable) {
    pImpl->bindlessTexturesEnabled = enable;
    if (enable) {
        LOGI("Bindless textures enabled");
    }
}

void DrawCallOptimizer::sortByMaterial() {
    std::sort(pImpl->batches.begin(), pImpl->batches.end(),
        [](const BatchGroup& a, const BatchGroup& b) {
            return a.materialId < b.materialId;
        });
}

void DrawCallOptimizer::sortByTexture() {
    std::sort(pImpl->batches.begin(), pImpl->batches.end(),
        [](const BatchGroup& a, const BatchGroup& b) {
            return a.textureId < b.textureId;
        });
}

int DrawCallOptimizer::getBatchCount() const {
    return (int)pImpl->batches.size();
}

int DrawCallOptimizer::getDrawCallCount() const {
    int count = 0;
    for (const auto& batch : pImpl->batches) {
        count += (int)batch.commands.size();
    }
    return count;
}

}
