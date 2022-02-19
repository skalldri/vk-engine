#pragma once

#include <engine/core/Device.hpp>
#include <engine/core/Vertex.hpp>
#include <filesystem>
#include <vector>

// Declare that a Shader will be a vector of chars
using ShaderBinary = std::vector<char>;

class ShaderModule {
 public:
  ShaderModule() = delete;                     // No default constructor
  ShaderModule(const ShaderModule&) = delete;  // Not copyable

  ShaderModule(const LogicalDevice& device, const std::filesystem::path& shaderFile);
  ShaderModule(const LogicalDevice& device, const ShaderBinary& shaderContents);

  ~ShaderModule();

  operator VkShaderModule() { return shaderModule_; }

  static VkShaderModule ShaderModule::createVkShaderModule(
      const VkDevice& device,
      const ShaderBinary& shaderBinary) noexcept;

  // TODO: make this dynamic, or introspect the loaded binary to make sure this
  // is the correct symbol name
  const char* entryPointName() { return "main"; }

 private:
  ShaderBinary readBinaryFile(const std::filesystem::path& shaderFile);

 private:
  const LogicalDevice& device_;
  ShaderBinary shaderBinary_;
  VkShaderModule shaderModule_;
};

template <class InputType>
class VertexShaderInput {
 public:
  static VkVertexInputBindingDescription getBindingDescription() {
    return InputType::getBindingDescription();
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
    return InputType::getAttributeDescriptions();
  }
};

template <class InputType>
class VertexShaderModule : public ShaderModule {
 public:
  VertexShaderModule(const LogicalDevice& device, const std::filesystem::path& shaderFile)
      : ShaderModule(device, shaderFile){};
  VertexShaderModule(const LogicalDevice& device, const ShaderBinary& shaderContents)
      : ShaderModule(device, shaderContents){};

  const VkPipelineVertexInputStateCreateInfo&
  VertexShaderModule::getVertexShaderBindingDescription() {
    vertexInputBindingDescription_ = VertexShaderInput<InputType>::getBindingDescription();
    vertexInputAttributeDescription_ = VertexShaderInput<InputType>::getAttributeDescriptions();

    // Dummy vertex shader input descriptor that indicates we aren't going to
    // upload anything (since the vertices are compiled in already)
    vertexShaderInputDescription_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexShaderInputDescription_.vertexBindingDescriptionCount = 1;
    vertexShaderInputDescription_.pVertexBindingDescriptions = &vertexInputBindingDescription_;
    vertexShaderInputDescription_.vertexAttributeDescriptionCount =
        vertexInputAttributeDescription_.size();
    vertexShaderInputDescription_.pVertexAttributeDescriptions =
        vertexInputAttributeDescription_.data();
    vertexShaderInputDescription_.flags = 0;
    vertexShaderInputDescription_.pNext = NULL;

    return vertexShaderInputDescription_;
  }

  const VkPipelineInputAssemblyStateCreateInfo& VertexShaderModule::getVertexShaderInputAssembly() {
    // Describe the kinds of input we're going to load
    vertexShaderInputAssembly_.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vertexShaderInputAssembly_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    vertexShaderInputAssembly_.primitiveRestartEnable = VK_FALSE;
    vertexShaderInputAssembly_.pNext = NULL;
    vertexShaderInputAssembly_.flags = 0;

    return vertexShaderInputAssembly_;
  }

 private:
  VkPipelineVertexInputStateCreateInfo vertexShaderInputDescription_;
  VkPipelineInputAssemblyStateCreateInfo vertexShaderInputAssembly_;
  VkVertexInputBindingDescription vertexInputBindingDescription_;
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescription_;
};
