#include "RenderPass.h"
#include "../Utils/LoggerUtils.h"
#include <stdexcept>

RenderPass::RenderPass(VkDevice device, VkFormat swapchainImageFormat)
    : device(device), renderPass(VK_NULL_HANDLE) {
    Logger::getInstance().log("Creating RenderPass...");
    Logger::getInstance().log("Received swapchain image format: " + std::to_string(swapchainImageFormat));
    createRenderPass(swapchainImageFormat);
}

RenderPass::~RenderPass() {
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
        Logger::getInstance().log("RenderPass destroyed successfully.");
    } else {
        Logger::getInstance().log("RenderPass destruction skipped (already null).");
    }
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

    VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    LogVulkanResult("RenderPass creation", result);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create RenderPass.");
    }

    Logger::getInstance().log("RenderPass created successfully.");
}