#include "VulkanBridge.h"
#include "../util/Logger.h"
#include <dlfcn.h>

namespace PrismGL {

VulkanBridge::VulkanBridge() : instance(VK_NULL_HANDLE), 
    physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE), 
    graphicsQueue(VK_NULL_HANDLE), available(false),
    angleAvailable(false), zinkAvailable(false) {}

VulkanBridge::~VulkanBridge() {
    shutdown();
}

void VulkanBridge::initialize() {
    available = checkVulkanAvailability();
    if (available) {
        angleAvailable = checkANGLE();
        zinkAvailable = checkZink();
        LOGI("Vulkan available: %s, ANGLE: %s, Zink: %s", 
            available ? "yes" : "no",
            angleAvailable ? "yes" : "no",
            zinkAvailable ? "yes" : "no");
    } else {
        LOGW("Vulkan not available on this device");
    }
}

bool VulkanBridge::checkVulkanAvailability() {
    return vkEnumerateInstanceExtensionProperties != nullptr;
}

bool VulkanBridge::checkANGLE() {
    void* lib = dlopen("libEGL.so", RTLD_NOW);
    if (!lib) return false;
    dlclose(lib);
    
    const char* renderer = getenv("EGL_PLATFORM");
    return renderer && strstr(renderer, "angle") != nullptr;
}

bool VulkanBridge::checkZink() {
    void* lib = dlopen("libEGL.so", RTLD_NOW);
    if (!lib) return false;
    dlclose(lib);
    
    const char* renderer = getenv("MESA_LOADER_DRIVER_OVERRIDE");
    return renderer && strstr(renderer, "zink") != nullptr;
}

void VulkanBridge::shutdown() {
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
    physicalDevice = VK_NULL_HANDLE;
    graphicsQueue = VK_NULL_HANDLE;
    available = false;
}

bool VulkanBridge::isAvailable() const {
    return available;
}

bool VulkanBridge::isAngleAvailable() const {
    return angleAvailable;
}

bool VulkanBridge::isZinkAvailable() const {
    return zinkAvailable;
}

void* VulkanBridge::getProcAddress(const char* name) {
    if (device != VK_NULL_HANDLE) {
        return (void*)vkGetDeviceProcAddr(device, name);
    }
    if (instance != VK_NULL_HANDLE) {
        return (void*)vkGetInstanceProcAddr(instance, name);
    }
    return nullptr;
}

}
