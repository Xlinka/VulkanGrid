#pragma once

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include <vector>
#include <vulkan/vulkan.h>

class VulkanSwapchain {
public:
    VulkanSwapchain(VulkanInstance& instance, VulkanDevice& device, VkSurfaceKHR surface);
    void init();
    void cleanup();
    
    VkSwapchainKHR getSwapchain() const { return swapchain; }
    VkFormat getSwapchainImageFormat() const { return swapchainImageFormat; }
    VkExtent2D getSwapchainExtent() const { return swapchainExtent; }
    std::vector<VkImageView> getSwapchainImageViews() const { return swapchainImageViews; }
    VkSemaphore getImageAvailableSemaphore() const { return imageAvailableSemaphore; }
    VkSemaphore getRenderFinishedSemaphore() const { return renderFinishedSemaphore; }

    VkRenderPass getRenderPass() const { return renderPass; }
    VkCommandBuffer getCommandBuffer() const { return commandBuffer; }

private:
    VulkanInstance& instance;
    VulkanDevice& device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    VkRenderPass renderPass;
    VkCommandBuffer commandBuffer;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void createRenderPass();  // Function to create the render pass
    void createCommandBuffer();  // Function to create the command buffer
};
