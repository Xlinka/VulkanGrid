#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;
class VulkanSwapchain;
class Pipeline;

class RenderPass {
public:
    RenderPass(VulkanDevice& device, VulkanSwapchain& swapchain, VkFormat swapchainImageFormat);
    ~RenderPass();

    VkRenderPass getRenderPass() const;

    void drawFrame(Pipeline* pipeline);

    void cleanup();

private:
    void createRenderPass(VkFormat swapchainImageFormat);
    void createFramebuffers();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, Pipeline* pipeline);

    VulkanDevice& device;
    VulkanSwapchain& swapchain;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
};
