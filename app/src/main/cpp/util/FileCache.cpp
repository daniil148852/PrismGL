#include "FileCache.h"
#include "Logger.h"
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <filesystem>

namespace PrismGL {

FileCache::FileCache(const std::string& path) : basePath(path), currentSize(0), maxSize(100 * 1024 * 1024) {
    mkdir(basePath.c_str(), 0777);
}

FileCache::~FileCache() = default;

std::string FileCache::getFilePath(const std::string& key) const {
    return basePath + "/" + key;
}

bool FileCache::save(const std::string& key, const std::vector<char>& data) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (data.empty()) return false;
    
    std::string path = getFilePath(key);
    FILE* file = fopen(path.c_str(), "wb");
    if (!file) {
        LOGE("Failed to open file for writing: %s", path.c_str());
        return false;
    }
    
    size_t written = fwrite(data.data(), 1, data.size(), file);
    fclose(file);
    
    if (written != data.size()) {
        LOGE("Failed to write all data to file: %s", path.c_str());
        remove(path.c_str());
        return false;
    }
    
    currentSize += data.size();
    LOGI("Saved to cache: %s (%zu bytes)", key.c_str(), data.size());
    return true;
}

bool FileCache::load(const std::string& key, std::vector<char>& data) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::string path = getFilePath(key);
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
        return false;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(file);
        return false;
    }
    
    data.resize(size);
    size_t read = fread(data.data(), 1, size, file);
    fclose(file);
    
    if (read != (size_t)size) {
        return false;
    }
    
    return true;
}

bool FileCache::exists(const std::string& key) const {
    std::string path = getFilePath(key);
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

void FileCache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::string path = getFilePath(key);
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        currentSize -= st.st_size;
        std::remove(path.c_str());
    }
}

void FileCache::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    
    DIR* dir = opendir(basePath.c_str());
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            std::string path = basePath + "/" + entry->d_name;
            std::remove(path.c_str());
        }
    }
    closedir(dir);
    currentSize = 0;
}

size_t FileCache::getSize() const {
    std::lock_guard<std::mutex> lock(mutex);
    return currentSize;
}

size_t FileCache::getMaxSize() const {
    std::lock_guard<std::mutex> lock(mutex);
    return maxSize;
}

void FileCache::setMaxSize(size_t newMaxSize) {
    std::lock_guard<std::mutex> lock(mutex);
    maxSize = newMaxSize;
}

}
