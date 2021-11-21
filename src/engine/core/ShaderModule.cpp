#include <fmt/core.h>
#include <vulkan/vulkan.h>

#include <ShaderModule.hpp>
#include <fstream>

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
    fmt::print("Error: Shader File '{}' does not exist\n",
               absoluteShaderFile.generic_string());
    return ShaderBinary();
  }

  std::ifstream file(absoluteShaderFile, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error(fmt::format("failed to open file {}",
                                         absoluteShaderFile.generic_string()));
  }

  size_t fileSize = file.tellg();
  ShaderBinary contents(fileSize);

  file.seekg(0);
  file.read(contents.data(), fileSize);
  file.close();

  return contents;
}

VkShaderModule ShaderModule::createVkShaderModule(
    const VkDevice& device,
    const ShaderBinary& shaderBinary) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = shaderBinary.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBinary.data());
  
  // TODO: return NULL on error instead of throwing an exception?
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}