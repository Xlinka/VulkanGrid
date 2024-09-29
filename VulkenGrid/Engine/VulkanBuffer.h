#pragma once

#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>

class VulkanBuffer {
public:
    static void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, 
                             VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                             VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    static void cleanup(VkDevice device, VkBuffer buffer, VkDeviceMemory bufferMemory);

private:
    static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    static void logMemoryInfo(const char* action, VkDeviceSize size);
};
