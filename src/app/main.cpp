#include <fmt/core.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <array>

// Core engine stuff
#include <engine/core/Buffer.hpp>
#include <engine/core/CommandPool.hpp>
#include <engine/core/Device.hpp>
#include <engine/core/GraphicsPipeline.hpp>
#include <engine/core/Instance.hpp>
#include <engine/core/RenderPass.hpp>
#include <engine/core/ShaderModule.hpp>
#include <engine/core/Swapchain.hpp>
#include <engine/core/Sync.hpp>
#include <engine/utils/to_string.hpp>

// Platform specific code
#include <engine/win32/GlfwWindowSystem.hpp>
#include <exception>
#include <fmtlog/Log.hpp>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <optional>
#include <string>

using namespace std;

GlfwWindowSystem *windowSystem;

Instance *instance;
Swapchain *swapchain;
RenderPass *renderPass;
Subpass *playerViewSubpass;

VkDebugUtilsMessengerEXT debugMessenger;

const Layers validationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<std::string> deviceExtensionsCpp = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef DEBUG_BUILD
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

// TODO: should this match the number of images in the Swapchain?
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

LogicalDevice *device;

QueueFamilyRequest graphicsQueueRequest;
QueueFamilyRequest presentationQueueRequest;

VkSurfaceKHR surface;

GraphicsPipeline<Vertex> *graphicsPipeline;
CommandPool *commandPool;

std::vector<Semaphore> imageAvailableSemaphores;
std::vector<Semaphore> renderFinishedSemaphores;

std::vector<Fence> inFlightFences;

std::vector<CommandBuffer> commandBuffers;
std::vector<std::reference_wrapper<const Framebuffer>> swapChainFramebuffers;

std::vector<ImageView> swapChainImageViews;

/**
 * The system can support up to MAX_FRAMES_IN_FLIGHT simultaneous frames
 * being processed by the rendering system. Each frame has associated
 * framebuffers for containing finished rendered content. Frames that have been
 * rendered need to be presented sequentially, and we can't overwrite data
 * that's already been produced: we can only wait for it to be consumed.
 *
 * We treat the sequence of frames
 *
 * This variable tracks
 *
 */
size_t currentFrame = 0;

bool framebufferResized = false;

Buffer<Vertex> *buffer;
const std::vector<Vertex> vertices = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

void initWindow() { windowSystem = new GlfwWindowSystem(); }

