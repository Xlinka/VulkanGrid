#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class ShaderModule {
public:
    ShaderModule(VkDevice device, const std::string& filepath, VkShaderStageFlagBits stage);
    ~ShaderModule();

    VkShaderModule getShaderModule() const { return shaderModule; }
    VkShaderStageFlagBits getShaderStage() const { return shaderStage; }

private:
    VkDevice device;
    VkShaderModule shaderModule;
    VkShaderStageFlagBits shaderStage;

    std::vector<char> readFile(const std::string& filepath);
    void createShaderModule(const std::vector<char>& code);
};
