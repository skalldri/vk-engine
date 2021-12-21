#include "GlfwWindowSystem.hpp"

#include <fmtlog/Log.hpp>

void GlfwWindowSystem::framebufferResizeCallback(GLFWwindow *window,
                                      int width,
                                      int height) {
  GlfwWindowSystem* instance = (GlfwWindowSystem*)glfwGetWindowUserPointer(window);
  instance->framebufferResized(width, height);
}

void GlfwWindowSystem::framebufferResized(int width, int height) {
  framebufferResized_ = true;
  LOG_I("Framebuffer resized. New size: {}x{}", width, height);
}

GlfwWindowSystem::GlfwWindowSystem() {
  // Initialize GLFW
  if (!glfwInit()) {
    LOG_F("Failed to init GLFW");
  }

  LOG_D("GLFW initialized");

  if (glfwVulkanSupported()) {
    // Vulkan is available, at least for compute
    LOG_D("GLFW detected vulkan support");
  } else {
    LOG_F("GLFW did not detect vulkan support");
  }

  // Tell GLFW we don't want it to assume a client API (since GLFW defaults to
  // OpenGL support, and we want Vulkan)
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Tell GLFW not to allow window resizing. Might revisit this later, but this
  // is fine for now
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(window_width,
                             window_height,
                             "VK Engine",
                             nullptr, // monitor, used for fullscreen mode. TODO: support fullscreen mode
                             nullptr);

  glfwSetWindowUserPointer(window_, this);

  glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
}

GlfwWindowSystem::GlfwWindowSystem(GlfwWindowSystem&& other) {
  window_ = other.window_;
  other.window_ = NULL;
}

VkExtent2D GlfwWindowSystem::getDesiredFramebufferSize() {
  int width, height;
  glfwGetFramebufferSize(window_, &width, &height);

  VkExtent2D extent = {width, height};

  return extent;
}

bool GlfwWindowSystem::isVisible() {
  // Test if we've been minimized
  int width = 0, height = 0;
  glfwGetFramebufferSize(window_, &width, &height);

  return (width != 0) && (height != 0);
}

bool GlfwWindowSystem::shouldApplicationExit() {
  return glfwWindowShouldClose(window_);
}

void GlfwWindowSystem::pollEvents() {
  glfwPollEvents();
}

void GlfwWindowSystem::waitEvents() {
  glfwWaitEvents();
}

Extensions GlfwWindowSystem::getRequiredVkInstanceExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;

  // Query GLFW to figure out what extensions it needs from Vulkan
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  Extensions requiredExtensions(glfwExtensions,
                                glfwExtensions + glfwExtensionCount);

  for (auto extension : requiredExtensions) {
    LOG_D("GLFW Requires Extension: {}", extension);
  }

  return requiredExtensions;
}

VkSurfaceKHR GlfwWindowSystem::createSurface(const Instance& instance) {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(instance.getInstance(),
                              window_,
                              nullptr,
                              &surface) != VK_SUCCESS) {
    LOG_F("failed to create window surface");
  }

  return surface;
}

GlfwWindowSystem::~GlfwWindowSystem() {
  glfwDestroyWindow(window_);
  window_ = NULL;

  glfwTerminate();
}