#include "Logger.h"

namespace PrismGL {

const char* Logger::tag = "PrismGL";
int Logger::level = ANDROID_LOG_INFO;

void Logger::init(const char* newTag) {
    tag = newTag;
}

void Logger::setLevel(int newLevel) {
    level = newLevel;
}

}
