#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "ShaderModule.h"

class Pipeline {
public:
    Pipeline(VulkanDevice& device, VulkanSwapchain& swapchain, VkRenderPass renderPass);
    ~Pipeline();

    void createGraphicsPipeline(VkExtent2D swapchainExtent, const std::vector<ShaderModule>& shaderModules);
    void cleanup();

private:
    VulkanDevice& device;
    VulkanSwapchain& swapchain;
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;

    VkShaderModule createShaderModule(const std::vector<char>& code);
};
