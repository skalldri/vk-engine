#pragma once

#include <vulkan/vulkan.h>

#include <engine/core/Device.hpp>
#include <engine/core/RenderPass.hpp>
#include <engine/core/ShaderModule.hpp>
#include <engine/core/Swapchain.hpp>
#include <fmtlog/Log.hpp>
#include <optional>

template <class InputType>
class GraphicsPipeline {
 public:
  GraphicsPipeline() = delete;
  GraphicsPipeline(GraphicsPipeline& other) = delete;

  GraphicsPipeline(const LogicalDevice& device,
                   const Swapchain& swapchain,
                   const Subpass& subpass,
                   VertexShaderModule<InputType>& vertexShader,
                   ShaderModule& fragmentShader)
      : device_(device),
        swapchain_(swapchain),
        subpass_(subpass) {
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexShader;
    vertShaderStageInfo.pName = vertexShader.entryPointName();

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentShader;
    fragShaderStageInfo.pName = fragmentShader.entryPointName();

    // TODO: allow more pipeline stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.push_back(vertShaderStageInfo);
    shaderStages.push_back(fragShaderStageInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain_.getExtent().width;
    viewport.height = (float)swapchain_.getExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_.getExtent();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;  // If TRUE, clamp depth values to the near and far
                                             // planes instead of discarding them
    rasterizer.rasterizerDiscardEnable =
        VK_FALSE;  // Set to TRUE to discard all output from the rasterizer. No
                   // idea why you'd want to do that...
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // Fill the outputs from the vertex shader
                                                    // completely. Other options include line and
                                                    // point. Other modes require a GPU feature to
                                                    // be enabled

    rasterizer.lineWidth = 1.0f;  // Thickness of lines. Anything greater than 1.0
                                  // requires a widelines GPU feature

    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;  // Cull backfacing
    rasterizer.frontFace =
        VK_FRONT_FACE_CLOCKWISE;  // Front facing vertices have clockwise ordering

    // Rasterizer can be configured to bias or offset or clamp depth values
    // We don't care about this for now
    // Can be used in shadow mapping
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
    rasterizer.depthBiasClamp = 0.0f;           // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

    // Disable MSAA for now
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    // Disable color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

    // Setup global color blending parameters
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;             // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr;          // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;     // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;  // Optional

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) !=
        VK_SUCCESS) {
      LOG_F("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexShader.getVertexShaderBindingDescription();
    pipelineInfo.pInputAssemblyState = &vertexShader.getVertexShaderInputAssembly();
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;  // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;  // Optional

    pipelineInfo.layout = pipelineLayout_;

    pipelineInfo.renderPass = subpass_.parent();
    pipelineInfo.subpass = subpass_.getIndex();

    // Allows for deriving a new pipeline from an old pipeline
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex = -1;               // Optional

    if (vkCreateGraphicsPipelines(device_,
                                  VK_NULL_HANDLE,
                                  1,
                                  &pipelineInfo,
                                  nullptr,
                                  &graphicsPipeline_) != VK_SUCCESS) {
      LOG_F("failed to create graphics pipeline!");
    }
  }

  ~GraphicsPipeline() {
    vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
  }

  operator VkPipeline() const { return graphicsPipeline_; }

 private:
  const LogicalDevice& device_;
  const Swapchain& swapchain_;
  const Subpass& subpass_;

  VkPipelineLayout pipelineLayout_;
  VkPipeline graphicsPipeline_;
};