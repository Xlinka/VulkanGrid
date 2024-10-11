#include "PipeLine.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "ShaderModule.h"
#include "../Utils/LoggerUtils.h"
#include <stdexcept>
#include <vector>

Pipeline::Pipeline(VulkanDevice& device, VulkanSwapchain& swapchain, VkRenderPass renderPass)
    : device(device), swapchain(swapchain), renderPass(renderPass), graphicsPipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE) {
    Logger::getInstance().log("Pipeline object created.");
}

Pipeline::~Pipeline() {
    cleanup();
}

void Pipeline::createGraphicsPipeline(VkExtent2D swapchainExtent) {
    Logger::getInstance().log("Creating Graphics Pipeline...");

    // Create shader modules
    ShaderModule vertShaderModule(device.getDevice(), "shaders/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    ShaderModule fragShaderModule(device.getDevice(), "shaders/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    // Shader stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
        vertShaderModule.getPipelineShaderStageCreateInfo(),
        fragShaderModule.getPipelineShaderStageCreateInfo()
    };

    // Vertex input state
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Fill polygons
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Cull back faces
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // Clockwise winding
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending state
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 0;
    pipelineLayoutInfo.pSetLayouts            = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;

    if (vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create pipeline layout.");
        throw std::runtime_error("Failed to create pipeline layout.");
    }

    // Graphics pipeline creation
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = nullptr;
    pipelineInfo.layout              = pipelineLayout;
    pipelineInfo.renderPass          = renderPass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex   = -1;

    VkResult result = vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
    if (result != VK_SUCCESS) {
        Logger::getInstance().logError("Failed to create Graphics Pipeline: " + std::to_string(result));
        throw std::runtime_error("Failed to create graphics pipeline.");
    }

    Logger::getInstance().log("Graphics Pipeline created successfully.");
}

void Pipeline::cleanup() {
    VkDevice logicalDevice = device.getDevice();

    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
        Logger::getInstance().log("Graphics Pipeline destroyed.");
    } else {
        Logger::getInstance().log("Graphics Pipeline destruction skipped (already null).");
    }

    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
        Logger::getInstance().log("Pipeline Layout destroyed.");
    } else {
        Logger::getInstance().log("Pipeline Layout destruction skipped (already null).");
    }
}
