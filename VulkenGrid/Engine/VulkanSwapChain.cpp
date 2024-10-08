#include "VulkanSwapChain.h"
#include "Logger.h"
#include <stdexcept>
#include <algorithm>

VulkanSwapchain::VulkanSwapchain(VulkanInstance& instance, VulkanDevice& device, VkSurfaceKHR surface)
    : instance(instance), device(device), surface(surface) {}

void VulkanSwapchain::init() {
    Logger::getInstance().log("Initializing Vulkan Swapchain...");

    SwapChainSupportDetails swapChainSupport = device.querySwapChainSupport(surface);

    Logger::getInstance().log("Available swapchain formats: " + std::to_string(swapChainSupport.formats.size()));
    if (swapChainSupport.formats.empty()) {
        Logger::getInstance().logError("No available swapchain formats!");
        throw std::runtime_error("No available swapchain formats!");
    }

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    Logger::getInstance().log("Chosen surface format: " + std::to_string(surfaceFormat.format));

    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    Logger::getInstance().log("Chosen present mode: " + std::to_string(presentMode));

    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    Logger::getInstance().log("Chosen swap extent: " + std::to_string(extent.width) + "x" + std::to_string(extent.height));

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { device.getQueueFamilyIndices().graphicsFamily.value(), device.getQueueFamilyIndices().presentFamily.value() };

    if (device.getQueueFamilyIndices().graphicsFamily != device.getQueueFamilyIndices().presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create swapchain!");
        throw std::runtime_error("Failed to create swapchain!");
    }

    vkGetSwapchainImagesKHR(device.getDevice(), swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device.getDevice(), swapchain, &imageCount, swapchainImages.data());

    swapchainImageFormat = surfaceFormat.format;
    Logger::getInstance().log("Swapchain image format selected: " + std::to_string(swapchainImageFormat));

    swapchainExtent = extent;

    Logger::getInstance().log("Creating image views...");
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.getDevice(), &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            Logger::getInstance().logError("Failed to create image views!");
            throw std::runtime_error("Failed to create image views!");
        }
    }

    Logger::getInstance().log("Creating semaphores...");
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create semaphores!");
        throw std::runtime_error("Failed to create semaphores!");
    }

    Logger::getInstance().log("Vulkan Swapchain and associated resources initialized successfully.");
}

void VulkanSwapchain::cleanup() {
    Logger::getInstance().log("Cleaning up Vulkan Swapchain...");

    for (auto framebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(device.getDevice(), framebuffer, nullptr);
    }

    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device.getDevice(), imageView, nullptr);
    }

    vkDestroySwapchainKHR(device.getDevice(), swapchain, nullptr);
    vkDestroySemaphore(device.getDevice(), imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device.getDevice(), renderFinishedSemaphore, nullptr);

    Logger::getInstance().log("Vulkan Swapchain and associated resources cleaned up successfully.");
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    Logger::getInstance().log("Choosing swap surface format from available formats...");
    for (const auto& availableFormat : availableFormats) {
        Logger::getInstance().log("Available format: format=" + std::to_string(availableFormat.format) + ", colorSpace=" + std::to_string(availableFormat.colorSpace));
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            Logger::getInstance().log("Selected swap surface format: VK_FORMAT_B8G8R8A8_SRGB");
            return availableFormat;
        }
    }

    Logger::getInstance().log("Fallback to first available surface format: format=" + std::to_string(availableFormats[0].format) + ", colorSpace=" + std::to_string(availableFormats[0].colorSpace));
    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    Logger::getInstance().log("Choosing swap present mode from available present modes...");
    for (const auto& availablePresentMode : availablePresentModes) {
        Logger::getInstance().log("Available present mode: " + std::to_string(availablePresentMode));
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            Logger::getInstance().log("Selected present mode: VK_PRESENT_MODE_MAILBOX_KHR");
            return availablePresentMode;
        }
    }

    Logger::getInstance().log("Using FIFO present mode as fallback: VK_PRESENT_MODE_FIFO_KHR");
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    Logger::getInstance().log("Choosing swap extent...");
    if (capabilities.currentExtent.width != UINT32_MAX) {
        Logger::getInstance().log("Using current extent: " + std::to_string(capabilities.currentExtent.width) + "x" + std::to_string(capabilities.currentExtent.height));
        return capabilities.currentExtent;
    }
    else {
        VkExtent2D actualExtent = { 800, 600 };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        Logger::getInstance().log("Calculated extent: " + std::to_string(actualExtent.width) + "x" + std::to_string(actualExtent.height));
        return actualExtent;
    }
}