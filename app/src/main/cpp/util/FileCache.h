#pragma once

#include <string>
#include <vector>
#include <mutex>

namespace PrismGL {

class FileCache {
public:
    explicit FileCache(const std::string& basePath);
    ~FileCache();
    
    bool save(const std::string& key, const std::vector<char>& data);
    bool load(const std::string& key, std::vector<char>& data);
    bool exists(const std::string& key) const;
    void remove(const std::string& key);
    void clear();
    
    size_t getSize() const;
    size_t getMaxSize() const;
    void setMaxSize(size_t maxSize);

private:
    std::string basePath;
    size_t currentSize;
    size_t maxSize;
    mutable std::mutex mutex;
    
    std::string getFilePath(const std::string& key) const;
};

}
