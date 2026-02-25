#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace PrismGL {

class VulkanBridge {
public:
    VulkanBridge();
    ~VulkanBridge();
    
    void initialize();
    void shutdown();
    
    bool isAvailable() const;
    bool isAngleAvailable() const;
    bool isZinkAvailable() const;
    
    void* getProcAddress(const char* name);
    
    VkInstance getInstance() const { return instance; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkDevice getDevice() const { return device; }

private:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    bool available;
    bool angleAvailable;
    bool zinkAvailable;
    
    bool checkVulkanAvailability();
    bool checkANGLE();
    bool checkZink();
};

}
