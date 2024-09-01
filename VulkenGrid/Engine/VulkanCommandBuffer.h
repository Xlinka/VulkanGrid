#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanCommandBuffer {
public:
    void createCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, std::vector<VkCommandBuffer>& commandBuffers);
    void beginCommandBuffer(VkCommandBuffer commandBuffer);
    void endCommandBuffer(VkCommandBuffer commandBuffer);
    void cleanup(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>& commandBuffers);
};
