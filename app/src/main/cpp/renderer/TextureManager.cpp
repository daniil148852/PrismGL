#include "TextureManager.h"
#include "../util/Logger.h"
#include <GLES3/gl3.h>
#include <stb_image.h>
#include <fstream>

namespace PrismGL {

struct TextureManager::Impl {
    std::unordered_map<std::string, Texture> textureCache;
    std::unordered_map<int, unsigned int> boundTextures;
    bool asyncLoading = true;
    std::atomic<int> loadingCount{0};
    float anisotropyLevel = 16.0f;
    int minFilter = GL_LINEAR_MIPMAP_LINEAR;
    int magFilter = GL_LINEAR;
};

TextureManager::TextureManager(bool asyncLoading) : pImpl(std::make_unique<Impl>()) {
    pImpl->asyncLoading = asyncLoading;
    glGetFloat32v(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &pImpl->anisotropyLevel);
}

TextureManager::~TextureManager() {
    clearAll();
}

unsigned int TextureManager::loadTexture(const std::string& path, bool generateMipmaps) {
    auto it = pImpl->textureCache.find(path);
    if (it != pImpl->textureCache.end()) {
        return it->second.id;
    }
    
    pImpl->loadingCount++;
    
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        LOGE("Failed to load texture: %s", path.c_str());
        pImpl->loadingCount--;
        return 0;
    }
    
    unsigned int texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    
    GLenum format = channels == 4 ? GL_RGBA : (channels == 3 ? GL_RGB : GL_LUMINANCE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    if (generateMipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pImpl->minFilter);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pImpl->minFilter == GL_NEAREST_MIPMAP_NEAREST ? GL_NEAREST : GL_LINEAR);
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, pImpl->magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    if (pImpl->anisotropyLevel > 1.0f) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, pImpl->anisotropyLevel);
    }
    
    stbi_image_free(data);
    
    Texture tex;
    tex.id = texId;
    tex.width = width;
    tex.height = height;
    tex.channels = channels;
    tex.isMipmapped = generateMipmaps;
    tex.is anisotropyEnabled = pImpl->anisotropyLevel > 1.0f;
    tex.path = path;
    
    pImpl->textureCache[path] = tex;
    pImpl->loadingCount--;
    
    LOGI("Loaded texture: %s (%dx%d)", path.c_str(), width, height);
    return texId;
}

unsigned int TextureManager::createTexture(int width, int height, int channels, const void* data) {
    unsigned int texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    
    GLenum internalFormat = (channels == 4) ? GL_RGBA : (channels == 3) ? GL_RGB : GL_LUMINANCE;
    GLenum format = internalFormat;
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return texId;
}

void TextureManager::bindTexture(unsigned int texId, int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texId);
    pImpl->boundTextures[slot] = texId;
}

void TextureManager::unbindTexture(int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
    pImpl->boundTextures.erase(slot);
}

void TextureManager::setAnisotropy(float level) {
    pImpl->anisotropyLevel = std::min(level, 16.0f);
    for (auto& [path, tex] : pImpl->textureCache) {
        if (tex.is anisotropyEnabled) {
            glBindTexture(GL_TEXTURE_2D, tex.id);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, pImpl->anisotropyLevel);
        }
    }
}

void TextureManager::setFilter(int minFilter, int magFilter) {
    pImpl->minFilter = minFilter;
    pImpl->magFilter = magFilter;
    for (auto& [path, tex] : pImpl->textureCache) {
        glBindTexture(GL_TEXTURE_2D, tex.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    }
}

void TextureManager::deleteTexture(unsigned int texId) {
    for (auto it = pImpl->textureCache.begin(); it != pImpl->textureCache.end(); ++it) {
        if (it->second.id == texId) {
            glDeleteTextures(1, &texId);
            pImpl->textureCache.erase(it);
            return;
        }
    }
}

void TextureManager::clearAll() {
    for (auto& [path, tex] : pImpl->textureCache) {
        glDeleteTextures(1, &tex.id);
    }
    pImpl->textureCache.clear();
    pImpl->boundTextures.clear();
}

int TextureManager::getTextureCount() const {
    return (int)pImpl->textureCache.size();
}

bool TextureManager::isLoading() const {
    return pImpl->loadingCount > 0;
}

}
