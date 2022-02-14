#include "Sync.hpp"

#include <fmtlog/Log.hpp>

Semaphore::Semaphore(const LogicalDevice& device) : device_(device) {
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &semaphore_) !=
      VK_SUCCESS) {
    LOG_F("Failed to create semaphore");
  }
}

Semaphore::Semaphore(Semaphore&& other) : device_(other.device_) {
  semaphore_ = other.semaphore_;
  other.semaphore_ = NULL;
}

Semaphore::~Semaphore() {
  if (semaphore_) {
    vkDestroySemaphore(device_, semaphore_, nullptr);
  }
}

Fence::Fence(const LogicalDevice& device, bool initiallySignalled) : device_(device) {
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = initiallySignalled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

  if (vkCreateFence(device_, &fenceInfo, nullptr, &fence_) !=
      VK_SUCCESS) {
    LOG_F("Failed to create fence");
  }
}

Fence::Fence(Fence&& other) : device_(other.device_) {
  fence_ = other.fence_;
  other.fence_ = NULL;
}

Fence::~Fence() {
  if (fence_) {
    vkDestroyFence(device_, fence_, nullptr);
  }
}

void Fence::reset() {
  vkResetFences(device_, 1, &fence_);
}

void Fence::wait(uint64_t timeout) {
  // Wait for this command buffer (and other things) to be free
  vkWaitForFences(device_,
                  1,
                  &fence_,
                  VK_TRUE, /* waitAll */
                  timeout);
}