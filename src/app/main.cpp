#include <fmt/core.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <array>
#include <engine/core/Device.hpp>
#include <engine/core/Instance.hpp>
#include <engine/core/Swapchain.hpp>
#include <engine/utils/to_string.hpp>

#include <engine/win32/GlfwWindowSystem.hpp>

#include <exception>
#include <fmtlog/Log.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

using namespace std;

GlfwWindowSystem* windowSystem;

Instance *instanceCpp;
Swapchain *swapchain;

VkDebugUtilsMessengerEXT debugMessenger;

constexpr size_t window_width = 800;
constexpr size_t window_height = 600;

const Layers validationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<std::string> deviceExtensionsCpp = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef DEBUG_BUILD
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

// TODO: should this match the number of images in the Swapchain?
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

LogicalDevice *deviceCpp;

QueueFamilyRequest graphicsQueueRequest;
QueueFamilyRequest presentationQueueRequest;

VkSurfaceKHR surface;

// VkSwapchainKHR swapChain;
// VkFormat swapChainImageFormat;
// VkExtent2D swapChainExtent;

VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
VkCommandPool commandPool;

std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;
std::vector<VkFence> imagesInFlight;

// std::vector<VkImage> swapChainImages;
std::vector<VkCommandBuffer> commandBuffers;
std::vector<VkFramebuffer> swapChainFramebuffers;
std::vector<VkImageView> swapChainImageViews;

size_t currentFrame = 0;

bool framebufferResized = false;

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  std::string severity = "INVALID SEVERITY";

  // TODO: implement message severity filtering
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    severity = "VERBOSE";
  } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    severity = "INFO";
  } else if (messageSeverity &
             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
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
      if (strcmp(requestedLayer.c_str(), availableLayer.layerName) == 0) {
        layerPresent = true;
        break;
      }
    }

    if (layerPresent) {
      LOG_I("Layer {} is availabe\n", requestedLayer);
    } else {
      LOG_W("Layer {} is not available\n", requestedLayer);
      allLayersFound = false;
    }
  }

  return allLayersFound;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance,
      "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance,
      "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;  // Optional
}

void initWindow() {
  windowSystem = new GlfwWindowSystem();
}

void createInstance() {
  instanceCpp = new Instance("Hello Triangle",
                             {1, 0, 0},  // App Version
                             enableValidationLayers,
                             windowSystem->getRequiredVkInstanceExtensions(),
                             {}  // required layers
  );
}

bool isDeviceSuitable(const PhysicalDevice &device) {
  // Check if this device has an appropriate queue family
  bool hasGraphicsFamily = false;
  bool hasPresentFamily = false;
  for (const auto &family : device.getQueueFamilies()) {
    if (family.graphics) {
      hasGraphicsFamily = true;
    }

    if (family.presentation) {
      hasPresentFamily = true;
    }
  }

  bool extensionsSupported = device.hasAllExtensions(deviceExtensionsCpp);

  bool swapChainAdequate = false;

  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport =
        device.querySwapChainSupport(surface);
    // TODO: more explicit swapchain requirements
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  return hasGraphicsFamily && hasPresentFamily &&
         // deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
         // && deviceFeatures.geometryShader &&
         extensionsSupported && swapChainAdequate;
}

std::optional<PhysicalDevice> pickPhysicalDevice() {
  std::vector<PhysicalDevice> deviceList =
      PhysicalDevice::getPhysicalDevices(*instanceCpp, surface);

  for (auto &device : deviceList) {
    if (isDeviceSuitable(device)) {
      LOG_I(
          "Physical Device '{}' is suitable for engine use, and has been "
          "selected",
          device.getProperties().deviceName);

      return std::move(device);
    }
  }

  return std::nullopt;
}

