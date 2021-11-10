#include <iostream>
#include <string>
#include <exception>
#include <optional>

#include <fmt/core.h>

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

#include <vulkan/to_string.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

using namespace std;

GLFWwindow* window;
VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;

constexpr size_t window_width = 800;
constexpr size_t window_height = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef DEBUG_BUILD
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkSurfaceKHR surface;
VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

std::vector<VkImageView> swapChainImageViews;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::string severity = "INVALID SEVERITY";

    // TODO: implement message severity filtering
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        severity = "VERBOSE";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        severity = "INFO";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        severity = "WARNING";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        severity = "ERROR";
    }

    std::string type = "INVALID TYPE";

    // TODO: implement message severity filtering
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        type = "GENERAL";
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        type = "VALIDATION";
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        type = "PERFORMANCE";
    }

    fmt::print("[VK] [{}] [{}] {}\n", severity, type, pCallbackData->pMessage);

    return VK_FALSE;
}

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool allLayersFound = true;
    for (auto requestedLayer : validationLayers) {
        bool layerPresent = false;

        for (auto availableLayer : availableLayers) {
            if (strcmp(requestedLayer, availableLayer.layerName) == 0) {
                layerPresent = true;
                break;
            }
        }

        if (layerPresent) {
            fmt::print("Layer {} is availabe\n", requestedLayer);
        } else {
            fmt::print("Layer {} is not available\n", requestedLayer);
            allLayersFound = false;
        }
    }

    return allLayersFound;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional
}

void setupDebugMessenger() {
    if (!enableValidationLayers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to setup debug message hook");
    }
}

void initWindow()
{
    // Initialize GLFW
    if (!glfwInit()) {
        fmt::print("Failed to init GLFW\n");
        throw std::runtime_error("GLFW Init Error");
    }

    fmt::print("GLFW initialized\n");

    if (glfwVulkanSupported())
    {
        // Vulkan is available, at least for compute
        fmt::print("GLFW detected vulkan support\n");
    } else {
        fmt::print("GLFW did not detect vulkan support\n");
        throw std::runtime_error("GLFW No Vulkan Support");
    }

    // Tell GLFW we don't want it to assume a client API (since GLFW defaults to OpenGL support)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Tell GLFW not to allow window resizing. Might revisit this later, but this is fine for now
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(window_width, window_height, "VK Engine", nullptr, nullptr);

}

std::vector<const char*> getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    // Query GLFW to figure out what extensions it needs from Vulkan
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    for (auto extension : extensions) {
        fmt::print("Required Extension: {}\n", extension);
    }

    return extensions;
}

void createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    fmt::print("Available Vulkan Extensions:\n");
    for (const auto& extension : extensions) {
        fmt::print("\t{}\n", extension.extensionName);
    }

    std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
    
    // Set the requested extensions from GLFW
    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers && checkValidationLayerSupport()) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

std::vector<VkPhysicalDevice> listPhysicalDevices() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);

    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    return devices;
}

const char* getDeviceTypeString(VkPhysicalDeviceType type) {
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

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device) {
    QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& family : queueFamilies) {
        std::vector<std::string> queueFlags;

        // Check if this queueFamily supports presentation
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            if (!indices.presentFamily.has_value()) {
                indices.presentFamily = i;
            }

            queueFlags.push_back("PRESENT");
        }

        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (!indices.graphicsFamily.has_value()) {
                // Indicate that this queue index can support gfx commands
                indices.graphicsFamily = i;
            }

            queueFlags.push_back("GRAPHICS");
        }

        if (family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFlags.push_back("COMPUTE");
        }

        if (family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            queueFlags.push_back("TRANSFER");
        }

        if (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
            queueFlags.push_back("SPARSE BINDING");
        }

        if (family.queueFlags & VK_QUEUE_PROTECTED_BIT) {
            queueFlags.push_back("PROTECTED");
        }

        fmt::print("Queue Count: {}\n", family.queueCount);
        fmt::print("Queue Flags: ");

        for (const auto& flag : queueFlags) {
            fmt::print("{}, ", flag);
        }

        fmt::print("\n");

        i++;
    }

    return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    for (const auto& extension : availableExtensions) {
        fmt::print("Available Device Extension: {}\n", extension.extensionName);
    }

    // Make a copy of the list of required extensions that we can locally modify
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    // Get generic swapchain support info
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Get supported texture formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // Get presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    fmt::print("Capabilities:\n");
    fmt::print("\tCurrent Extent: {}x{}\n", details.capabilities.currentExtent.width, details.capabilities.currentExtent.height);
    fmt::print("\tImage Count: Min {}, Max {}\n", details.capabilities.minImageCount, details.capabilities.maxImageCount);
    fmt::print("\tMax Image Array Layers: Max {}\n", details.capabilities.maxImageArrayLayers);
    fmt::print("\tImage Extent: Min {}x{}, Max {}x{}\n",
        details.capabilities.minImageExtent.width,
        details.capabilities.minImageExtent.height,
        details.capabilities.maxImageExtent.width,
        details.capabilities.maxImageExtent.height);

    std::vector<std::string> supportedTransforms = vkSurfaceTransformFlagsToString(details.capabilities.supportedTransforms);

    fmt::print("\tSupported Transforms: ");

    for (const auto& transformStr : supportedTransforms) {
        fmt::print("{}, ", transformStr);
    }

    fmt::print("\n");

    std::vector<std::string> currentTransforms = vkSurfaceTransformFlagsToString(details.capabilities.currentTransform);

    fmt::print("\tCurrent Transform: ");

    for (const auto& transformStr : currentTransforms) {
        fmt::print("{}, ", transformStr);
    }

    fmt::print("\n");

    for (const auto& format : details.formats) {
        fmt::print("Format: {}, {}\n", vkFormatToString(format.format), vkColorspaceKHRToString(format.colorSpace));
    }

    for (const auto& presentMode : details.presentModes) {
        fmt::print("Present Mode: {}\n", vkPresentModeKHRToString(presentMode));
    }

    return details;
}

