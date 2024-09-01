#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

class Pipeline {
public:
    Pipeline(VulkanDevice& device, VulkanSwapchain& swapchain, VkRenderPass renderPass);
    ~Pipeline();

    void createGraphicsPipeline(VkExtent2D swapchainExtent);  // Updated function signature
    void cleanup();

    VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }

private:
    VulkanDevice& device;
    VulkanSwapchain& swapchain;
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;

    void createPipelineLayout();  
};

