#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../util/FileCache.h"

namespace PrismGL {

struct ShaderProgram {
    unsigned int programId = 0;
    unsigned int vertexShader = 0;
    unsigned int fragmentShader = 0;
    bool isBinary = false;
    std::string sourceHash;
    int useCount = 0;
};

class ShaderManager {
public:
    explicit ShaderManager(bool cacheEnabled = true);
    ~ShaderManager();
    
    void setCache(FileCache* cache);
    void enableCache(bool enable);
    
    unsigned int compileShader(const std::string& vertSource, 
                               const std::string& fragSource,
                               const std::string& defines = "");
    
    void useShader(unsigned int program);
    void unbindShader();
    
    unsigned int getCurrentProgram() const;
    
    void clearCache();
    
    int getUniformLocation(const char* name);
    int getAttribLocation(const char* name);
    
    void setUniform1i(const char* name, int value);
    void setUniform1f(const char* name, float value);
    void setUniform2f(const char* name, float x, float y);
    void setUniform3f(const char* name, float x, float y, float z);
    void setUniform4f(const char* name, float x, float y, float z, float w);
    void setUniformMatrix4fv(const char* name, const float* value);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    std::string generateHash(const std::string& vert, const std::string& frag, const std::string& defines);
    unsigned int loadFromCache(const std::string& hash);
    void saveToCache(const std::string& hash, unsigned int program);
    unsigned int compileGLShader(int type, const std::string& source);
    bool linkProgram(unsigned int vert, unsigned int frag, unsigned int& program);
};

}
