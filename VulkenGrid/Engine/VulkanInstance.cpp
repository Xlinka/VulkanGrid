#include "VulkanInstance.h"

#include <cstring>
#include <stdexcept>
#include <iostream>

// Define enableValidationLayers based on build configuration
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// Define validation layers to enable
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// VulkanInstance class implementation
void VulkanInstance::init() {
    Logger::getInstance().log("Initializing Vulkan instance...");

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        Logger::getInstance().logError("Validation layers requested, but not available.");
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GridSpace";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create Vulkan instance. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to create Vulkan instance!");
    }

    Logger::getInstance().log("Vulkan Instance created successfully.");

    // Log all loaded extensions
    Logger::getInstance().log("Loaded Extensions:");
    for (const auto& extension : extensions) {
        Logger::getInstance().log(extension);
    }
}

void VulkanInstance::cleanup() {
    Logger::getInstance().log("Destroying Vulkan instance...");
    vkDestroyInstance(instance, nullptr);
    Logger::getInstance().log("Vulkan instance destroyed.");
}

bool VulkanInstance::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            Logger::getInstance().logError(std::string("Validation layer not found: ") + layerName);
            return false;
        }
    }

    return true;
}

std::vector<const char*> VulkanInstance::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Define enableDebugUtils based on build configuration
    #ifdef NDEBUG
        const bool enableDebugUtils = false;
    #else
        const bool enableDebugUtils = true;
    #endif

    if (enableValidationLayers && enableDebugUtils) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    Logger::getInstance().log("Required extensions gathered.");
    return extensions;
}
