#pragma once

#include <vulkan/vulkan.h>

class RenderPass {
public:
    RenderPass(VkDevice device, VkFormat swapchainImageFormat);
    ~RenderPass();

    VkRenderPass getRenderPass() const { return renderPass; }

private:
    VkDevice device;
    VkRenderPass renderPass;

    void createRenderPass(VkFormat swapchainImageFormat);
};
