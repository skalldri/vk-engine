#include "Instance.hpp"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <fmtlog/Log.hpp>

const Extensions debugExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
const Layers debugLayers = {"VK_LAYER_KHRONOS_validation"};

Instance::Instance(const std::string& applicationName,
                   const Version& applicationVersion, bool enableDebugMessages,
                   const Extensions& extensions, const Layers& layers) {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = applicationName.c_str();
  appInfo.applicationVersion = VK_MAKE_VERSION(std::get<0>(applicationVersion),
                                               std::get<1>(applicationVersion),
                                               std::get<2>(applicationVersion));

  appInfo.pEngineName = "VK_ENGINE";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); // TODO: use project version from CMake

  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

  createInfo.pApplicationInfo = &appInfo;

  Extensions requiredExtensions = extensions;
  Layers requiredLayers = layers;

  Extensions availableExtensions = getInstanceExtensions();
  Layers availableLayers = getInstanceLayers();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableDebugMessages) {
    // Check if the Debug messages extension is supported by our Vulkan instance
    if (!instanceHasAllExtensions(debugExtensions) || !instanceHasAllLayers(debugLayers)) {
      LOG_E("Cannot enable debug messages: required extension(s) or layer(s) not available");
    } else {
      // Add the requried extension to the list of required extensions passed in by the user:
      requiredExtensions.insert(requiredExtensions.end(), debugExtensions.begin(), debugExtensions.end());

      // Add the required layers to the list of required layers passed in by the user
      requiredLayers.insert(requiredLayers.end(), debugLayers.begin(), debugLayers.end());

      debugCreateInfo = {};
      debugCreateInfo.sType =
          VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      
      // TODO: implement message severity filtering. 
      // Maybe do this at runtime in the log message handler?
      debugCreateInfo.messageSeverity =
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

      // TODO: implement message type filtering
      // Maybe do this at runtime in the log message handler?
      debugCreateInfo.messageType =
          VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      debugCreateInfo.pfnUserCallback = debugCallbackShim;
      debugCreateInfo.pUserData = this;

      // Add the debugCreateInfo to the main vkInstance createInfo so that both get initialized
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
  }

  std::vector<const char*> vkRequiredExtensions(requiredExtensions.size());

  for (size_t i = 0; i < requiredExtensions.size(); i++) {
    vkRequiredExtensions[i] = requiredExtensions[i].c_str();
  }

  // Set the requested extensions
  createInfo.enabledExtensionCount = vkRequiredExtensions.size();
  createInfo.ppEnabledExtensionNames = vkRequiredExtensions.data();

  std::vector<const char*> vkRequiredlayers(requiredLayers.size());

  for (size_t i = 0; i < layers.size(); i++) {
    vkRequiredlayers[i] = requiredLayers[i].c_str();
  }

  createInfo.enabledLayerCount = vkRequiredlayers.size();
  createInfo.ppEnabledLayerNames = vkRequiredlayers.data();

  if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
    LOG_F("Failed to create Vulkan instance");
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Instance::debugCallbackShim(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  Instance* pThis = static_cast<Instance*>(pUserData);
  return pThis->debugCallback(messageSeverity, messageType, pCallbackData);
}

VkBool32 Instance::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData) {
  std::string type = "INVALID TYPE";

  // TODO: implement message type filtering
  if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
    type = "GENERAL";
  } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
    type = "VALIDATION";
  } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
    type = "PERFORMANCE";
  }

  // TODO: implement message severity filtering
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    // Map 'verbose' messages to fmtlog's 'debug' severity
    LOG_D("[VK] [{}]: {}", type, pCallbackData->pMessage);
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    LOG_I("[VK] [{}]: {}", type, pCallbackData->pMessage);
  } else if (messageSeverity &
             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    LOG_W("[VK] [{}]: {}", type, pCallbackData->pMessage);
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    LOG_E("[VK] [{}]: {}", type, pCallbackData->pMessage);
  }

  return VK_FALSE;
}

Instance::~Instance() {
  vkDestroyInstance(instance_, nullptr);
}

Extensions Instance::getInstanceExtensions() {
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> vkExtensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         vkExtensions.data());

  Extensions extensions(extensionCount);

  for (size_t i = 0; i < extensionCount; i++) {
    extensions[i] = vkExtensions[i].extensionName;
  }

  return extensions;
}

Layers Instance::getInstanceLayers() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> vkLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, vkLayers.data());

  std::vector<Layer> layers(layerCount);

  for (size_t i = 0; i < layerCount; i++) {
    layers[i] = vkLayers[i].layerName;
  }

  return layers;
}

bool Instance::instanceHasAllExtensions(const Extensions& extensions) {
  Extensions availableExtensions = getInstanceExtensions();

  bool hasAll = true;

  for (const auto& wantedExtension : extensions) {
    size_t count = std::count(availableExtensions.begin(),
                              availableExtensions.end(), wantedExtension);
    if (count == 0) {
      LOG_W("Instance is missing extension '{}', which was requested",
            wantedExtension);
      hasAll = false;
    }
  }

  return hasAll;
}

bool Instance::instanceHasAllLayers(const Layers& layers) {
  Layers availableLayers = getInstanceLayers();

  bool hasAll = true;

  for (const auto& wantedLayer : layers) {
    size_t count =
        std::count(availableLayers.begin(), availableLayers.end(), wantedLayer);
    if (count == 0) {
      LOG_W("Instance is missing layer '{}', which was requested", wantedLayer);
      hasAll = false;
    }
  }

  return hasAll;
}
