#pragma once

#include <filesystem>
#include <vector>

// Declare that a Shader will be a vector of chars
using ShaderBinary = std::vector<char>;

class ShaderModule {
 public:
  ShaderModule() = delete;                     // No default constructor
  ShaderModule(const ShaderModule&) = delete;  // Not copyable

  ShaderModule(const std::filesystem::path& shaderFile);
  ShaderModule(const ShaderBinary& shaderContents);

  static VkShaderModule ShaderModule::createVkShaderModule(const VkDevice& device, const ShaderBinary& shaderBinary);

 private:
  ShaderBinary readBinaryFile(const std::filesystem::path& shaderFile);

  

 private:
  ShaderBinary shaderBinary_;
};