#pragma once

#include <vulkan/vulkan.h>

#include <engine/core/Device.hpp>

class Semaphore {
 public:
  Semaphore() = delete;
  Semaphore(Semaphore& other) = delete;

  Semaphore(const LogicalDevice& device);
  Semaphore(Semaphore&& other);
  ~Semaphore();

  operator const VkSemaphore&() { return semaphore_; }

 private:
  const LogicalDevice& device_;
  VkSemaphore semaphore_;
};

class Fence {
 public:
  Fence() = delete;
  Fence(Fence& other) = delete;

  Fence(const LogicalDevice& device, bool initiallySignalled);
  Fence(Fence&& other);
  ~Fence();

  void reset();
  
  void wait(uint64_t timeout = UINT64_MAX);

  operator const VkFence&() { return fence_; }

 private:
  const LogicalDevice& device_;
  VkFence fence_;
};
