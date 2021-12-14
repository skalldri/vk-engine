#include "Device.hpp"

#include <engine/utils/to_string.hpp>
#include <fmtlog/Log.hpp>
#include <map>
#include <set>

std::string getDeviceTypeString(VkPhysicalDeviceType type) {
  switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
      return "OTHER";

    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      return "INTEGRATED";

    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      return "DISCRETE";

    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      return "VIRTUAL";

    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      return "CPU";
  }

  return "UNKNOWN";
}

std::vector<PhysicalDevice> PhysicalDevice::getPhysicalDevices(
    Instance& instance, std::optional<VkSurfaceKHR> surface) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance.getInstance(), &deviceCount, nullptr);

  if (deviceCount == 0) {
    LOG_F("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> vkPhysicalDevices(deviceCount);
  vkEnumeratePhysicalDevices(instance.getInstance(),
                             &deviceCount,
                             vkPhysicalDevices.data());

  std::vector<PhysicalDevice> physicalDevices;
  physicalDevices.reserve(deviceCount);

  for (size_t i = 0; i < deviceCount; i++) {
    // TODO: resolve issue using emplace_back with private constructors
    physicalDevices.push_back(
        std::move(PhysicalDevice(instance, vkPhysicalDevices[i], surface)));
  }

  return physicalDevices;
}

PhysicalDevice::PhysicalDevice(PhysicalDevice&& other)
    : instance_(other.instance_),
      device_(other.device_),
      surface_(other.surface_) {
  // Take ownership of the underlying Vulkan physical device
  // It's OK if we already had a physicalDevice handle internally:
  // physical devices don't need to be explicitly destroyed in vulkan
  other.device_ = nullptr;
  other.queueFamilies_.clear();

  init();
}

PhysicalDevice::PhysicalDevice(Instance& instance, VkPhysicalDevice device, std::optional<VkSurfaceKHR> surface)
    : instance_(instance),
      device_(device),
      surface_(surface) {
  init();
}

void debugQueueFamily(const QueueFamily& family) {
  LOG_D("Queue Family:");
  LOG_D("\tIndex:\t{}", family.index);
  LOG_D("\tNum Queues:\t{}", family.count);
  LOG_D("\tGraphics:\t{}", family.graphics ? "YES" : "NO");
  LOG_D("\tCompute:\t{}", family.compute ? "YES" : "NO");
  LOG_D("\tTransfer:\t{}", family.transfer ? "YES" : "NO");
  LOG_D("\tSparse Binding:\t{}", family.sparseBinding ? "YES" : "NO");
  LOG_D("\tProtected:\t{}", family.protect ? "YES" : "NO");
  LOG_D("\tPresentation:\t{}", family.presentation ? "YES" : "NO");
}

void PhysicalDevice::init() {
  vkGetPhysicalDeviceProperties(device_, &deviceProperties_);
  vkGetPhysicalDeviceFeatures(device_, &deviceFeatures_);

  LOG_I("Device Name: {}", deviceProperties_.deviceName);
  LOG_I("Device Type: {}", getDeviceTypeString(deviceProperties_.deviceType));
  LOG_I("Device Features:");
  LOG_I("\tSupports Geometry Shader:\t{}",
        deviceFeatures_.geometryShader ? "YES" : "NO");
  LOG_I("\tSupports Tesselation Shader:\t{}",
        deviceFeatures_.tessellationShader ? "YES" : "NO");

  // Empty the vector so we can refill it without worrying about this function
  // being called multiple times
  queueFamilies_.clear();

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device_, &queueFamilyCount, nullptr);

  queueFamilies_.resize(queueFamilyCount);

  std::vector<VkQueueFamilyProperties> vkQueueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device_,
                                           &queueFamilyCount,
                                           vkQueueFamilies.data());

  for (size_t i = 0; i < queueFamilyCount; i++) {
    // Ensure we can always identify this queue, even if the vector gets sorted
    queueFamilies_[i].index = i;

    queueFamilies_[i].count = vkQueueFamilies[i].queueCount;

    // TODO: On Android, _all_ queues must be present-capable, so we can assume this is
    // always true on Android.
    if (surface_.has_value()) {
      VkBool32 presentationSupport = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(device_, i, surface_.value(), &presentationSupport);
      queueFamilies_[i].presentation = presentationSupport;
    } else {
      LOG_F("presentation detection not supported on this platform");
    }

    if (vkQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queueFamilies_[i].graphics = true;
    }

    if (vkQueueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      queueFamilies_[i].compute = true;
    }

    if (vkQueueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      queueFamilies_[i].transfer = true;
    }

    if (vkQueueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
      queueFamilies_[i].sparseBinding = true;
    }

    if (vkQueueFamilies[i].queueFlags & VK_QUEUE_PROTECTED_BIT) {
      queueFamilies_[i].protect = true;
    }

    debugQueueFamily(queueFamilies_[i]);
  }

  // TODO: The Vulkan spec states:
  //
  // The general expectation is that a physical device groups
  // all queues of matching capabilities into a single family.
  // However, while implementations should do this, it is possible
  // that a physical device may return two separate queue families
  // with the same capabilities.
  //
  // Maybe add code to detect this happening? That might be something
  // to push up into the application layer
}

