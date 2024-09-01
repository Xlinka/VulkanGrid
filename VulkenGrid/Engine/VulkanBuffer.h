#pragma once

#include <vulkan/vulkan.h>

class VulkanBuffer {
public:
    void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void cleanup(VkDevice device, VkBuffer buffer, VkDeviceMemory bufferMemory);

private:
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
