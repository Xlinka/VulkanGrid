
#include "ShaderModule.h"
#include <fstream>
#include <stdexcept>
#include "../Utils/FileUtils.h"

ShaderModule::ShaderModule(VkDevice device, const std::string& filepath, VkShaderStageFlagBits stage)
    : device(device), shaderStage(stage) {
    // Use the readFile method from FileUtils
    auto code = FileUtils::readFile(filepath);
    createShaderModule(code);
}

ShaderModule::~ShaderModule() {
    vkDestroyShaderModule(device, shaderModule, nullptr);
}


void ShaderModule::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
}

VkPipelineShaderStageCreateInfo ShaderModule::getPipelineShaderStageCreateInfo() const {
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage  = shaderStage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName  = "main"; // Entry point function name
    return shaderStageInfo;
}
