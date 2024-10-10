#include "RenderPass.h"
#include "../Utils/LoggerUtils.h"
#include <stdexcept>

RenderPass::RenderPass(VulkanDevice& device, VulkanSwapchain& swapchain, VkFormat swapchainImageFormat)
    : device(device), swapchain(swapchain), renderPass(VK_NULL_HANDLE) {
    Logger::getInstance().log("Initializing RenderPass...");

    // Initial device check
    if (device.getDevice() == VK_NULL_HANDLE) {
        Logger::getInstance().logError("Device handle is null during RenderPass initialization. Aborting RenderPass creation.");
        throw std::runtime_error("Device handle is null, cannot initialize RenderPass.");
    } else {
        Logger::getInstance().log("Device handle is valid during RenderPass initialization.");
    }

    // Swapchain check
    if (swapchain.getSwapchain() == VK_NULL_HANDLE) {
        Logger::getInstance().logError("Swapchain handle is null during RenderPass initialization. Aborting RenderPass creation.");
        throw std::runtime_error("Swapchain handle is null, cannot initialize RenderPass.");
    } else {
        Logger::getInstance().log("Swapchain handle is valid during RenderPass initialization.");
    }

    Logger::getInstance().log("Received swapchain image format: " + std::to_string(swapchainImageFormat));

    Logger::getInstance().log("Verifying swapchain image format before creating RenderPass...");
    if (swapchainImageFormat == VK_FORMAT_UNDEFINED) {
        Logger::getInstance().logError("Swapchain image format is undefined. Aborting RenderPass creation.");
        throw std::runtime_error("Swapchain image format is undefined, cannot create RenderPass.");
    }
    createRenderPass(swapchainImageFormat);
    createFramebuffers();
    createCommandBuffers();
}

RenderPass::~RenderPass() {
    cleanup();
}

VkRenderPass RenderPass::getRenderPass() const {
    return renderPass;
}

void RenderPass::createRenderPass(VkFormat swapchainImageFormat) {
    Logger::getInstance().log("Creating color attachment description...");

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    Logger::getInstance().log("Color attachment format set to: " + std::to_string(swapchainImageFormat));
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    Logger::getInstance().log("Color attachment samples set to VK_SAMPLE_COUNT_1_BIT");
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    Logger::getInstance().log("Color attachment load operation set to VK_ATTACHMENT_LOAD_OP_CLEAR");
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    Logger::getInstance().log("Color attachment store operation set to VK_ATTACHMENT_STORE_OP_STORE");
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Logger::getInstance().log("Color attachment stencil load operation set to VK_ATTACHMENT_LOAD_OP_DONT_CARE");
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Logger::getInstance().log("Color attachment stencil store operation set to VK_ATTACHMENT_STORE_OP_DONT_CARE");
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    Logger::getInstance().log("Color attachment initial layout set to VK_IMAGE_LAYOUT_UNDEFINED");
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    Logger::getInstance().log("Color attachment final layout set to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR");

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    Logger::getInstance().log("Color attachment reference attachment set to 0");
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    Logger::getInstance().log("Color attachment reference layout set to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL");

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Logger::getInstance().log("Subpass pipeline bind point set to VK_PIPELINE_BIND_POINT_GRAPHICS");
    subpass.colorAttachmentCount = 1;
    Logger::getInstance().log("Subpass color attachment count set to 1");
    subpass.pColorAttachments = &colorAttachmentRef;
    Logger::getInstance().log("Subpass color attachments set");

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    Logger::getInstance().log("Render pass create info sType set to VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO");
    renderPassInfo.attachmentCount = 1;
    Logger::getInstance().log("Render pass attachment count set to 1");
    renderPassInfo.pAttachments = &colorAttachment;
    Logger::getInstance().log("Render pass attachments set");
    renderPassInfo.subpassCount = 1;
    Logger::getInstance().log("Render pass subpass count set to 1");
    renderPassInfo.pSubpasses = &subpass;
    Logger::getInstance().log("Render pass subpasses set");

    VkDevice logicalDevice = device.getDevice();
    if (logicalDevice == VK_NULL_HANDLE) {
        Logger::getInstance().logError("Device handle is null before creating RenderPass. Aborting RenderPass creation.");
        throw std::runtime_error("Device handle is null, cannot create RenderPass.");
    } else {
        Logger::getInstance().log("Device handle value before creating RenderPass: " + std::to_string(reinterpret_cast<uint64_t>(logicalDevice)));
    }

    VkResult result = vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass);
    LogVulkanResult("RenderPass creation", result);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create RenderPass. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to create RenderPass.");
    }

    Logger::getInstance().log("RenderPass created successfully.");
}

