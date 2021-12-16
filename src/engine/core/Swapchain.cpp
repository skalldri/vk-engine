#include "Swapchain.hpp"

#include <algorithm>
#include <engine/utils/to_string.hpp>
#include <fmtlog/Log.hpp>
#include <set>

VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  // Pick first format if our preferred SRGBA8888 format isn't available
  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  // Pick MAILBOX present mode (effectively tripple buffering) if available
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;  // This is guaranteed to be available by the
                                    // Vulkan spec
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  // If the Width / Height are UINT32_MAX, then our underlying Vulkan
  // implementation is going to pick the correct size extent to fill the window
  // automatically, and we can just return that value
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    LOG_F("Swapchain cannot determine correct extents");
    return capabilities
        .currentExtent;  // THIS IS WRONG, BUT ALLOWS COMPILATION FOR NOW
    /*
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {width, height};

    actualExtent.width = std::clamp(actualExtent.width,
                                    capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.width = std::clamp(actualExtent.height,
                                    capabilities.minImageExtent.height,
                                    capabilities.maxImageExtent.height);

    return actualExtent;
    */
  }
}

Swapchain::Swapchain(const LogicalDevice &device,
                     const VkSurfaceKHR &surface,
                     std::vector<QueueFamilyRequest> queues)
    : device_(device),
      surface_(surface) {
  SwapChainSupportDetails swapChainSupport =
      device_.getPhysicalDevice().querySwapChainSupport(surface_);

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);

  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);

  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  // The minimum number of images required in the swap chain
  // We ask for one additional image to help prevent us from getting stuck
  // waiting for the driver to give us another image.
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

  // Make sure the driver actually supports this many images though...
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  LOG_I("Initializing SwapChain with these paramters:");
  LOG_I("\tFormat: {}, {}",
        vkFormatToString(surfaceFormat.format),
        vkColorspaceKHRToString(surfaceFormat.colorSpace));
  LOG_I("\tPresent Mode: {}", vkPresentModeKHRToString(presentMode));
  LOG_I("\tExtent: {}x{}", extent.width, extent.height);

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1; // Likely change this when doing VR
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  std::set<uint32_t> uniqueIndices;
  std::vector<uint32_t> queueFamilyIndices;
  for (const auto &queue : queues) {
    // If we haven't seen this queue family index before, add it to
    // the vector that tracks queue family indices
    if (uniqueIndices.count(queue.family.index) == 0) {
      queueFamilyIndices.push_back(queue.family.index);
    }

    // Track that we've seen this queue family index before
    uniqueIndices.insert(queue.family.index);
  }

  if (uniqueIndices.size() == 0) {
    LOG_F("Must have at least one queue for swapchain creation");
    return;
  }

  // If the application is going to operate on this swapchain from multiple
  // queue family indices, then we have to initialize it in a different mode
  if (uniqueIndices.size() > 1) {
    createInfo.imageSharingMode =
        VK_SHARING_MODE_CONCURRENT;  // Poor performance, easier to use
    createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
    createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    LOG_W(
        "Using concurrent queue family sharing for swapchain, expect "
        "reduced performance");
    LOG_W("The following queue families are using the swapchain:");
    for (const auto &index : queueFamilyIndices) {
      LOG_W("\tFamily {}", index);
    }
  } else {
    LOG_I("Using exclusive image sharing mode for swapchain: expect optimal performance");
    createInfo.imageSharingMode =
        VK_SHARING_MODE_EXCLUSIVE;             // Best performance
    createInfo.queueFamilyIndexCount = 0;      // Optional
    createInfo.pQueueFamilyIndices = nullptr;  // Optional
  }

  // Specify that we don't want any transform on the image by setting the
  // preTransform to the currentTransform
  // TODO: it's unlikely, but if currentTransform isn't the identity transform
  // then we should raise an error / try to see what's going on
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

  // Should this be blended with other windows in the windowing system?
  // Set to opaque for no
  // TODO: can this create transparent / blurred windows?
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  createInfo.presentMode = presentMode;

  // We don't care about the contents of pixels obscurred by other windows
  createInfo.clipped = VK_TRUE;

  // Used when we want to re-create the swapchain, for example if the window
  // gets resized. NULL for now.
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_) !=
      VK_SUCCESS) {
    LOG_F("failed to create swap chain!");
  }

  // Retrieve the images from the swapchain
  vkGetSwapchainImagesKHR(device_,
                          swapchain_,
                          &imageCount,
                          nullptr);
  images_.resize(imageCount);
  vkGetSwapchainImagesKHR(device_,
                          swapchain_,
                          &imageCount,
                          images_.data());

  // Track the configuration of the swapchain
  format_ = surfaceFormat.format;
  extent_ = extent;
}

Swapchain::~Swapchain() {
  vkDestroySwapchainKHR(device_, swapchain_, nullptr);
  images_.clear();
}