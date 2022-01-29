#pragma once

#include <engine/core/Device.hpp>

#include <vulkan/vulkan.h>

class CommandPool {
 public:
  CommandPool(const LogicalDevice& device, const QueueFamilyRequest& queue);

  ~CommandPool();

  operator VkCommandPool() { return commandPool_; }

 private:
  const LogicalDevice& device_;
  const QueueFamilyRequest& queue_;
  VkCommandPool commandPool_;
};