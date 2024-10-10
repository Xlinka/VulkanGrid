#pragma once

#include <vulkan/vulkan.h>

class VulkanDevice;
class VulkanSwapchain;

class Pipeline {
public:
    Pipeline(VulkanDevice& device, VulkanSwapchain& swapchain, VkRenderPass renderPass);
    ~Pipeline();

    void createGraphicsPipeline(VkExtent2D swapchainExtent);
    void cleanup();

    VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }

private:
    VulkanDevice& device;
    VulkanSwapchain& swapchain;
    VkRenderPass renderPass;

    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
};
