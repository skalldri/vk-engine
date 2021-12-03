#include <fmt/core.h>
#include <vulkan/vulkan.h>

#include <ShaderModule.hpp>
#include <fstream>

#include <fmtlog/Log.hpp>

using namespace std;
using namespace std::filesystem;

ShaderModule::ShaderModule(const path& shaderFile) {
  // TODO: async?
  shaderBinary_ = readBinaryFile(shaderFile);
}

ShaderModule::ShaderModule(const ShaderBinary& shaderContents) {
  shaderBinary_ = shaderContents;
}

ShaderBinary ShaderModule::readBinaryFile(const path& shaderFile) {
  path absoluteShaderFile = absolute(shaderFile);

  if (!exists(absoluteShaderFile) || !is_regular_file(absoluteShaderFile)) {
    LOG_E("Shader file '{}' does not exist",
               absoluteShaderFile.generic_string());
    return ShaderBinary();
  }

  std::ifstream file(absoluteShaderFile, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    LOG_E("Failed to open shader file '{}'", absoluteShaderFile.generic_string());
    return ShaderBinary();
  }

  size_t fileSize = file.tellg();
  ShaderBinary contents(fileSize);

  file.seekg(0);
  file.read(contents.data(), fileSize);
  file.close();

  LOG_D("Read contents of shader file '{}', size = {}", absoluteShaderFile.generic_string(), fileSize);

  return contents;
}

VkShaderModule ShaderModule::createVkShaderModule(
    const VkDevice& device,
    const ShaderBinary& shaderBinary) noexcept {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = shaderBinary.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBinary.data());
  
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    LOG_E("failed to create shader module!");
    return VK_NULL_HANDLE;
  }

  return shaderModule;
}