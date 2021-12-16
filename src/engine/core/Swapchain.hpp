#pragma once

#include <vulkan/vulkan.h>

#include <engine/core/Device.hpp>

#include <vector>

/**
 * @brief A class representing a Swapchain.
 *
 * Swapchains are part of the Vulkan Window System Integration (WSI)
 * core extension. If a Swapchain is being used, it can be assumed that
 * VkSurfaceKHR is available.
 * 
 * The Swapchain is a specific set of VkImages
 * that is used as the final render targets that get
 * presented to the user. Since these VkImages
 * need to interface with the OS's underlying compositor framework,
 * we can't simply instantiate these VkImages like other kinds of render
 * targets. Instead, we need to interact with the OS via the Swapchain to
 * request access to special buffers that the OS will then present to the user
 * as part of it's compositor work.
 *
 * Because the Swapchain is tied to the OS compositor, it can become invalid
 * if something at the OS level changes. For example, if the user resizes the
 * Window, the swapchain can become invalid because the dimensions of the
 * underlying VkImages needs to change. The Swapchain class provides mechanisms to 
 * regenerate the Swapchain when this occurs, as well as mechanisms for detecting when
 * the Swapchain needs to be regenerated.
 * 
 * 
 */
class Swapchain {
 public:
  Swapchain() = delete;
  Swapchain(Swapchain& other) = delete;

  ~Swapchain();

  Swapchain(Swapchain&& other);

  Swapchain(const LogicalDevice& device, 
            const VkSurfaceKHR& surface,
            std::vector<QueueFamilyRequest> queues);

  VkExtent2D getExtent() { return extent_; }

  VkFormat getFormat() { return format_; }

  const std::vector<VkImage>& getImages() { return images_; }

  bool isOptimal();

  operator VkSwapchainKHR() { return swapchain_; }

 private:
  const LogicalDevice& device_;
  const VkSurfaceKHR& surface_;
  std::vector<VkImage> images_;
  VkFormat format_;
  VkExtent2D extent_;
  VkSwapchainKHR swapchain_;
};