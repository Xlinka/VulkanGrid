#include <iostream>
#include <stdexcept>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "Logger.h"

// Function declarations
void initVulkan(GLFWwindow* window, VulkanInstance& instance, VulkanDevice& device, VulkanSwapchain& swapchain, RenderPass& renderPass, Pipeline& pipeline);
void mainLoop(GLFWwindow* window, VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline& pipeline, RenderPass& renderPass);
void drawFrame(VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline& pipeline, RenderPass& renderPass);
void cleanup(GLFWwindow* window, VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline& pipeline, RenderPass& renderPass);

int main() {
    Logger::getInstance().log("Application started.");

    // Initialize GLFW
    if (!glfwInit()) {
        Logger::getInstance().logError("Failed to initialize GLFW.");
        return -1;
    }
    Logger::getInstance().log("GLFW Initialized.");

    // Create GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Window", nullptr, nullptr);
    if (!window) {
        Logger::getInstance().logError("Failed to create GLFW window.");
        glfwTerminate();
        return -1;
    }
    Logger::getInstance().log("Window Created.");

    VulkanInstance vulkanInstance;
    VulkanDevice device(vulkanInstance);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(vulkanInstance.getInstance(), window, nullptr, &surface) != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create Vulkan surface.");
        throw std::runtime_error("Failed to create Vulkan surface.");
    }
    Logger::getInstance().log("Vulkan Surface Created.");

    VulkanSwapchain swapchain(vulkanInstance, device, surface);
    RenderPass renderPass(device.getDevice(), swapchain.getSwapchainImageFormat());
    Pipeline pipeline(device, swapchain, renderPass.getRenderPass());

    try {
        initVulkan(window, vulkanInstance, device, swapchain, renderPass, pipeline);
        mainLoop(window, device, swapchain, pipeline, renderPass);
    }
    catch (const std::exception& e) {
        Logger::getInstance().logError(e.what());
        return -1;
    }

    cleanup(window, device, swapchain, pipeline, renderPass);
    return 0;
}

void initVulkan(GLFWwindow* window, VulkanInstance& instance, VulkanDevice& device, VulkanSwapchain& swapchain, RenderPass& renderPass, Pipeline& pipeline) {
    Logger::getInstance().log("Initializing Vulkan...");

    // Initialize Vulkan instance
    instance.init();
    Logger::getInstance().log("Vulkan Instance Initialized.");

    // Vulkan device and swapchain initialization
    device.init(swapchain.getSurface());
    Logger::getInstance().log("Vulkan Device Initialized.");

    swapchain.init();
    Logger::getInstance().log("Vulkan Swapchain Initialized.");

    // Create RenderPass
    renderPass = RenderPass(device.getDevice(), swapchain.getSwapchainImageFormat());
    Logger::getInstance().log("RenderPass Created.");

    // Create Graphics Pipeline without shaders (just to clear the screen)
    pipeline.createGraphicsPipeline(swapchain.getSwapchainExtent());
    Logger::getInstance().log("Graphics Pipeline Created (No Shaders, Clear Color Only).");
}

void drawFrame(VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline& pipeline, RenderPass& renderPass) {
    Logger::getInstance().log("Drawing Frame...");

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device.getDevice(), swapchain.getSwapchain(), UINT64_MAX, swapchain.getImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);

    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to acquire swapchain image.");
        throw std::runtime_error("Failed to acquire swapchain image.");
    }

    VkCommandBuffer commandBuffer = swapchain.getCommandBuffer();

    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass.getRenderPass();
    renderPassInfo.framebuffer = swapchain.getFramebuffers()[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapchain.getSwapchainExtent();

    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };  // Clear color to black
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // No drawing commands since we are only clearing the screen

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { swapchain.getImageAvailableSemaphore() };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = { swapchain.getRenderFinishedSemaphore() };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapchains[] = { swapchain.getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to present swapchain image.");
        throw std::runtime_error("Failed to present swapchain image.");
    }

    Logger::getInstance().log("Frame Drawn.");
}

void mainLoop(GLFWwindow* window, VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline& pipeline, RenderPass& renderPass) {
    Logger::getInstance().log("Entering main loop...");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame(device, swapchain, pipeline, renderPass);
    }

    vkDeviceWaitIdle(device.getDevice());
    Logger::getInstance().log("Exiting main loop.");
}

void cleanup(GLFWwindow* window, VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline& pipeline, RenderPass& renderPass) {
    pipeline.cleanup();
    renderPass.~RenderPass();
    swapchain.cleanup();
    device.cleanup();

    Logger::getInstance().log("Application cleaned up and closing.");

    glfwDestroyWindow(window);
    glfwTerminate();
}