void createInstance() {
  instance = new Instance("Hello Triangle",
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
    SwapChainSupportDetails swapChainSupport = device.querySwapChainSupport(surface);
    // TODO: more explicit swapchain requirements
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  return hasGraphicsFamily && hasPresentFamily &&
         device.getProperties().deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         device.getFeatures().geometryShader && extensionsSupported && swapChainAdequate;
}

std::optional<PhysicalDevice> pickPhysicalDevice() {
  std::vector<PhysicalDevice> deviceList = PhysicalDevice::getPhysicalDevices(*instance, surface);

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
  std::vector<QueueFamily> availableQueueFamilies = physicalDevice.getQueueFamilies();

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

  if (graphicsQueueRequest.priority < 0.0f || presentationQueueRequest.priority < 0.0f) {
    LOG_F("Failed to pick a graphics and presentation queue");
  }

  QueueFamilyRequests requests = {graphicsQueueRequest, presentationQueueRequest};

  device = new LogicalDevice(*instance, std::move(physicalDevice), deviceExtensionsCpp, requests);

  if (graphicsQueueRequest.getQueue() == VK_NULL_HANDLE ||
      presentationQueueRequest.getQueue() == VK_NULL_HANDLE) {
    LOG_F("Failed to get valid queue");
  }
}

void createSwapChain(const LogicalDevice &device) {
  swapchain = new Swapchain(device, surface, {graphicsQueueRequest, presentationQueueRequest});
}

void createSurface() { surface = windowSystem->createSurface(*instance); }

void createImageViews() {
  for (auto &image : swapchain->getImages()) {
    swapChainImageViews.emplace_back(image);
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
  if (vkCreateShaderModule(device->getVkDevice(), &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

void createRenderPass() {
  renderPass = new RenderPass(*device, swapchain->getExtent().width, swapchain->getExtent().height);

  // Our render pass is only going to need a single attachment
  const Attachment &outputColorAttachment = renderPass->createAttachment(swapchain->getFormat());

  // Create a subpass within the render pass for rendering the player view
  // Currently this is the only subpass in the render pass
  playerViewSubpass = &renderPass->createSubpass({outputColorAttachment});

  // Indicate that this subpass depends on the completion of the previous
  // frame's commmand buffer
  playerViewSubpass->addStartExternalDependency();

  // Indicate that we're finished building the render pass
  renderPass->finalize();

  // Create framebuffers for this renderpass using each of the image views we
  // acquired from the swapchain
  LOG_D("Creating {} framebuffers", swapChainImageViews.size());
  for (const auto &swapchainImageView : swapChainImageViews) {
    swapChainFramebuffers.emplace_back(
        renderPass->createFramebuffer({{outputColorAttachment, swapchainImageView}}));
  }
}

void createGraphicsPipeline() {
  VertexShaderModule<Vertex> vertexShader(*device, "shaders/shader.vert.spv");
  ShaderModule fragmentShader(*device, "shaders/shader.frag.spv");

  graphicsPipeline =
      new GraphicsPipeline(*device, *swapchain, *playerViewSubpass, vertexShader, fragmentShader);
}

// Command pools are memory regions from which we allocate a command buffer
// A command pool can only be associated with a single command queue family
void createCommandPool() {
  // Create a command pool on the graphics queue
  commandPool = new CommandPool(*device, graphicsQueueRequest);
}

void createCommandBuffers() {
  // Create a command buffer for each framebuffer in the swapchain
  for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
    commandBuffers.emplace_back(commandPool->allocateCommandBuffer());
  }

  // Start command buffer recording
  for (size_t i = 0; i < commandBuffers.size(); i++) {
    commandBuffers[i].begin();

    commandBuffers[i].beginRenderPass(*renderPass, swapChainFramebuffers[i]);

    commandBuffers[i].bindPipeline(*graphicsPipeline);

    commandBuffers[i].bindVertexBuffers(*buffer);

    commandBuffers[i].draw(buffer->getNumElements(),  // Num Vertices to draw
                           1,                         // Instance count
                           0,                         // First vertex index offset
                           0                          // First instance offset
    );

    commandBuffers[i].endRenderPass();

    commandBuffers[i].end();
  }
}

void createSyncObjects() {
  // One fence per swapchain image
  // Initially, no one is using any swapchain images, so we initialize them to a
  // NULL handle
  // imagesInFlight.resize(swapchain->getImages().size(),
  // std::reference_wrapper::);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    inFlightFences.emplace_back(*device, true /* initially signaled*/);
    imageAvailableSemaphores.emplace_back(*device);
    renderFinishedSemaphores.emplace_back(*device);
  }
}

void cleanupSwapChain() {
  commandBuffers.clear();

  delete graphicsPipeline;

  // These references are now invalid since we're about to destroy the
  // underlying framebuffers
  swapChainFramebuffers.clear();

  // Destroy the render pass, all associated framebuffers and attachments
  delete renderPass;

  // Delete all the swapchain image views
  swapChainImageViews.clear();

  delete swapchain;
}

void recreateSwapChain() {
  // Test if we've been minimized: block
  // until we are visible again before trying to regenerate
  // the swapchain
  while (!windowSystem->isVisible()) {
    windowSystem->waitEvents();
  }

  vkDeviceWaitIdle(device->getVkDevice());

  cleanupSwapChain();

  createSwapChain(*device);
  createImageViews();

  createRenderPass();
  createGraphicsPipeline();

  createCommandBuffers();
}

void drawFrame() {
  // Wait for this command buffer (and other things) to be free
  inFlightFences[currentFrame].wait();  // Wait forever for the current frame to become free

  uint32_t imageIndex;

  // Signals the imageAvailableSemaphore when imageIndex image is ready to be
  // written to
  // Note: imageAvailableSemaphore may only exist in GPU-space
  VkResult result = vkAcquireNextImageKHR(device->getVkDevice(),
                                          *swapchain,
                                          UINT64_MAX,
                                          imageAvailableSemaphores[currentFrame],
                                          VK_NULL_HANDLE,
                                          &imageIndex);

  // Our swapchain doesn't match the presentation surface anymore:
  // recreate the swapchain and skip this frame
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swapchain image!");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  // The GFX engine needs to wait until the imageAvailableSemaphore is signaled
  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

  // Wait at the COLOR_ATTACHMENT_OUTPUT stage (ie: the pixel shader). Other
  // stages (ie: the vertex shader) are allowed to run before the semaphore is
  // signaled Note: indexes in this array correspond to semaphore indexes in the
  // waitSemaphores array
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  submitInfo.waitSemaphoreCount = 1;  // ARRAY_SIZE(waitSemaphores) ?
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  VkCommandBuffer local = commandBuffers[imageIndex];  // ?????????
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &local;

  // Signal the renderFinishedSemaphore when rendering is complete for this
  // frame
  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  // Reset the fence for this frame: this un-signals the fence, so other callers
  // will have to wait
  inFlightFences[currentFrame].reset();

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

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swapchain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void createVertexBuffer() { buffer = new Buffer<Vertex>(*device, vertices); }

void initVulkan() {
  createInstance();

  createSurface();

  std::optional<PhysicalDevice> physicalDevice = pickPhysicalDevice();

  if (!physicalDevice.has_value()) {
    LOG_F("Failed to pick a physical device");
    return;
  }

  createLogicalDevice(std::move(physicalDevice.value()));

  createSwapChain(*device);

  createVertexBuffer();

  createImageViews();

  createRenderPass();

  createGraphicsPipeline();

  createCommandPool();
  createCommandBuffers();

  createSyncObjects();
}

void mainLoop() {
  while (!windowSystem->shouldApplicationExit()) {
    windowSystem->pollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(device->getVkDevice());
}

void cleanup() {
  cleanupSwapChain();

  delete buffer;

  renderFinishedSemaphores.clear();

  imageAvailableSemaphores.clear();

  inFlightFences.clear();

  delete commandPool;

  delete device;

  vkDestroySurfaceKHR(instance->getInstance(), surface, nullptr);

  delete instance;

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