#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

namespace PrismGL {

struct Texture {
    unsigned int id = 0;
    int width = 0;
    int height = 0;
    int channels = 4;
    bool isMipmapped = false;
    bool anisotropyEnabled = false;
    std::string path;
};

class TextureManager {
public:
    explicit TextureManager(bool asyncLoading = true);
    ~TextureManager();
    
    unsigned int loadTexture(const std::string& path, bool generateMipmaps = true);
    unsigned int createTexture(int width, int height, int channels, const void* data);
    
    void bindTexture(unsigned int texId, int slot = 0);
    void unbindTexture(int slot = 0);
    
    void setAnisotropy(float level);
    void setFilter(int minFilter, int magFilter);
    
    void deleteTexture(unsigned int texId);
    void clearAll();
    
    int getTextureCount() const;
    bool isLoading() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}
