#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <GLFW/glfw3.h>

#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include "Logger.h"

class VulkanInstance {
public:
    void init();
    void cleanup();
    VkInstance getInstance() const { return instance; }
    bool enableValidationLayers = true;
    const std::vector<const char*>& getValidationLayers() const { return validationLayers; }

private:
    VkInstance instance;
    std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData);
};

#endif // VULKAN_INSTANCE_H