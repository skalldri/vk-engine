#pragma once

#include <engine/core/WindowSystem.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

class GlfwWindowSystem : public IWindowSystem {
 public:
  ~GlfwWindowSystem();
  
  GlfwWindowSystem();

  GlfwWindowSystem(GlfwWindowSystem&& other);

  GlfwWindowSystem(GlfwWindowSystem& other) = delete;

  virtual VkExtent2D getDesiredFramebufferSize();

  virtual bool isVisible();

  virtual bool shouldApplicationExit();

  virtual void pollEvents();

  virtual void waitEvents();

  virtual Extensions getRequiredVkInstanceExtensions();

  virtual VkSurfaceKHR createSurface(const Instance& instance);

 private:

  /**
   * @brief Trampoline that extracts the user pointer from window, 
   * casts it to a GlfwWindowSystem*, and then calls framebufferResized()
   * 
   * @param window glfw window that the user pointer is retreived from
   * @param width window width, passed to framebufferResized()
   * @param height window height, passed to framebufferResized()
   */
  static void framebufferResizeCallback(GLFWwindow* window,
                                        int width,
                                        int height);

  void framebufferResized(int width, int height);

 private:
  GLFWwindow* window_;
  bool framebufferResized_ = false;

  // TODO: make this runtime parameters
  // TODO: allow window resizing? via options menu
  size_t window_width = 800;
  size_t window_height = 600;
};