#pragma once

#include <android/log.h>

#define LOG_TAG "PrismGL"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace PrismGL {

class Logger {
public:
    static void init(const char* tag);
    static void setLevel(int level);
    
    static int getLevel() { return level; }
    
private:
    static const char* tag;
    static int level;
};

}
