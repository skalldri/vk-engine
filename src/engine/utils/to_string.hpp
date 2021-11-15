
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

std::string vkFormatToString(VkFormat format);

std::string vkColorspaceKHRToString(VkColorSpaceKHR colorspace);

std::string vkPresentModeKHRToString(VkPresentModeKHR presentMode);

std::vector<std::string> vkSurfaceTransformFlagsToString(
    VkSurfaceTransformFlagsKHR flags);