void RenderPass::createFramebuffers() {
    Logger::getInstance().log("Creating framebuffers...");
    Logger::getInstance().log("Fetching swapchain image views...");
    const auto& imageViews = swapchain.getSwapchainImageViews();
    if (imageViews.empty()) {
        Logger::getInstance().logError("No swapchain image views available. Aborting framebuffer creation.");
        throw std::runtime_error("No swapchain image views available, cannot create framebuffers.");
    }
    framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        Logger::getInstance().log("Creating framebuffer for swapchain image view index: " + std::to_string(i));
        VkImageView attachments[] = {
            imageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchain.getSwapchainExtent().width;
        framebufferInfo.height = swapchain.getSwapchainExtent().height;
        framebufferInfo.layers = 1;

        VkDevice logicalDevice = device.getDevice();
        VkResult result = vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &framebuffers[i]);
        if (result != VK_SUCCESS) {
            Logger::getInstance().logError("Failed to create framebuffer at index " + std::to_string(i) + ". VkResult: " + std::to_string(result));
            throw std::runtime_error("Failed to create framebuffer!");
        }
        Logger::getInstance().log("Framebuffer created successfully at index: " + std::to_string(i));
    }
    Logger::getInstance().log("All framebuffers created successfully.");
}

void RenderPass::createCommandBuffers() {
    Logger::getInstance().log("Allocating command buffers...");
    commandBuffers.resize(framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = device.getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    VkDevice logicalDevice = device.getDevice();
    VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data());
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to allocate command buffers. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to allocate command buffers!");
    }
    Logger::getInstance().log("Command buffers allocated successfully.");
}

void RenderPass::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    Logger::getInstance().log("Recording command buffer for image index: " + std::to_string(imageIndex));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to begin recording command buffer. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapchain.getSwapchainExtent();

    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    Logger::getInstance().log("Beginning render pass for command buffer...");
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    Logger::getInstance().log("Render pass begun for command buffer.");

    // Record draw commands here
    Logger::getInstance().log("Recording draw commands... (currently none)");

    vkCmdEndRenderPass(commandBuffer);
    Logger::getInstance().log("Render pass ended for command buffer.");

    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to record command buffer. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to record command buffer!");
    }
    Logger::getInstance().log("Command buffer recorded successfully.");
}

void RenderPass::drawFrame() {
    Logger::getInstance().log("Drawing frame...");
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device.getDevice(), swapchain.getSwapchain(), UINT64_MAX, swapchain.getImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        Logger::getInstance().log("Swapchain is out of date, needs recreation.");
        // Handle swapchain recreation
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        Logger::getInstance().logError("Failed to acquire swap chain image. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    Logger::getInstance().log("Image acquired successfully. Recording command buffer...");
    recordCommandBuffer(commandBuffers[imageIndex], imageIndex);

    // Submit command buffer and present image (add detailed logging here)
    Logger::getInstance().log("Submitting command buffer and presenting image (functionality not fully implemented yet).");
}

void RenderPass::cleanup() {
    if (renderPass != VK_NULL_HANDLE) {
        Logger::getInstance().log("Destroying RenderPass...");
        vkDestroyRenderPass(device.getDevice(), renderPass, nullptr);
        Logger::getInstance().log("RenderPass destroyed successfully.");
    } else {
        Logger::getInstance().log("RenderPass destruction skipped (already null).");
    }
    for (auto framebuffer : framebuffers) {
        Logger::getInstance().log("Destroying framebuffer...");
        vkDestroyFramebuffer(device.getDevice(), framebuffer, nullptr);
    }
    Logger::getInstance().log("Framebuffers destroyed successfully.");
}