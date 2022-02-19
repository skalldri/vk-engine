#pragma once

#include <engine/core/Device.hpp>
#include <fmtlog/Log.hpp>

template <class InputType>
class Buffer {
 public:
  Buffer(const Buffer& other) = delete;
  Buffer() = delete;

  Buffer(const LogicalDevice& device, const std::vector<InputType>& bufferContents)
      : device_(device),
        numElements_(bufferContents.size()) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(InputType) * bufferContents.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer_) != VK_SUCCESS) {
      LOG_F("failed to create VK buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory_) != VK_SUCCESS) {
      LOG_F("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device_, buffer_, bufferMemory_, 0 /*offset*/);

    // Copy the buffer contents into GPU space
    void* data;
    vkMapMemory(device_, bufferMemory_, 0 /*offset*/, bufferInfo.size, 0 /*flags*/, &data);
    memcpy(data, bufferContents.data(), (size_t)bufferInfo.size);
    vkUnmapMemory(device_, bufferMemory_);
  }

  ~Buffer() {
    vkDestroyBuffer(device_, buffer_, nullptr);
    vkFreeMemory(device_, bufferMemory_, nullptr);
  }

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device_.getPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }

    LOG_F("failed to find suitable memory type!");
  }

  operator VkBuffer() const { return buffer_; }

  size_t getNumElements() { return numElements_; }

 private:
  const LogicalDevice& device_;
  VkBuffer buffer_;
  VkDeviceMemory bufferMemory_;
  size_t numElements_;
};