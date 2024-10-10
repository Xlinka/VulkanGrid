#include "VulkanDevice.h"
#include <stdexcept>
#include <sstream>

VulkanDevice::VulkanDevice(VulkanInstance& instance) : instance(instance) {}

void VulkanDevice::init(VkSurfaceKHR surface) {
    Logger::getInstance().log("Initializing Vulkan Device...");
    pickPhysicalDevice(surface);
    createLogicalDevice(surface);
    createCommandPool();
    Logger::getInstance().log("Vulkan Device initialized successfully.");
}

void VulkanDevice::cleanup() {
    Logger::getInstance().log("Cleaning up Vulkan Device...");
    if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool, nullptr);
        Logger::getInstance().log("Command pool destroyed successfully.");
    }
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        Logger::getInstance().log("Logical device destroyed successfully.");
    }
}

void VulkanDevice::pickPhysicalDevice(VkSurfaceKHR surface) {
    Logger::getInstance().log("Picking physical device...");
    uint32_t deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance.getInstance(), &deviceCount, nullptr);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to enumerate physical devices. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to enumerate physical devices!");
    }

    if (deviceCount == 0) {
        Logger::getInstance().logError("Failed to find GPUs with Vulkan support!");
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    Logger::getInstance().log("Number of physical devices found: " + std::to_string(deviceCount));

    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(instance.getInstance(), &deviceCount, devices.data());
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to enumerate physical devices. VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to enumerate physical devices!");
    }

    for (const auto& device : devices) {
        Logger::getInstance().log("Evaluating physical device...");
        if (isDeviceSuitable(device, surface)) {
            physicalDevice = device;
            Logger::getInstance().log("Physical device selected.");
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        Logger::getInstance().logError("Failed to find a suitable GPU!");
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void VulkanDevice::createLogicalDevice(VkSurfaceKHR surface) {
    Logger::getInstance().log("Creating logical device...");
    queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value()
    };

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        Logger::getInstance().log("Setting up queue for queue family index: " + std::to_string(queueFamily));
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;

    auto extensions = getDeviceExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    Logger::getInstance().log("Enabled Device Extensions:");
    for (const auto& ext : extensions) {
        Logger::getInstance().log(std::string(" - ") + ext);
    }

    if (instance.enableValidationLayers) {
        const auto& validationLayers = instance.getValidationLayers();
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        Logger::getInstance().log("Enabled Validation Layers:");
        for (const auto& layer : validationLayers) {
            Logger::getInstance().log(std::string(" - ") + layer);
        }
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create logical device! VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
    Logger::getInstance().log("Logical device created successfully.");
}

void VulkanDevice::createCommandPool() {
    Logger::getInstance().log("Creating command pool...");
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create command pool! VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to create command pool!");
    }
    Logger::getInstance().log("Command pool created successfully.");
}

bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    Logger::getInstance().log("Checking if device is suitable...");
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;

    Logger::getInstance().log("Queue Family Indices completeness: " + std::string(indices.isComplete() ? "Complete" : "Incomplete"));
    Logger::getInstance().log("Extensions supported: " + std::string(extensionsSupported ? "Yes" : "No"));

    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        Logger::getInstance().log("Swap chain formats count: " + std::to_string(swapChainSupport.formats.size()));
        Logger::getInstance().log("Swap chain present modes count: " + std::to_string(swapChainSupport.presentModes.size()));
    }

    bool isSuitable = indices.isComplete() && extensionsSupported && swapChainAdequate;
    Logger::getInstance().log("Device suitability: " + std::string(isSuitable ? "Suitable" : "Not Suitable"));
    return isSuitable;
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const {
    Logger::getInstance().log("Finding queue families...");
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    Logger::getInstance().log("Queue family count: " + std::to_string(queueFamilyCount));

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int index = 0;
    for (const auto& queueFamily : queueFamilies) {
        Logger::getInstance().log("Evaluating queue family index: " + std::to_string(index));
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = index;
            Logger::getInstance().log("Graphics queue family found at index: " + std::to_string(index));
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = index;
            Logger::getInstance().log("Present queue family found at index: " + std::to_string(index));
        }

        if (indices.isComplete()) {
            Logger::getInstance().log("Required queue families found.");
            break;
        }

        index++;
    }

    return indices;
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupport(VkSurfaceKHR surface) const {
    return querySwapChainSupport(physicalDevice, surface);
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const {
    Logger::getInstance().log("Querying swap chain support...");
    SwapChainSupportDetails details;

    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to get physical device surface capabilities! VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to get physical device surface capabilities!");
    }

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    Logger::getInstance().log("Surface format count: " + std::to_string(formatCount));

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        Logger::getInstance().log("Surface formats retrieved.");
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    Logger::getInstance().log("Present mode count: " + std::to_string(presentModeCount));

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        Logger::getInstance().log("Present modes retrieved.");
    }

    return details;
}

bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) const {
    Logger::getInstance().log("Checking device extension support...");
    uint32_t extensionCount = 0;
    VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to enumerate device extension properties! VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to enumerate device extension properties!");
    }

    Logger::getInstance().log("Available device extension count: " + std::to_string(extensionCount));

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to enumerate device extension properties! VkResult: " + std::to_string(result));
        throw std::runtime_error("Failed to enumerate device extension properties!");
    }

    // Store the device extensions in a local variable to keep iterators valid
    std::vector<const char*> deviceExtensions = getDeviceExtensions();
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    Logger::getInstance().log("Required Device Extensions:");
    for (const auto& ext : requiredExtensions) {
        Logger::getInstance().log(std::string(" - ") + ext);
    }

    Logger::getInstance().log("Available Device Extensions:");
    for (const auto& extension : availableExtensions) {
        Logger::getInstance().log(std::string(" - ") + extension.extensionName);
        requiredExtensions.erase(extension.extensionName);
    }

    if (!requiredExtensions.empty()) {
        Logger::getInstance().log("Missing required device extensions:");
        for (const auto& missingExt : requiredExtensions) {
            Logger::getInstance().logError(missingExt.c_str());
        }
    }

    Logger::getInstance().log("Device extension support " + std::string(requiredExtensions.empty() ? "available" : "not available"));
    return requiredExtensions.empty();
}

std::vector<const char*> VulkanDevice::getDeviceExtensions() const {
    return {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
        // Removed VK_KHR_SURFACE_EXTENSION_NAME as it is an instance extension
    };
}
