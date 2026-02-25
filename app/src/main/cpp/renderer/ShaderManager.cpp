#include "ShaderManager.h"
#include "../util/Logger.h"
#include <GLES3/gl3.h>
#include <chrono>
#include <fstream>

namespace PrismGL {

struct ShaderManager::Impl {
    std::unordered_map<std::string, ShaderProgram> shaderCache;
    unsigned int currentProgram = 0;
    FileCache* cache = nullptr;
    bool cacheEnabled = true;
    std::unordered_map<std::string, int> uniformLocations;
    std::unordered_map<std::string, int> attribLocations;
};

ShaderManager::ShaderManager(bool cacheEnabled) : pImpl(std::make_unique<Impl>()) {
    pImpl->cacheEnabled = cacheEnabled;
}

ShaderManager::~ShaderManager() {
    for (auto& [hash, shader] : pImpl->shaderCache) {
        if (shader.programId) glDeleteProgram(shader.programId);
        if (shader.vertexShader) glDeleteShader(shader.vertexShader);
        if (shader.fragmentShader) glDeleteShader(shader.fragmentShader);
    }
}

void ShaderManager::setCache(FileCache* cache) {
    pImpl->cache = cache;
}

void ShaderManager::enableCache(bool enable) {
    pImpl->cacheEnabled = enable;
}

std::string ShaderManager::generateHash(const std::string& vert, const std::string& frag, const std::string& defines) {
    std::string combined = vert + "|" + frag + "|" + defines;
    std::hash<std::string> hasher;
    size_t hash = hasher(combined);
    return std::to_string(hash);
}

unsigned int ShaderManager::loadFromCache(const std::string& hash) {
    if (!pImpl->cache || !pImpl->cacheEnabled) return 0;
    
    std::vector<char> data;
    if (pImpl->cache->load(hash, data)) {
        LOGI("Shader cache loading not fully supported on GLES3, recompiling: %s", hash.c_str());
        return 0;
    }
    return 0;
}

void ShaderManager::saveToCache(const std::string& hash, unsigned int program) {
    if (!pImpl->cache || !pImpl->cacheEnabled || !program) return;
    
    LOGI("Shader binary cache not supported on GLES3, skipping: %s", hash.c_str());
}

unsigned int ShaderManager::compileGLShader(int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        LOGE("Shader compilation failed: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool ShaderManager::linkProgram(unsigned int vert, unsigned int frag, unsigned int& program) {
    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        LOGE("Program linking failed: %s", log);
        glDeleteProgram(program);
        return false;
    }
    return true;
}

unsigned int ShaderManager::compileShader(const std::string& vertSource, 
                                           const std::string& fragSource,
                                           const std::string& defines) {
    std::string hash = generateHash(vertSource, fragSource, defines);
    
    auto it = pImpl->shaderCache.find(hash);
    if (it != pImpl->shaderCache.end()) {
        it->second.useCount++;
        return it->second.programId;
    }
    
    unsigned int cachedProgram = loadFromCache(hash);
    if (cachedProgram) {
        ShaderProgram sp;
        sp.programId = cachedProgram;
        sp.sourceHash = hash;
        sp.isBinary = true;
        sp.useCount = 1;
        pImpl->shaderCache[hash] = sp;
        return cachedProgram;
    }
    
    std::string vertWithDefines = defines.empty() ? vertSource : "#define " + defines + "\n" + vertSource;
    std::string fragWithDefines = defines.empty() ? fragSource : "#define " + defines + "\n" + fragSource;
    
    unsigned int vert = compileGLShader(GL_VERTEX_SHADER, vertWithDefines);
    unsigned int frag = compileGLShader(GL_FRAGMENT_SHADER, fragWithDefines);
    if (!vert || !frag) {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return 0;
    }
    
    unsigned int program;
    if (!linkProgram(vert, frag, program)) {
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }
    
    ShaderProgram sp;
    sp.programId = program;
    sp.vertexShader = vert;
    sp.fragmentShader = frag;
    sp.sourceHash = hash;
    sp.useCount = 1;
    pImpl->shaderCache[hash] = sp;
    
    saveToCache(hash, program);
    
    LOGI("Compiled new shader: %s", hash.c_str());
    return program;
}

void ShaderManager::useShader(unsigned int program) {
    if (pImpl->currentProgram != program) {
        glUseProgram(program);
        pImpl->currentProgram = program;
        pImpl->uniformLocations.clear();
        pImpl->attribLocations.clear();
    }
}

void ShaderManager::unbindShader() {
    glUseProgram(0);
    pImpl->currentProgram = 0;
}

unsigned int ShaderManager::getCurrentProgram() const {
    return pImpl->currentProgram;
}

void ShaderManager::clearCache() {
    for (auto& [hash, shader] : pImpl->shaderCache) {
        if (shader.programId) glDeleteProgram(shader.programId);
        if (shader.vertexShader) glDeleteShader(shader.vertexShader);
        if (shader.fragmentShader) glDeleteShader(shader.fragmentShader);
    }
    pImpl->shaderCache.clear();
    if (pImpl->cache) {
        pImpl->cache->clear();
    }
}

int ShaderManager::getUniformLocation(const char* name) {
    auto it = pImpl->uniformLocations.find(name);
    if (it != pImpl->uniformLocations.end()) {
        return it->second;
    }
    int loc = glGetUniformLocation(pImpl->currentProgram, name);
    pImpl->uniformLocations[name] = loc;
    return loc;
}

int ShaderManager::getAttribLocation(const char* name) {
    auto it = pImpl->attribLocations.find(name);
    if (it != pImpl->attribLocations.end()) {
        return it->second;
    }
    int loc = glGetAttribLocation(pImpl->currentProgram, name);
    pImpl->attribLocations[name] = loc;
    return loc;
}

void ShaderManager::setUniform1i(const char* name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void ShaderManager::setUniform1f(const char* name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void ShaderManager::setUniform2f(const char* name, float x, float y) {
    glUniform2f(getUniformLocation(name), x, y);
}

void ShaderManager::setUniform3f(const char* name, float x, float y, float z) {
    glUniform3f(getUniformLocation(name), x, y, z);
}

void ShaderManager::setUniform4f(const char* name, float x, float y, float z, float w) {
    glUniform4f(getUniformLocation(name), x, y, z, w);
}

void ShaderManager::setUniformMatrix4fv(const char* name, const float* value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, value);
}

}
