#pragma once

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include <vector>
#include <vulkan/vulkan.h>

class VulkanRenderer {
public:
    VulkanRenderer(VulkanDevice& device, VulkanSwapchain& swapchain);
    void init();
    void drawFrame();
    void cleanup();
    bool shouldClose();

private:
    VulkanDevice& device;
    VulkanSwapchain& swapchain;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    void createRenderPass();
    void createFramebuffers();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};
