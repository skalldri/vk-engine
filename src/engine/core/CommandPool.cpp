#include "CommandPool.hpp"

#include <fmtlog/Log.hpp>

CommandPool::CommandPool(const LogicalDevice& device, const QueueFamilyRequest& queue) : device_(device), queue_(queue) {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queue_.family.index;
  poolInfo.flags = 0;  // Optional.
                       // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, Hint that
                       // command buffers are rerecorded with new commands very
                       // often (may change memory allocation behavior)
                       // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow
                       // command buffers to be rerecorded individually, without
                       // this flag they all have to be reset together

  if (vkCreateCommandPool(device.getVkDevice(),
                          &poolInfo,
                          nullptr,
                          &commandPool_) != VK_SUCCESS) {
    LOG_F("failed to create command pool!");
  }
}

CommandPool::~CommandPool() {
  vkDestroyCommandPool(device_.getVkDevice(), commandPool_, nullptr);
}