bool isDeviceSuitable(const VkPhysicalDevice& device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    fmt::print("Device Name: {}\n", deviceProperties.deviceName);
    fmt::print("Device Type: {}\n", getDeviceTypeString(deviceProperties.deviceType));
    fmt::print("Has Geometry Shader: {}\n", deviceFeatures.geometryShader ? "YES" : "NO");

    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;

    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        // TODO: more explicit swapchain requirements
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && 
        deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader && 
        extensionsSupported &&
        swapChainAdequate;
}

void pickPhysicalDevice() {
    std::vector<VkPhysicalDevice> deviceList = listPhysicalDevices();

    for (const auto& device : deviceList) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    fmt::print("Physical Device '{}' is suitable for engine use, and has been selected\n", deviceProperties.deviceName);
}

void createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // The graphics and presentation queues might share the same queue family index,
    // so use a set to de-duplicate the list of indexes
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    // Submit all the queue create infos to Vulkan
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    if (!indices.isComplete()) {
        throw std::runtime_error("cannot create logical device without a graphics queue");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // Pick first format if our preferred SRGBA8888 format isn't available
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {

    // Pick MAILBOX present mode (effectively tripple buffering) if available
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR; // This is guaranteed to be available by the Vulkan spec
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {

    // If the Width / Height are UINT32_MAX, then our underlying Vulkan implementation
    // is going to pick the correct size extent to fill the window automatically, and we
    // can just return that value
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            width, height
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.width = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // The minimum number of images required in the swap chain
    // We ask for one additional image to help prevent us from getting stuck waiting for the driver
    // to give us another image.
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // Make sure the driver actually supports this many images though...
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    fmt::print("Initializing SwapChain with these paramters:\n");
    fmt::print("\tFormat: {}, {}\n", vkFormatToString(surfaceFormat.format), vkColorspaceKHRToString(surfaceFormat.colorSpace));
    fmt::print("\tPresent Mode: {}\n", vkPresentModeKHRToString(presentMode));
    fmt::print("\tExtent: {}x{}\n", extent.width, extent.height);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    // If the graphics and present queues are separate, then we need to indicate
    // which queue indices are going to be allowed to operate on the swapchain and pick the correct operating mode
    if (indices.graphicsFamily.value() != indices.presentFamily.value()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Poor performance, easier to use
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
        fmt::print("Warning: using concurrent queue family sharing for swapchain, expect reduced performance\n");
        fmt::print("Graphics family: {}, Present Family: {}\n", indices.graphicsFamily.value(), indices.presentFamily.value());
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Best performance
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    // Specify that we don't want any transform on the image by setting the preTransform to the currentTransform
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // Should this be blended with other windows in the windowing system?
    // Set to opaque for no
    // TODO: can this create transparent / blurred windows?
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;

    // We don't care about the contents of pixels obscurred by other windows
    createInfo.clipped = VK_TRUE;

    // Used when we want to re-create the swapchain, for example if the window gets resized. NULL for now.
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // Retrieve the images from the swapchain
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void createSurface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
}

void createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Treat the image as a 2D array of pixels
        createInfo.format = swapChainImageFormat; // Match the format of the underlying view

        // Allows us to swizzle one color channel into another, but for now
        // we just stick with the default
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Tell Vulkan we only need a single MIP level, MIP0, because
        // we don't care about MIP levels for the final rendered image that we
        // are going to display to the user
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;

        // Since this is currently a 2D application, we only need one layer per VkImage
        // but for a VR application, we might request a VkImage with multiple layers, and then
        // bind each layer of a single VkImage into multiple VkImageViews
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void createGraphicsPipeline() {

}

void initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
}

void mainLoop()
{
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void cleanup()
{
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main(int argc, char** argv)
{
    cout << "Hello world" << endl;

    fmt::print("Hello world from {}\n", "fmtlib");

    CLI::App app{"Vulkan Engine"};

    std::string filename = "default";
    app.add_option("-f,--file", filename, "A help string");

    CLI11_PARSE(app, argc, argv);

    initWindow();
    initVulkan();

    mainLoop();

    cleanup();

    return 0;
}