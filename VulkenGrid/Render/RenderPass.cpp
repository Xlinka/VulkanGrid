#include "RenderPass.h"
#include "../Utils/LoggerUtils.h"
#include <stdexcept>

RenderPass::RenderPass(VulkanDevice& device, VulkanSwapchain& swapchain, VkFormat swapchainImageFormat)
    : device(device), swapchain(swapchain), renderPass(VK_NULL_HANDLE) {
    Logger::getInstance().log("Creating RenderPass...");
    Logger::getInstance().log("Received swapchain image format: " + std::to_string(swapchainImageFormat));

    createRenderPass(swapchainImageFormat);
    createFramebuffers();
    createCommandBuffers();
}

RenderPass::~RenderPass() {
    if (renderPass != VK_NULL_HANDLE) {
        Logger::getInstance().log("Destroying RenderPass...");
        vkDestroyRenderPass(device.getDevice(), renderPass, nullptr);
        Logger::getInstance().log("RenderPass destroyed successfully.");
    }
    else {
        Logger::getInstance().log("RenderPass destruction skipped (already null).\n");
    }
    for (auto framebuffer : framebuffers) {
        Logger::getInstance().log("Destroying framebuffer...");
        vkDestroyFramebuffer(device.getDevice(), framebuffer, nullptr);
    }
    Logger::getInstance().log("Framebuffers destroyed successfully.");
}

VkRenderPass RenderPass::getRenderPass() const {
    return renderPass;
}

void RenderPass::createRenderPass(VkFormat swapchainImageFormat) {
    Logger::getInstance().log("Creating color attachment description...");

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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

    VkResult result = vkCreateRenderPass(device.getDevice(), &renderPassInfo, nullptr, &renderPass);
    LogVulkanResult("RenderPass creation", result);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create RenderPass. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to create RenderPass.");
    }

    Logger::getInstance().log("RenderPass created successfully.");
}

void RenderPass::createFramebuffers() {
    Logger::getInstance().log("Creating framebuffers...");
    framebuffers.resize(swapchain.getSwapchainImageViews().size());

    for (size_t i = 0; i < swapchain.getSwapchainImageViews().size(); i++) {
        VkImageView attachments[] = {
            swapchain.getSwapchainImageViews()[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchain.getSwapchainExtent().width;
        framebufferInfo.height = swapchain.getSwapchainExtent().height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(device.getDevice(), &framebufferInfo, nullptr, &framebuffers[i]);
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

    VkResult result = vkAllocateCommandBuffers(device.getDevice(), &allocInfo, commandBuffers.data());
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
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        Logger::getInstance().logError("Failed to acquire swap chain image. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    Logger::getInstance().log("Image acquired successfully. Recording command buffer...");
    recordCommandBuffer(commandBuffers[imageIndex], imageIndex);

    // Submit command buffer and present image (add detailed logging here)
    Logger::getInstance().log("Submitting command buffer and presenting image (functionality not fully implemented yet).");
}
