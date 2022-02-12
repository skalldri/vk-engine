#include <fmt/core.h>
#include <vulkan/vulkan.h>

#include <ShaderModule.hpp>
#include <fstream>

#include <fmtlog/Log.hpp>

using namespace std;
using namespace std::filesystem;

ShaderModule::ShaderModule(const LogicalDevice& device, const path& shaderFile) : device_(device) {
  // TODO: async?
  shaderBinary_ = readBinaryFile(shaderFile);
  shaderModule_ = createVkShaderModule(device_, shaderBinary_);
}

ShaderModule::ShaderModule(const LogicalDevice& device, const ShaderBinary& shaderContents) : device_(device) {
  shaderBinary_ = shaderContents;
  shaderModule_ = createVkShaderModule(device_, shaderBinary_);
}

ShaderModule::~ShaderModule() {
  vkDestroyShaderModule(device_, shaderModule_, nullptr);
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

const VkPipelineVertexInputStateCreateInfo& VertexShaderModule::getVertexShaderBindingDescription() {
  // Dummy vertex shader input descriptor that indicates we aren't going to
  // upload anything (since the vertices are compiled in already)
  vertexShaderInputDescription_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexShaderInputDescription_.vertexBindingDescriptionCount = 0;
  vertexShaderInputDescription_.pVertexBindingDescriptions = nullptr;  // Optional
  vertexShaderInputDescription_.vertexAttributeDescriptionCount = 0;
  vertexShaderInputDescription_.pVertexAttributeDescriptions = nullptr;  // Optional
  vertexShaderInputDescription_.flags = 0;
  vertexShaderInputDescription_.pNext = NULL;

  return vertexShaderInputDescription_;
}

const VkPipelineInputAssemblyStateCreateInfo& VertexShaderModule::getVertexShaderInputAssembly() {
  // Describe the kinds of input we're going to load
  vertexShaderInputAssembly_.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  vertexShaderInputAssembly_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  vertexShaderInputAssembly_.primitiveRestartEnable = VK_FALSE;
  vertexShaderInputAssembly_.pNext = NULL;
  vertexShaderInputAssembly_.flags = 0;

  return vertexShaderInputAssembly_;
}