#include "Engine/VulkanInstance.h"
#include "Engine/VulkanDevice.h"
#include "Engine/VulkanSwapChain.h"
#include "Render/Pipeline.h"
#include "Logger/Logger.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>

int main() {
    try {
        Logger::getInstance().log("Application started.");

        // Initialize GLFW
        if (!glfwInit()) {
            Logger::getInstance().logError("Failed to initialize GLFW.");
            throw std::runtime_error("Failed to initialize GLFW");
        }
        Logger::getInstance().log("GLFW Initialized.");

        // Specify that we do not want an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Create a GLFW window
        GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            Logger::getInstance().logError("Failed to create GLFW window.");
            throw std::runtime_error("Failed to create GLFW window");
        }
        Logger::getInstance().log("Window Created.");

        // Initialize Vulkan instance
        VulkanInstance vulkanInstance;
        vulkanInstance.init();
        Logger::getInstance().log("Vulkan Instance Initialized.");

        // Create the Vulkan surface
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(vulkanInstance.getInstance(), window, nullptr, &surface) != VK_SUCCESS) {
            Logger::getInstance().logError("Failed to create window surface.");
            throw std::runtime_error("Failed to create window surface!");
        }
        Logger::getInstance().log("Vulkan Surface Created.");

        // Initialize Vulkan device
        VulkanDevice vulkanDevice(vulkanInstance);
        vulkanDevice.init(surface);
        Logger::getInstance().log("Vulkan Device Initialized.");

        // Initialize Vulkan swapchain
        VulkanSwapchain vulkanSwapchain(vulkanInstance, vulkanDevice, surface);
        vulkanSwapchain.init();
        Logger::getInstance().log("Vulkan Swapchain Initialized.");

       std::vector<ShaderModule> shaderModules = {
        ShaderModule(vulkanDevice.getDevice(), "Render/Shaders/triangle.vert", VK_SHADER_STAGE_VERTEX_BIT),
        ShaderModule(vulkanDevice.getDevice(), "Render/Shaders/triangle.frag", VK_SHADER_STAGE_FRAGMENT_BIT)
        };

        Pipeline pipeline(vulkanDevice, vulkanSwapchain, vulkanSwapchain.getRenderPass());  // Pass render pass here
        pipeline.createGraphicsPipeline(vulkanSwapchain.getSwapchainExtent(), shaderModules);
        Logger::getInstance().log("Pipeline Initialized.");
        ;

        // Main render loop
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // Acquire an image from the swapchain
            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(vulkanDevice.getDevice(), vulkanSwapchain.getSwapchain(), UINT64_MAX, vulkanSwapchain.getImageAvailableSemaphore(), VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                // Handle window resize/recreation of the swapchain
                // For now, we can skip this as we haven't handled resizing yet
                continue;
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                Logger::getInstance().logError("Failed to acquire swapchain image.");
                throw std::runtime_error("Failed to acquire swapchain image.");
            }

            // Submit the command buffer for rendering (this would include actual rendering commands)
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = { vulkanSwapchain.getImageAvailableSemaphore() };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            // Assume you have a command buffer recorded with rendering commands
            VkCommandBuffer commandBuffers[] = { vulkanSwapchain.getCommandBuffer(imageIndex) };
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = commandBuffers;

            VkSemaphore signalSemaphores[] = { vulkanSwapchain.getRenderFinishedSemaphore() };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(vulkanDevice.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
                Logger::getInstance().logError("Failed to submit draw command buffer.");
                throw std::runtime_error("Failed to submit draw command buffer.");
            }

            // Present the rendered image to the screen
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapchains[] = { vulkanSwapchain.getSwapchain() };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapchains;
            presentInfo.pImageIndices = &imageIndex;

            result = vkQueuePresentKHR(vulkanDevice.getPresentQueue(), &presentInfo);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                // Handle window resize/recreation of the swapchain
                // For now, we can skip this as we haven't handled resizing yet
            } else if (result != VK_SUCCESS) {
                Logger::getInstance().logError("Failed to present swapchain image.");
                throw std::runtime_error("Failed to present swapchain image.");
            }

            vkQueueWaitIdle(vulkanDevice.getPresentQueue()); // Ensure the presentation is done before moving to the next frame
        }

        // Clean up
        pipeline.cleanup();
        vulkanSwapchain.cleanup();
        vulkanDevice.cleanup();
        vulkanInstance.cleanup();
        Logger::getInstance().log("Application cleaned up.");

        glfwDestroyWindow(window);
        glfwTerminate();
        Logger::getInstance().log("GLFW Terminated.");

    } catch (const std::exception& e) {
        Logger::getInstance().logError(std::string("Exception: ") + e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
