#pragma once
#include <vulkan/vulkan.h>

VkPipelineColorBlendAttachmentState CreateDefaultColorBlendAttachment();
VkPipelineViewportStateCreateInfo CreateViewportStateInfo(VkViewport& viewport, VkRect2D& scissor);