VkPhysicalDeviceProperties PhysicalDevice::getProperties() const {
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(device_, &deviceProperties);

  return deviceProperties;
}

std::vector<QueueFamily> PhysicalDevice::getQueueFamilies() const {
  return queueFamilies_;
}

bool PhysicalDevice::hasAllExtensions(
    const DeviceExtensions& extensions) const {
  DeviceExtensions availableExtensions = getDeviceExtensions();

  bool hasAll = true;

  for (const auto& wantedExtension : extensions) {
    size_t count = std::count(availableExtensions.begin(),
                              availableExtensions.end(),
                              wantedExtension);
    if (count == 0) {
      LOG_W("Instance is missing extension '{}', which was requested",
            wantedExtension);
      hasAll = false;
    }
  }

  return hasAll;
}

DeviceExtensions PhysicalDevice::getDeviceExtensions() const {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device_,
                                       nullptr,
                                       &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> vkAvailableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device_,
                                       nullptr,
                                       &extensionCount,
                                       vkAvailableExtensions.data());

  DeviceExtensions availableExtensions(extensionCount);

  LOG_D("Available Device Extensions:");
  for (size_t i = 0; i < extensionCount; i++) {
    LOG_D("\t{}", vkAvailableExtensions[i].extensionName);
    availableExtensions[i] = vkAvailableExtensions[i].extensionName;
  }

  return availableExtensions;
}

SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(VkSurfaceKHR surface) const {
  SwapChainSupportDetails details;

  // Get generic swapchain support info
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_,
                                            surface,
                                            &details.capabilities);

  // Get supported texture formats
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device_, surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device_,
                                         surface,
                                         &formatCount,
                                         details.formats.data());
  }

  // Get presentation modes
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device_,
                                            surface,
                                            &presentModeCount,
                                            nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device_,
                                              surface,
                                              &presentModeCount,
                                              details.presentModes.data());
  }

  LOG_D("Capabilities:");
  LOG_D("\tCurrent Extent: {}x{}",
        details.capabilities.currentExtent.width,
        details.capabilities.currentExtent.height);
  LOG_D("\tImage Count: Min {}, Max {}",
        details.capabilities.minImageCount,
        details.capabilities.maxImageCount);
  LOG_D("\tMax Image Array Layers: Max {}",
        details.capabilities.maxImageArrayLayers);
  LOG_D("\tImage Extent: Min {}x{}, Max {}x{}",
        details.capabilities.minImageExtent.width,
        details.capabilities.minImageExtent.height,
        details.capabilities.maxImageExtent.width,
        details.capabilities.maxImageExtent.height);

  std::vector<std::string> supportedTransforms =
      vkSurfaceTransformFlagsToString(details.capabilities.supportedTransforms);

  LOG_D("\tSupported Transforms: ");

  for (const auto& transformStr : supportedTransforms) {
    LOG_D("\t\t{}, ", transformStr);
  }

  std::vector<std::string> currentTransforms =
      vkSurfaceTransformFlagsToString(details.capabilities.currentTransform);

  LOG_D("\tCurrent Transform: ");

  for (const auto& transformStr : currentTransforms) {
    LOG_D("\t\t{}, ", transformStr);
  }

  for (const auto& format : details.formats) {
    LOG_D("Format: {}, {}\n",
          vkFormatToString(format.format),
          vkColorspaceKHRToString(format.colorSpace));
  }

  for (const auto& presentMode : details.presentModes) {
    LOG_D("Present Mode: {}\n", vkPresentModeKHRToString(presentMode));
  }

  return details;
}

struct QueueRequestsForSingleFamily {
  uint32_t familyIndex;

  // Note: the ordering of priorities and originalRequests must match exactly!
  std::vector<float> priorities;
  std::vector<std::reference_wrapper<QueueFamilyRequest>> originalRequests;
};

