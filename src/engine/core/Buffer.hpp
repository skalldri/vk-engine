#pragma once

#include <engine/core/Device.hpp>
#include <fmtlog/Log.hpp>

template <class InputType>
class Buffer {
 public:
  Buffer(const Buffer& other) = delete;
  Buffer() = delete;

  Buffer(const LogicalDevice& device,
         const std::vector<InputType>& bufferContents,
         VkBufferUsageFlags usageFlags,
         VkMemoryPropertyFlags propertyFlags,
         const QueueFamilyRequests sharingQueues = {})
      : device_(device),
        numElements_(bufferContents.size()),
        bufferSize_(sizeof(InputType) * bufferContents.size()) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize_;
    bufferInfo.usage = usageFlags;  // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Handle potential sharing cases
    std::vector<uint32_t> sharedFamilyIndices;
    if (sharingQueues.size() != 0) {
      sharedFamilyIndices = getUniqueQueueFamilyIndices(sharingQueues);

      if (sharedFamilyIndices.size() > 1) {
        bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        bufferInfo.pQueueFamilyIndices = sharedFamilyIndices.data();
      }
    }

    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer_) != VK_SUCCESS) {
      LOG_F("failed to create VK buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, propertyFlags);
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory_) != VK_SUCCESS) {
      LOG_F("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device_, buffer_, bufferMemory_, 0 /*offset*/);
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

  size_t getNumElements() const { return numElements_; }

  size_t getBufferSize() const { return bufferSize_; }

 protected:
  const LogicalDevice& device_;
  VkBuffer buffer_;
  VkDeviceMemory bufferMemory_;
  size_t numElements_;
  size_t bufferSize_;
};

// Buffer used to store data in device memory, optimal format
template <class InputType>
class OnDeviceBuffer : public Buffer<InputType> {
 public:
  OnDeviceBuffer(const LogicalDevice& device,
                 const std::vector<InputType>& bufferContents,
                 const QueueFamilyRequests sharingQueues = {})
      : Buffer<InputType>(
            device,
            bufferContents,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            sharingQueues) {}
};

// Buffer used to transfer data from host to device memory
template <class InputType>
class TransferBuffer : public Buffer<InputType> {
 public:
  TransferBuffer(const LogicalDevice& device,
                 const std::vector<InputType>& bufferContents,
                 const QueueFamilyRequests sharingQueues = {})
      : Buffer<InputType>(
            device,
            bufferContents,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sharingQueues) {
    // Copy the buffer contents into the transfer buffer
    void* data;
    vkMapMemory(device_, bufferMemory_, 0 /*offset*/, bufferSize_, 0 /*flags*/, &data);
    memcpy(data, bufferContents.data(), bufferSize_);
    vkUnmapMemory(device_, bufferMemory_);
  }
};