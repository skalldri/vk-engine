#pragma once

#include <engine/core/Device.hpp>

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

  static VkShaderModule ShaderModule::createVkShaderModule(const VkDevice& device, const ShaderBinary& shaderBinary) noexcept;  

  // TODO: make this dynamic, or introspect the loaded binary to make sure this is the correct symbol name
  const char* entryPointName() { return "main"; }

 private:
  ShaderBinary readBinaryFile(const std::filesystem::path& shaderFile);

 private:
  const LogicalDevice& device_;
  ShaderBinary shaderBinary_;
  VkShaderModule shaderModule_;
};

class VertexShaderModule : public ShaderModule {
 public:
  VertexShaderModule(const LogicalDevice& device, const std::filesystem::path& shaderFile) : ShaderModule(device, shaderFile) {};
  VertexShaderModule(const LogicalDevice& device, const ShaderBinary& shaderContents) : ShaderModule(device, shaderContents) {};

  const VkPipelineVertexInputStateCreateInfo& getVertexShaderBindingDescription();
  const VkPipelineInputAssemblyStateCreateInfo& getVertexShaderInputAssembly();

 private:
  VkPipelineVertexInputStateCreateInfo vertexShaderInputDescription_;
  VkPipelineInputAssemblyStateCreateInfo vertexShaderInputAssembly_;
};