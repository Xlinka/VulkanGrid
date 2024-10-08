#include "LoggerUtils.h"
#include "Logger.h"

void LogVulkanResult(const std::string& action, VkResult result) {
    if (result == VK_SUCCESS) {
        Logger::getInstance().log(action + " succeeded.");
    } else {
        Logger::getInstance().logError(action + " failed with error code: " + std::to_string(result));
    }
}
