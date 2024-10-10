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
#include "SystemInfo.h"

#include "../Utils/LoggerUtils.h"

void mainLoop(GLFWwindow* window, VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline* pipeline, RenderPass* renderPass);
void cleanup(GLFWwindow* window, VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline* pipeline, RenderPass* renderPass);

int main() {
    Logger::getInstance().log("Application started.");

    // Log system info before any Vulkan setup
    try {
        Logger::getInstance().log("Collecting system information...");
        Logger::getInstance().log("Operating System: " + SystemInfo::getOSName());
        Logger::getInstance().log("CPU: " + SystemInfo::getCPUName());
        Logger::getInstance().log("RAM Available: " + std::to_string(SystemInfo::getAvailableRAM()) + " GB");
        Logger::getInstance().log("RAM Usable: " + std::to_string(SystemInfo::getUsableRAM()) + " GB");
        Logger::getInstance().log("GPU: " + SystemInfo::getGPUName());
        Logger::getInstance().log("VRAM: " + std::to_string(SystemInfo::getGPUVRAM()) + " GB");
        Logger::getInstance().log("System information collected.");
    }
    catch (const std::exception& e) {
        Logger::getInstance().logError("Failed to collect system information: " + std::string(e.what()));
        return -1;
    }

    // Initialize GLFW
    if (!glfwInit()) {
        Logger::getInstance().logError("Failed to initialize GLFW.");
        return -1;
    }
    Logger::getInstance().log("GLFW Initialized.");

    // Configure GLFW to not use OpenGL and set the window to be resizable
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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

    // Initialize Vulkan instance
    try {
        vulkanInstance.init();
    }
    catch (const std::runtime_error& e) {
        Logger::getInstance().logError(std::string("Failed to initialize Vulkan Instance: ") + e.what());
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Create Vulkan Surface
    if (glfwCreateWindowSurface(vulkanInstance.getInstance(), window, nullptr, &surface) != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create Vulkan surface.");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    Logger::getInstance().log("Vulkan Surface Created.");

    // Initialize Device
    try {
        device.init(surface);
    }
    catch (const std::runtime_error& e) {
        Logger::getInstance().logError(std::string("Failed to initialize Vulkan Device: ") + e.what());
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    Logger::getInstance().log("Vulkan Device Initialized.");

    VulkanSwapchain swapchain(vulkanInstance, device, surface);
    RenderPass* renderPass = nullptr;
    Pipeline* pipeline = nullptr;

    try {
        swapchain.init();
        Logger::getInstance().log("Vulkan Swapchain Initialized.");

        // Create RenderPass
        renderPass = new RenderPass(device, swapchain, swapchain.getSwapchainImageFormat());
        Logger::getInstance().log("RenderPass created.");

        // Create Pipeline
        pipeline = new Pipeline(device, swapchain, renderPass->getRenderPass());
        pipeline->createGraphicsPipeline(swapchain.getSwapchainExtent());
        Logger::getInstance().log("Graphics Pipeline Created.");

        // Enter the main application loop
        mainLoop(window, device, swapchain, pipeline, renderPass);
    }
    catch (const std::exception& e) {
        Logger::getInstance().logError(std::string("Error during Vulkan initialization or execution: ") + e.what());
        cleanup(window, device, swapchain, pipeline, renderPass);
        return -1;
    }

    // Cleanup resources
    cleanup(window, device, swapchain, pipeline, renderPass);
    Logger::getInstance().log("Application exited cleanly.");
    return 0;
}

void mainLoop(GLFWwindow* window, VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline* pipeline, RenderPass* renderPass) {
    Logger::getInstance().log("Entering main loop...");
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderPass->drawFrame(pipeline); // Use RenderPass's drawFrame method
    }
    vkDeviceWaitIdle(device.getDevice());
    Logger::getInstance().log("Exiting main loop.");
}

void cleanup(GLFWwindow* window, VulkanDevice& device, VulkanSwapchain& swapchain, Pipeline* pipeline, RenderPass* renderPass) {
    if (pipeline) {
        pipeline->cleanup();
        delete pipeline;
    }
    if (renderPass) {
        renderPass->cleanup();
        delete renderPass;
    }
    swapchain.cleanup();
    device.cleanup();

    Logger::getInstance().log("Application cleaned up and closing.");
    glfwDestroyWindow(window);
    glfwTerminate();
}
