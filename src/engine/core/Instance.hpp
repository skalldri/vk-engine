#pragma once

#include <vulkan/vulkan.h>

#include <set>
#include <string>
#include <tuple>
#include <vector>

using Version = std::tuple<uint32_t, uint32_t, uint32_t>;

using Extension = std::string;
using Extensions = std::vector<Extension>;

using Layer = std::string;
using Layers = std::vector<Layer>;

class Instance {
 public:

  // No default constructor
  Instance() = delete;

  // No copy-constructor
  Instance(Instance& inst) = delete;

  /**
   * @brief Construct a new Instance object
   *
   * @param applicationName the name of the application that will be passed to
   * Vulkan
   * @param applicationVersion the version of the application, which willm be
   * passed to Vulkan
   * @param enableDebugMessages if true, print Vulkan debug messages to the
   * logger
   * @param extensions a list of extensions that should be enabled
   * @param layers a list of layers that should be enabled
   */
  Instance(const std::string& applicationName,
           const Version& applicationVersion, bool enableDebugMessages,
           const Extensions& extensions, const Layers& layers);

  ~Instance();

  /**
   * @brief Get the Instance object
   * DEBUG ONLY WHILE INTEGRATING INTO THE CURRENT CODE
   * @return VkInstance 
   */
  VkInstance getInstance() {return instance_; }

  /**
   * @brief Get the list of extensions supported by this instance
   *
   * @return Extensions the list of supported extensions
   */
  static Extensions getInstanceExtensions();

  /**
   * @brief Check if the underlying Vulkan instance has all the specficied
   * extensions
   *
   * @param extensions the list of extensions to test the instance for
   * @return true the instance has all the extensions specified in `extensions`
   * @return false the instance does not have all the extensions specified in
   * `extensions`
   */
  static bool instanceHasAllExtensions(const Extensions& extensions);

  /**
   * @brief Get the list of layers supported by this instance
   *
   * @return Layers the list of supported layers
   */
  static Layers getInstanceLayers();

  /**
   * @brief Check if the underlying Vulkan instance has all the specficied
   * layers
   *
   * @param layers the list of layers to test the instance for
   * @return true the instance has all the layers specified in `layers`
   * @return false the instance does not have all the layers specified in
   * `layers`
   */
  static bool instanceHasAllLayers(const Layers& layers);

 private:
  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallbackShim(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                    void* pUserData);

  VkBool32 debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

 private:
  VkInstance instance_;
};