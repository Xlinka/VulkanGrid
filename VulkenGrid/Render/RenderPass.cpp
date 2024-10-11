#include "RenderPass.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "PipeLine.h"
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

    // Subpass dependencies (optional but recommended)
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkDevice logicalDevice = device.getDevice();
    VkResult result = vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass);
    LogVulkanResult("RenderPass creation", result);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create RenderPass. VkResult: " + std::to_string(result));
        renderPass = VK_NULL_HANDLE; // Ensure handle is null
        throw std::runtime_error("Failed to create RenderPass.");
    }

    Logger::getInstance().log("RenderPass created successfully.");
}

void RenderPass::createFramebuffers() {
    Logger::getInstance().log("Creating framebuffers...");
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
            framebuffers[i] = VK_NULL_HANDLE; // Ensure handle is null
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

void RenderPass::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, Pipeline* pipeline) {
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

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    Logger::getInstance().log("Beginning render pass for command buffer...");
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    Logger::getInstance().log("Render pass begun for command buffer.");

    // Bind the graphics pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getGraphicsPipeline());

    // Record draw commands
    Logger::getInstance().log("Recording draw commands...");
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    Logger::getInstance().log("Draw command recorded.");

    vkCmdEndRenderPass(commandBuffer);
    Logger::getInstance().log("Render pass ended for command buffer.");

    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to record command buffer. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to record command buffer!");
    }
    Logger::getInstance().log("Command buffer recorded successfully.");
}

void RenderPass::drawFrame(Pipeline* pipeline) {
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
    recordCommandBuffer(commandBuffers[imageIndex], imageIndex, pipeline);

    // Submit the command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { swapchain.getImageAvailableSemaphore() };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { swapchain.getRenderFinishedSemaphore() };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    result = vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to submit draw command buffer. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    // Present the image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { swapchain.getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        Logger::getInstance().log("Swapchain is out of date or suboptimal, needs recreation.");
        // Handle swapchain recreation
        return;
    } else if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to present swap chain image. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to present swap chain image!");
    }

    Logger::getInstance().log("Frame drawn successfully.");
}

void RenderPass::cleanup() {
    VkDevice logicalDevice = device.getDevice();

    if (renderPass != VK_NULL_HANDLE) {
        Logger::getInstance().log("Destroying RenderPass...");
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
        Logger::getInstance().log("RenderPass destroyed successfully.");
    } else {
        Logger::getInstance().log("RenderPass destruction skipped (already null).");
    }

    for (auto& framebuffer : framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            Logger::getInstance().log("Destroying framebuffer...");
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        } else {
            Logger::getInstance().log("Framebuffer destruction skipped (already null).");
        }
    }
    framebuffers.clear();
    Logger::getInstance().log("Framebuffers destroyed successfully.");
}
