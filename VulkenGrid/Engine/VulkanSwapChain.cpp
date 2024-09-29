#include "VulkanSwapChain.h"
#include "Logger.h"
#include <stdexcept>
#include <algorithm>

VulkanSwapchain::VulkanSwapchain(VulkanInstance& instance, VulkanDevice& device, VkSurfaceKHR surface)
    : instance(instance), device(device), surface(surface) {}

void VulkanSwapchain::init() {
    Logger::getInstance().log("Initializing Vulkan Swapchain...");

    SwapChainSupportDetails swapChainSupport = device.querySwapChainSupport(surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format; // Updated to use the selected format
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

    createRenderPass();  // Initialize render pass
    createFramebuffers(); // Initialize framebuffers
    createCommandBuffer();  // Initialize command buffer

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
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            Logger::getInstance().log("Selected swap surface format: VK_FORMAT_B8G8R8A8_SRGB");
            return availableFormat; // Return preferred format
        }
    }
    
    // Log fallback format
    Logger::getInstance().log("Fallback to first available surface format.");
    return availableFormats[0]; // Fallback to the first format if preferred isn't available
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode; // Use mailbox mode if available
        }
    }
    
    Logger::getInstance().log("Using FIFO present mode as fallback.");
    return VK_PRESENT_MODE_FIFO_KHR; // Default to FIFO if no other present modes are available
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        VkExtent2D actualExtent = { 800, 600 }; // Default window size
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

void VulkanSwapchain::createRenderPass() {
    Logger::getInstance().log("Creating RenderPass for Swapchain...");

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat; // Use the selected swapchain image format
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // This is correct: loadOp is of type VkAttachmentLoadOp
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // This is correct: storeOp is of type VkAttachmentStoreOp
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Correct: same type
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Correct: same type
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(device.getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create RenderPass!");
        throw std::runtime_error("Failed to create RenderPass!");
    }

    Logger::getInstance().log("RenderPass created successfully.");
}

void VulkanSwapchain::createFramebuffers() {
    Logger::getInstance().log("Creating Framebuffers for Swapchain...");

    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        VkImageView attachments[] = { swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device.getDevice(), &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
            Logger::getInstance().logError("Failed to create framebuffer!");
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }

    Logger::getInstance().log("Framebuffers created successfully.");
}

void VulkanSwapchain::createCommandBuffer() {
    Logger::getInstance().log("Creating Command Buffer for Swapchain...");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getCommandPool();  // Assume command pool is managed in VulkanDevice
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device.getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to allocate command buffer!");
        throw std::runtime_error("Failed to allocate command buffer!");
    }

    Logger::getInstance().log("Command Buffer created successfully.");
}
