#pragma once

#include <vulkan/vulkan.h>

#include <engine/core/Instance.hpp>
#include <tuple>
#include <vector>
#include <optional>

class LogicalDevice;
class PhysicalDevice;

struct QueueFamily {
  bool graphics;
  bool compute;
  bool transfer;
  bool sparseBinding;
  bool protect;  // Can't call this "protected" because it's a language
                 // keyword... :/
  bool presentation;
  uint32_t count;
  uint32_t index;
};

// Type for requesting a Queue from a QueueFamily
// QueueFamily: the queue family to allocate from
// float: the queue priority (0 -> 1, where 0 is lowest priority and 1 is
// highest priority)
class QueueFamilyRequest {
 public:
  QueueFamily family;
  float priority = -1.0f; // Initialize with negative priority to indicate bad state

  const VkQueue& getQueue() {return allocatedQueue; }

 protected:
  friend LogicalDevice;
  VkQueue allocatedQueue = VK_NULL_HANDLE;
};

using QueueFamilyRequests = std::vector<std::reference_wrapper<QueueFamilyRequest>>;

using DeviceExtensions = std::vector<std::string>;

/**
 * @brief LogicalDevices represent a VkDevice. LogicalDevices must be "derived"
 * or "bound" to a corresponding PhysicalDevice: for that reason, we don't allow
 * LogicalDevices to be instantiated directly. Instead, they can be generated by
 * an instance of the PhysicalDevice class
 */
class LogicalDevice {
 public:
  ~LogicalDevice();

  // No default constructor, no copy constructor
  LogicalDevice() = delete;
  LogicalDevice(LogicalDevice& other) = delete;

  // Allow move constructor
  LogicalDevice(LogicalDevice&& other);

  // Normal construction
  LogicalDevice(Instance& instance,
                const PhysicalDevice& physicalDevice,
                const DeviceExtensions& requredExtensions,
                QueueFamilyRequests& requests);

  // TODO: REMOVE THIS, ONLY FOR INTEGRGATION WITH ORIGINAL ENGINE
  VkDevice getVkDevice() const {return device_; }

 private:
  Instance& instance_;
  std::vector<VkQueue> queues_;
  VkDevice device_ = nullptr;
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

/**
 * @brief Represents a physical piece of hardware on the system, capable of
 * hosting Vulkan. An instance of a PhysicalDevice is required to instantiate a
 * LogicalDevice
 *
 */
class PhysicalDevice {
 public:
  // No default constructor
  PhysicalDevice() = delete;

  // No copy constructor
  PhysicalDevice(PhysicalDevice& other) = delete;

  // Move constructor
  PhysicalDevice(PhysicalDevice&& other);

  // Destructor has to be public for most things to work
  ~PhysicalDevice() = default;

  std::vector<QueueFamily> getQueueFamilies() const;

  static std::vector<PhysicalDevice> getPhysicalDevices(Instance& instance, std::optional<VkSurfaceKHR> surface);

  DeviceExtensions getDeviceExtensions() const;

  bool hasAllExtensions(const DeviceExtensions& extensions) const;

  SwapChainSupportDetails querySwapChainSupport(VkSurfaceKHR surface) const;

  // TODO: REMOVE THIS, ONLY FOR INTEGRGATION WITH ORIGINAL ENGINE
  VkPhysicalDevice getVkPhysicalDevice() const {return device_; }

 private:
  PhysicalDevice(Instance& instance, VkPhysicalDevice device, std::optional<VkSurfaceKHR> surface);

  void init();

 protected:
  friend LogicalDevice;
  VkPhysicalDevice device_ = nullptr;

 private:
  Instance& instance_;
  VkPhysicalDeviceProperties deviceProperties_;
  VkPhysicalDeviceFeatures deviceFeatures_;
  std::vector<QueueFamily> queueFamilies_;
  std::optional<VkSurfaceKHR> surface_;
};
