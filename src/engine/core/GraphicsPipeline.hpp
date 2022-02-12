#pragma once

#include <engine/core/Device.hpp>
#include <engine/core/ShaderModule.hpp>
#include <engine/core/Swapchain.hpp>
#include <engine/core/RenderPass.hpp>
#include <optional>

#include <vulkan/vulkan.h>

class GraphicsPipeline {
 public:

  GraphicsPipeline() = delete;
  GraphicsPipeline(GraphicsPipeline& other) = delete;

  GraphicsPipeline(const LogicalDevice& device,
                   const Swapchain& swapchain,
                   const Subpass& subpass,
                   VertexShaderModule& vertexShader,
                   ShaderModule& fragmentShader);

  ~GraphicsPipeline();

  operator VkPipeline() const { return graphicsPipeline_; }

 private:
  const LogicalDevice& device_;
  const Swapchain& swapchain_;
  const Subpass& subpass_;

  VkPipelineLayout pipelineLayout_;
  VkPipeline graphicsPipeline_;
};