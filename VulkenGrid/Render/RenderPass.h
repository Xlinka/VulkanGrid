#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

class RenderPass {
public:
    RenderPass(VulkanDevice& device, VulkanSwapchain& swapchain, VkFormat swapchainImageFormat);
    ~RenderPass();
    
    VkRenderPass getRenderPass() const;
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void drawFrame();

private:
    VulkanDevice& device;
    VulkanSwapchain& swapchain;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    void createRenderPass(VkFormat swapchainImageFormat);
    void createFramebuffers();
    void createCommandBuffers();
};