LogicalDevice::LogicalDevice(Instance& instance,
                             PhysicalDevice&& physicalDevice,
                             const DeviceExtensions& requiredExtensions,
                             QueueFamilyRequests& requests)
    : instance_(instance), physicalDevice_(std::move(physicalDevice)) {
  // Step 1: figure out how many different queue families were requested
  // Map QueueFamily Index -> QueueFamilyRequest
  std::multimap<uint32_t, QueueFamilyRequest&> familyRequests;

  // De-duplicate the QueueFamily indexes
  std::set<uint32_t> families;

  for (auto& request : requests) {
    familyRequests.insert({request.get().family.index, request});
    families.insert(request.get().family.index);
  }

  // The number of unique families we are requesting queues for
  size_t numFamilies = families.size();

  std::vector<QueueRequestsForSingleFamily> sortedRequests(numFamilies);
  size_t i = 0;

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(numFamilies);

  // Step 2: figure out how many queues from each family were requested, and
  // pair up with the list of priorities
  for (const auto& familyIndex : families) {
    size_t numRequestsForFamily = familyRequests.count(familyIndex);

    if (numRequestsForFamily == 0) {
      LOG_F(
          "Logic error: did not find matching request for family {} in "
          "familyRequests",
          familyIndex);
    }

    sortedRequests[i].familyIndex = familyIndex;
    sortedRequests[i].originalRequests.reserve(numRequestsForFamily);
    sortedRequests[i].priorities.reserve(numRequestsForFamily);

    auto it = familyRequests.equal_range(familyIndex);

    for (auto itr = it.first; itr != it.second; ++itr) {
      sortedRequests[i].originalRequests.push_back(itr->second);
      sortedRequests[i].priorities.push_back(itr->second.priority);
    }

    // Build the create info
    queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[i].queueFamilyIndex = familyIndex;
    queueCreateInfos[i].queueCount = numRequestsForFamily;
    queueCreateInfos[i].pQueuePriorities = sortedRequests[i].priorities.data();

    i++;
  }

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  // Submit all the queue create infos to Vulkan
  createInfo.queueCreateInfoCount = queueCreateInfos.size();
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  // TODO: enable device features
  VkPhysicalDeviceFeatures deviceFeatures{};
  createInfo.pEnabledFeatures = &deviceFeatures;

  if (!physicalDevice_.hasAllExtensions(requiredExtensions)) {
    LOG_F("Physical device does not have all required extensions");
  }

  std::vector<const char*> vkDeviceExtensions(requiredExtensions.size());
  for (size_t i = 0; i < requiredExtensions.size(); i++) {
    vkDeviceExtensions[i] = requiredExtensions[i].c_str();
  }

  createInfo.enabledExtensionCount = vkDeviceExtensions.size();
  createInfo.ppEnabledExtensionNames = vkDeviceExtensions.data();

  // Device layers are a deprecated Vulkan feature and can likely be ignored in
  // future. HOWEVER:
  //
  // See:
  // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#extendingvulkan-layers-devicelayerdeprecation
  //
  // The Vulkan spec says:
  //
  // In order to maintain compatibility with implementations released prior to
  // device-layer deprecation, applications should still enumerate and enable
  // device layers. The behavior of vkEnumerateDeviceLayerProperties and valid
  // usage of the ppEnabledLayerNames member of VkDeviceCreateInfo maximizes
  // compatibility with applications written to work with the previous
  // requirements.
  //
  // The list of layers enumerated by vkEnumerateDeviceLayerProperties must be
  // exactly the sequence of layers enabled for the instance. The members of
  // VkLayerProperties for each enumerated layer must be the same as the
  // properties when the layer was enumerated by
  // vkEnumerateInstanceLayerProperties.
  //
  // So, TL;DR: we need to pass in the same set of layer parameters as we did
  // for the Instance. Lets query the instance to figure out what layers we need
  // to enable.

  std::vector<const char*> vkValidationLayers(
      instance_.getEnabledLayers().size());

  for (size_t i = 0; i < instance_.getEnabledLayers().size(); i++) {
    vkValidationLayers[i] = instance_.getEnabledLayers()[i].c_str();
  }

  createInfo.enabledLayerCount = vkValidationLayers.size();
  createInfo.ppEnabledLayerNames = vkValidationLayers.data();

  if (vkCreateDevice(physicalDevice_.device_, &createInfo, nullptr, &device_) !=
      VK_SUCCESS) {
    LOG_F("failed to create logical device!");
  }

  // OK! So now we have a logical device. Retrieve the queues for each family
  // and assign to the respective requests

  for (auto& request : sortedRequests) {
    for (size_t j = 0; j < request.originalRequests.size(); j++) {
      vkGetDeviceQueue(device_,
                       request.familyIndex,
                       j,
                       &request.originalRequests[j].get().allocatedQueue);

      if (request.originalRequests[j].get().allocatedQueue == VK_NULL_HANDLE) {
        LOG_F("Failed to get queue for request");
      }
    }
  }
}

LogicalDevice::LogicalDevice(LogicalDevice&& other)
    : instance_(other.instance_), physicalDevice_(std::move(other.physicalDevice_)) {
  if (device_) {
    vkDestroyDevice(device_, nullptr);
  }

  // No need to take ownership of queues, since they don't need to be
  // explicitly tracked or destroyed
}

LogicalDevice::~LogicalDevice() {
  if (device_) {
    vkDestroyDevice(device_, nullptr);
  }
}