void createLogicalDevice(PhysicalDevice &&physicalDevice) {
  std::vector<QueueFamily> availableQueueFamilies =
      physicalDevice.getQueueFamilies();

  // Pick appropriate queues to use from the availableQueueFamilies
  // On Windows+Nvidia:
  // - Don't do anything fancy: just pick N queues from the only one that
  // supports Graphics + Presentation
  //
  // On Other platforms:
  // - TBD

  for (auto &family : availableQueueFamilies) {
    if (graphicsQueueRequest.priority < 0.0f && family.graphics) {
      graphicsQueueRequest.family = family;
      graphicsQueueRequest.priority = 1.0f;
    }

    if (presentationQueueRequest.priority < 0.0f && family.presentation) {
      presentationQueueRequest.family = family;
      presentationQueueRequest.priority = 1.0f;
    }
  }

  if (graphicsQueueRequest.priority < 0.0f ||
      presentationQueueRequest.priority < 0.0f) {
    LOG_F("Failed to pick a graphics and presentation queue");
  }

  QueueFamilyRequests requests = {graphicsQueueRequest,
                                  presentationQueueRequest};

  deviceCpp = new LogicalDevice(*instanceCpp,
                                std::move(physicalDevice),
                                deviceExtensionsCpp,
                                requests);

  if (graphicsQueueRequest.getQueue() == VK_NULL_HANDLE ||
      presentationQueueRequest.getQueue() == VK_NULL_HANDLE) {
    LOG_F("Failed to get valid queue");
  }
}

void createSwapChain(const LogicalDevice& device) {

  swapchain = new Swapchain(device, surface, {graphicsQueueRequest, presentationQueueRequest});

  /*
  // We are presenting to a particular kind of Surface.
  // Surfaces represent a way that the OS can display content to the user.
  // We need to query the underlying OS to determine what Swapchain formats this
  // Surface could display. As a concrete example, the OS compositor might not
  // support 16-bit color formats. If we created a swapchain that used 16-bit
  // color formats, we wouldn't be able to present it. So, query the underlying
  // surface to figure out what swapchain format we should generate
  SwapChainSupportDetails swapChainSupport =
      deviceCpp->getPhysicalDevice().querySwapChainSupport(surface);

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
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {graphicsQueueRequest.family.index,
                                   presentationQueueRequest.family.index};

  // If the graphics and present queues are separate, then we need to indicate
  // which queue indices are going to be allowed to operate on the swapchain and
  // pick the correct operating mode
  if (graphicsQueueRequest.family.index !=
      presentationQueueRequest.family.index) {
    createInfo.imageSharingMode =
        VK_SHARING_MODE_CONCURRENT;  // Poor performance, easier to use
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
    LOG_W(
        "Using concurrent queue family sharing for swapchain, expect "
        "reduced performance\n");
    LOG_W("Graphics family: {}, Present Family: {}\n",
          graphicsQueueRequest.family.index,
          presentationQueueRequest.family.index);
  } else {
    createInfo.imageSharingMode =
        VK_SHARING_MODE_EXCLUSIVE;             // Best performance
    createInfo.queueFamilyIndexCount = 0;      // Optional
    createInfo.pQueueFamilyIndices = nullptr;  // Optional
  }

  // Specify that we don't want any transform on the image by setting the
  // preTransform to the currentTransform
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

  if (vkCreateSwapchainKHR(deviceCpp->getVkDevice(),
                           &createInfo,
                           nullptr,
                           &swapChain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  // Retrieve the images from the swapchain
  vkGetSwapchainImagesKHR(deviceCpp->getVkDevice(),
                          swapChain,
                          &imageCount,
                          nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(deviceCpp->getVkDevice(),
                          swapChain,
                          &imageCount,
                          swapChainImages.data());

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
  */
}

void createSurface() {
  surface = windowSystem->createSurface(*instanceCpp);
}

void createImageViews() {
  swapChainImageViews.resize(swapchain->getImages().size());

  for (size_t i = 0; i < swapchain->getImages().size(); i++) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapchain->getImages()[i];
    createInfo.viewType =
        VK_IMAGE_VIEW_TYPE_2D;  // Treat the image as a 2D array of pixels
    createInfo.format =
        swapchain->getFormat();  // Match the format of the underlying view

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

    // Since this is currently a 2D application, we only need one layer per
    // VkImage but for a VR application, we might request a VkImage with
    // multiple layers, and then bind each layer of a single VkImage into
    // multiple VkImageViews
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(deviceCpp->getVkDevice(),
                          &createInfo,
                          nullptr,
                          &swapChainImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image views!");
    }
  }
}

