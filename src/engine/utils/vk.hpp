#pragma once

#include <vulkan/vulkan.h>

bool operator ==(const VkExtent2D& a, const VkExtent2D& b) {
  return (a.width == b.width) && (a.height == b.height);
}

bool operator !=(const VkExtent2D& a, const VkExtent2D& b) {
  return (a.width != b.width) || (a.height != b.height);
}