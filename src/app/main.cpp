#include <fmt/core.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <array>

// Core engine stuff
#include <engine/core/CommandPool.hpp>
#include <engine/core/Device.hpp>
#include <engine/core/GraphicsPipeline.hpp>
#include <engine/core/Instance.hpp>
#include <engine/core/RenderPass.hpp>
#include <engine/core/ShaderModule.hpp>
#include <engine/core/Swapchain.hpp>
#include <engine/utils/to_string.hpp>

// Platform specific code
#include <engine/win32/GlfwWindowSystem.hpp>
#include <exception>
#include <fmtlog/Log.hpp>
#include <fstream>
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

const std::vector<std::string> deviceExtensionsCpp = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

GraphicsPipeline *graphicsPipeline;
CommandPool *commandPool;

std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences;
std::vector<VkFence> imagesInFlight;

std::vector<CommandBuffer> commandBuffers;
std::vector<std::reference_wrapper<const Framebuffer>> swapChainFramebuffers;

std::vector<ImageView> swapChainImageViews;

size_t currentFrame = 0;

bool framebufferResized = false;

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
    SwapChainSupportDetails swapChainSupport =
        device.querySwapChainSupport(surface);
    // TODO: more explicit swapchain requirements
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  return hasGraphicsFamily && hasPresentFamily &&
         device.getProperties().deviceType ==
             VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         device.getFeatures().geometryShader && extensionsSupported &&
         swapChainAdequate;
}

std::optional<PhysicalDevice> pickPhysicalDevice() {
  std::vector<PhysicalDevice> deviceList =
      PhysicalDevice::getPhysicalDevices(*instance, surface);

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

  device = new LogicalDevice(*instance,
                             std::move(physicalDevice),
                             deviceExtensionsCpp,
                             requests);

  if (graphicsQueueRequest.getQueue() == VK_NULL_HANDLE ||
      presentationQueueRequest.getQueue() == VK_NULL_HANDLE) {
    LOG_F("Failed to get valid queue");
  }
}

void createSwapChain(const LogicalDevice &device) {
  swapchain = new Swapchain(device,
                            surface,
                            {graphicsQueueRequest, presentationQueueRequest});
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
  if (vkCreateShaderModule(device->getVkDevice(),
                           &createInfo,
                           nullptr,
                           &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

void createRenderPass() {
  renderPass = new RenderPass(*device,
                              swapchain->getExtent().width,
                              swapchain->getExtent().height);

  // Our render pass is only going to need a single attachment
  const Attachment &outputColorAttachment =
      renderPass->createAttachment(swapchain->getFormat());

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
    swapChainFramebuffers.emplace_back(renderPass->createFramebuffer(
        {{outputColorAttachment, swapchainImageView}}));
  }
}

void createGraphicsPipeline() {
  VertexShaderModule vertexShader(*device, "shaders/shader.vert.spv");
  ShaderModule fragmentShader(*device, "shaders/shader.frag.spv");

  graphicsPipeline = new GraphicsPipeline(*device,
                                          *swapchain,
                                          *playerViewSubpass,
                                          vertexShader,
                                          fragmentShader);
}

// Command pools are memory regions from which we allocate a command buffer
// A command pool can only be associated with a single command queue family
void createCommandPool() {
  // Create a command pool on the graphics queue
  commandPool = new CommandPool(*device, graphicsQueueRequest);
}

void createCommandBuffers() {
  // Create a command buffer for each framebuffer in the swapchain
  // commandBuffers.resize(swapChainFramebuffers.size());

  for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
    commandBuffers.emplace_back(commandPool->allocateCommandBuffer());
  }

  /*
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = *commandPool;
    allocInfo.level =
        VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // VK_COMMAND_BUFFER_LEVEL_PRIMARY:
    Can
                                          // be submitted to a queue for
                                          // execution, but cannot be called
    from
                                          // other command buffers.
                                          // VK_COMMAND_BUFFER_LEVEL_SECONDARY:
                                          // Cannot be submitted directly, but
    can
                                          // be called from primary command
                                          // buffers.

    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device->getVkDevice(),
                                 &allocInfo,
                                 commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers!");
    }
  */
  // Start command buffer recording
  for (size_t i = 0; i < commandBuffers.size(); i++) {
    commandBuffers[i].begin();

    commandBuffers[i].beginRenderPass(*renderPass, swapChainFramebuffers[i]);

    commandBuffers[i].bindPipeline(*graphicsPipeline);

    commandBuffers[i].draw(3,  // Num Vertices to draw
                           1,  // Instance count
                           0,  // First vertex index offset
                           0   // First instance offset
    );

    commandBuffers[i].end();

    /*
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
    renderPassInfo.renderPass = *renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[i].get();
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
                      *graphicsPipeline);

    vkCmdDraw(commandBuffers[i],
              3,  // Num Vertices to draw
              1,  // Instance count
              0,  // First vertex index offset
              0   // First instance offset
    );

    vkCmdEndRenderPass(commandBuffers[i]);

    if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }*/
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
    if (vkCreateSemaphore(device->getVkDevice(),
                          &semaphoreInfo,
                          nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device->getVkDevice(),
                          &semaphoreInfo,
                          nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device->getVkDevice(),
                      &fenceInfo,
                      nullptr,
                      &inFlightFences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create semaphores!");
    }
  }
}

void cleanupSwapChain() {
  commandBuffers.clear();
  /*
  vkFreeCommandBuffers(device,
                       *commandPool,
                       static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());*/

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
  vkWaitForFences(device->getVkDevice(),
                  1,
                  &inFlightFences[currentFrame],
                  VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;

  // Signal the imageAvailableSemaphore when imageIndex image is ready to be
  // written to Note: imageAvailableSemaphore may only exist in GPU-space
  VkResult result =
      vkAcquireNextImageKHR(device->getVkDevice(),
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
    vkWaitForFences(device->getVkDevice(),
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
  vkResetFences(device->getVkDevice(), 1, &inFlightFences[currentFrame]);

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
  createSwapChain(*device);
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

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device->getVkDevice(),
                       renderFinishedSemaphores[i],
                       nullptr);
    vkDestroySemaphore(device->getVkDevice(),
                       imageAvailableSemaphores[i],
                       nullptr);
    vkDestroyFence(device->getVkDevice(), inFlightFences[i], nullptr);
  }

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