#include "RenderPass.h"
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

    // Log color attachment details
    Logger::getInstance().log("Color attachment description created:");
    Logger::getInstance().log("  Format: " + std::to_string(colorAttachment.format));
    Logger::getInstance().log("  Samples: " + std::to_string(colorAttachment.samples));
    Logger::getInstance().log("  Load Op: " + std::to_string(colorAttachment.loadOp));
    Logger::getInstance().log("  Store Op: " + std::to_string(colorAttachment.storeOp));
    Logger::getInstance().log("  Initial Layout: " + std::to_string(colorAttachment.initialLayout));
    Logger::getInstance().log("  Final Layout: " + std::to_string(colorAttachment.finalLayout));

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Log color attachment reference details
    Logger::getInstance().log("Color attachment reference created:");
    Logger::getInstance().log("  Attachment Index: " + std::to_string(colorAttachmentRef.attachment));
    Logger::getInstance().log("  Layout: " + std::to_string(colorAttachmentRef.layout));

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Log subpass details
    Logger::getInstance().log("Subpass description created:");
    Logger::getInstance().log("  Pipeline Bind Point: " + std::to_string(subpass.pipelineBindPoint));
    Logger::getInstance().log("  Color Attachment Count: " + std::to_string(subpass.colorAttachmentCount));

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    // Log render pass creation details
    Logger::getInstance().log("Creating render pass with the following settings:");
    Logger::getInstance().log("  Attachment Count: " + std::to_string(renderPassInfo.attachmentCount));
    Logger::getInstance().log("  Subpass Count: " + std::to_string(renderPassInfo.subpassCount));

    // Attempt to create the render pass
    Logger::getInstance().log("Calling vkCreateRenderPass...");
    VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create RenderPass: " + std::to_string(result));
        throw std::runtime_error("Failed to create RenderPass.");
    }

    Logger::getInstance().log("RenderPass created successfully.");
}
