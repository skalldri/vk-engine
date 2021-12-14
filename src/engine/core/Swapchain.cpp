#include "Swapchain.hpp"

#include <fmtlog/Log.hpp>

Swapchain::Swapchain(const LogicalDevice& device, const VkSurfaceKHR& surface)
    : device_(device),
      surface_(surface) {

          

}