static std::vector<char> readBinaryFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error(fmt::format("failed to open file {}", filename));
  }

  size_t fileSize = file.tellg();
  std::vector<char> contents(fileSize);

  file.seekg(0);
  file.read(contents.data(), fileSize);
  file.close();

  return contents;
}

VkShaderModule createShaderModule(const std::vector<char> &shaderSpirv) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = shaderSpirv.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(shaderSpirv.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(deviceCpp->getVkDevice(),
                           &createInfo,
                           nullptr,
                           &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

void createRenderPass() {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapchain->getFormat();
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkSubpassDependency dependency{};

  // VK_SUBPASS_EXTERNAL refers to the implicit subpass that occurrs before our
  // current renderPass. Effectively, this refers to the previous frame
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;

  // dstSubpass == our current renderPass (which is the only subpass we will
  // have for this frame)
  dependency.dstSubpass = 0;

  // The "source" of our dependency is, effectively, the color attachment of the
  // previous frame. we can't write to the color attachment until the previous
  // frame has been scanned out
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;

  // Destination of our dependency is the color attachment for the current
  // frame. And we are not allowed to write to the color attachment until it's
  // complete. We are allowed to read it, for example.
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(deviceCpp->getVkDevice(),
                         &renderPassInfo,
                         nullptr,
                         &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}

void createGraphicsPipeline() {
  auto vertexShader = readBinaryFile("shaders/shader.vert.spv");
  auto fragmentShader = readBinaryFile("shaders/shader.frag.spv");

  // Once we have bound these modules to a graphics pipeline, we don't need to
  // keep them around and so we can let them go out of scope (after destroying
  // them, of course)
  VkShaderModule vertShaderModule = createShaderModule(vertexShader);
  VkShaderModule fragShaderModule = createShaderModule(fragmentShader);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  // Dummy vertex shader input descriptor that indicates we aren't going to
  // upload anything (since the vertices are compiled in already)
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;  // Optional
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;  // Optional

  // Describe the kinds of input we're going to load
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapchain->getExtent().width;
  viewport.height = (float)swapchain->getExtent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchain->getExtent();

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable =
      VK_FALSE;  // If TRUE, clamp depth values to the near and far planes
                 // instead of discarding them
  rasterizer.rasterizerDiscardEnable =
      VK_FALSE;  // Set to TRUE to discard all output from the rasterizer. No
                 // idea why you'd want to do that...
  rasterizer.polygonMode =
      VK_POLYGON_MODE_FILL;  // Fill the outputs from the vertex shader
                             // completely. Other options include line and
                             // point. Other modes require a GPU feature to be
                             // enabled

  rasterizer.lineWidth = 1.0f;  // Thickness of lines. Anything greater than 1.0
                                // requires a widelines GPU feature

  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;  // Cull backfacing
  rasterizer.frontFace =
      VK_FRONT_FACE_CLOCKWISE;  // Front facing vertices have clockwise ordering

  // Rasterizer can be configured to bias or offset or clamp depth values
  // We don't care about this for now
  // Can be used in shadow mapping
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
  rasterizer.depthBiasClamp = 0.0f;           // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f;     // Optional

  // Disable MSAA for now
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;           // Optional
  multisampling.pSampleMask = nullptr;             // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
  multisampling.alphaToOneEnable = VK_FALSE;       // Optional

  // Disable color blending
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

  // Setup global color blending parameters
  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;  // Optional
  colorBlending.blendConstants[1] = 0.0f;  // Optional
  colorBlending.blendConstants[2] = 0.0f;  // Optional
  colorBlending.blendConstants[3] = 0.0f;  // Optional

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;             // Optional
  pipelineLayoutInfo.pSetLayouts = nullptr;          // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0;     // Optional
  pipelineLayoutInfo.pPushConstantRanges = nullptr;  // Optional

  if (vkCreatePipelineLayout(deviceCpp->getVkDevice(),
                             &pipelineLayoutInfo,
                             nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;  // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;  // Optional

  pipelineInfo.layout = pipelineLayout;

  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;

  // Allows for deriving a new pipeline from an old pipeline
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
  pipelineInfo.basePipelineIndex = -1;               // Optional

  if (vkCreateGraphicsPipelines(deviceCpp->getVkDevice(),
                                VK_NULL_HANDLE,
                                1,
                                &pipelineInfo,
                                nullptr,
                                &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(deviceCpp->getVkDevice(), fragShaderModule, nullptr);
  vkDestroyShaderModule(deviceCpp->getVkDevice(), vertShaderModule, nullptr);
}

void createFramebuffers() {
  fmt::print("Creating {} framebuffers\n", swapChainImageViews.size());
  swapChainFramebuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapchain->getExtent().width;
    framebufferInfo.height = swapchain->getExtent().height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(deviceCpp->getVkDevice(),
                            &framebufferInfo,
                            nullptr,
                            &swapChainFramebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

// Command pools are memory regions from which we allocate a command buffer
// A command pool can only be associated with a single command queue family
void createCommandPool() {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = graphicsQueueRequest.family.index;
  poolInfo.flags = 0;  // Optional.
                       // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, Hint that
                       // command buffers are rerecorded with new commands very
                       // often (may change memory allocation behavior)
                       // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow
                       // command buffers to be rerecorded individually, without
                       // this flag they all have to be reset together

  if (vkCreateCommandPool(deviceCpp->getVkDevice(),
                          &poolInfo,
                          nullptr,
                          &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
}

void createCommandBuffers() {
  // Create a command buffer for each framebuffer in the swapchain
  commandBuffers.resize(swapChainFramebuffers.size());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level =
      VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can
                                        // be submitted to a queue for
                                        // execution, but cannot be called from
                                        // other command buffers.
                                        // VK_COMMAND_BUFFER_LEVEL_SECONDARY:
                                        // Cannot be submitted directly, but can
                                        // be called from primary command
                                        // buffers.

  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

  if (vkAllocateCommandBuffers(deviceCpp->getVkDevice(),
                               &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  // Start command buffer recording
  for (size_t i = 0; i < commandBuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags =
        0;  // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer
            // will be rerecorded right after executing it once.
            // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a
            // secondary command buffer that will be entirely within a single
            // render pass. VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The
            // command buffer can be resubmitted while it is also already
            // pending execution.
    beginInfo.pInheritanceInfo =
        nullptr;  // Optional, only used for "secondary" command buffers

    if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[i];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain->getExtent();

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    // Record this command to the command buffer
    vkCmdBeginRenderPass(commandBuffers[i],
                         &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    // Indicate this command should be bound to a graphics pipeline (instead of
    // a compute pipeline)
    vkCmdBindPipeline(commandBuffers[i],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);

    vkCmdDraw(commandBuffers[i],
              3,  // Num Vertices to draw
              1,  // Instance count
              0,  // First vertex index offset
              0   // First instance offset
    );

    vkCmdEndRenderPass(commandBuffers[i]);

    if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }
}

void createSyncObjects() {
  // One fence per swapchain image
  // Initially, no one is using any swapchain images, so we initialize them to a
  // NULL handle
  imagesInFlight.resize(swapchain->getImages().size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags =
      VK_FENCE_CREATE_SIGNALED_BIT;  // Create fence in the "signaled" state so
                                     // we don't stall

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(deviceCpp->getVkDevice(),
                          &semaphoreInfo,
                          nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(deviceCpp->getVkDevice(),
                          &semaphoreInfo,
                          nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(deviceCpp->getVkDevice(),
                      &fenceInfo,
                      nullptr,
                      &inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores!");
    }
  }
}

void cleanupSwapChain() {
  for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
    vkDestroyFramebuffer(deviceCpp->getVkDevice(),
                         swapChainFramebuffers[i],
                         nullptr);
  }

  vkFreeCommandBuffers(deviceCpp->getVkDevice(),
                       commandPool,
                       static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());

  vkDestroyPipeline(deviceCpp->getVkDevice(), graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(deviceCpp->getVkDevice(), pipelineLayout, nullptr);
  vkDestroyRenderPass(deviceCpp->getVkDevice(), renderPass, nullptr);

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    vkDestroyImageView(deviceCpp->getVkDevice(),
                       swapChainImageViews[i],
                       nullptr);
  }

  //vkDestroySwapchainKHR(deviceCpp->getVkDevice(), swapChain, nullptr);
  delete swapchain;
}

void recreateSwapChain() {
  // Test if we've been minimized: block 
  // until we are visible again before trying to regenerate 
  // the swapchain
  while (!windowSystem->isVisible()) {
    windowSystem->waitEvents();
  }

  vkDeviceWaitIdle(deviceCpp->getVkDevice());

  cleanupSwapChain();

  createSwapChain(*deviceCpp);
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandBuffers();
}

void drawFrame() {
  // Wait for this command buffer (and other things) to be free
  vkWaitForFences(deviceCpp->getVkDevice(),
                  1,
                  &inFlightFences[currentFrame],
                  VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;

  // Signal the imageAvailableSemaphore when imageIndex image is ready to be
  // written to Note: imageAvailableSemaphore may only exist in GPU-space
  VkResult result =
      vkAcquireNextImageKHR(deviceCpp->getVkDevice(),
                            *swapchain,
                            UINT64_MAX,
                            imageAvailableSemaphores[currentFrame],
                            VK_NULL_HANDLE,
                            &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swapchain image!");
  }

  // Check if a previous frame is using this image (i.e. there is its fence to
  // wait on)
  if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(deviceCpp->getVkDevice(),
                    1,
                    &imagesInFlight[imageIndex],
                    VK_TRUE,
                    UINT64_MAX);
  }

  // Mark the image as now being in use by this frame
  imagesInFlight[imageIndex] = inFlightFences[currentFrame];

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  // The GFX engine needs to wait until the imageAvailableSemaphore is signaled
  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

  // Wait at the COLOR_ATTACHMENT_OUTPUT stage (ie: the pixel shader). Other
  // stages (ie: the vertex shader) are allowed to run before the semaphore is
  // signaled Note: indexes in this array correspond to semaphore indexes in the
  // waitSemaphores array
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  submitInfo.waitSemaphoreCount = 1;  // ARRAY_SIZE(waitSemaphores) ?
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

  // Signal the renderFinishedSemaphore when rendering is complete for this
  // frame
  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  // Reset the fence for this frame: this un-signals the fence, so other callers
  // will have to wait
  vkResetFences(deviceCpp->getVkDevice(), 1, &inFlightFences[currentFrame]);

  // vkQueueSubmit will raise inFlightFences[currentFrame] when this set of
  // commands has finished rendering
  if (vkQueueSubmit(graphicsQueueRequest.getQueue(),
                    1,
                    &submitInfo,
                    inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {*swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  presentInfo.pResults = nullptr;  // Optional

  result = vkQueuePresentKHR(presentationQueueRequest.getQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swapchain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void initVulkan() {
  createInstance();

  createSurface();

  std::optional<PhysicalDevice> physicalDevice = pickPhysicalDevice();

  if (!physicalDevice.has_value()) {
    LOG_F("Failed to pick a physical device");
    return;
  }

  createLogicalDevice(std::move(physicalDevice.value()));

  createSwapChain(*deviceCpp);

  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPool();
  createCommandBuffers();
  createSyncObjects();
}

void mainLoop() {
  while (!windowSystem->shouldApplicationExit()) {
    windowSystem->pollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(deviceCpp->getVkDevice());
}

void cleanup() {
  cleanupSwapChain();

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(deviceCpp->getVkDevice(),
                       renderFinishedSemaphores[i],
                       nullptr);
    vkDestroySemaphore(deviceCpp->getVkDevice(),
                       imageAvailableSemaphores[i],
                       nullptr);
    vkDestroyFence(deviceCpp->getVkDevice(), inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(deviceCpp->getVkDevice(), commandPool, nullptr);

  delete deviceCpp;  // vkDestroyDevice(deviceCpp->getVkDevice(), nullptr);

  vkDestroySurfaceKHR(instanceCpp->getInstance(), surface, nullptr);

  delete instanceCpp;

  delete windowSystem;
}

int main(int argc, char **argv) {
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