#pragma once
#include <string>
#include <vulkan/vulkan.h>

void LogVulkanResult(const std::string& action, VkResult result);
