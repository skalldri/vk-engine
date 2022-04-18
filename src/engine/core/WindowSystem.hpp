#pragma once

#include <vulkan/vulkan.h>

#include <engine/core/Instance.hpp>

class IWindowSystem {
 public:
  /**
   * @brief Queries the window system to see if the application should exit.
   * The window system might indicate this to the application if, for example,
   * the user has closed the window, or if the application is being terminated
   * by the OS for any reason. On some platforms, it may not be possible for the
   * window system to indicate this to the application: if this is the case, the function
   * should always return false.
   *
   * @return true
   * @return false
   */
  virtual bool shouldApplicationExit() = 0;

  /**
   * @brief Get the Desired Framebuffer Size for the current platform.
   * This value can change at runtime. For example, on Win32, the window can be
   * resized. On Oculus, the render target resolution could change due to poor
   * thermal performance.
   * Therefore, this function can be called at any time to get the desired resolution
   * of the framebuffer / swapchain.
   *
   * @return VkExtent2D the width / height of the optimal framebuffer
   */
  virtual VkExtent2D getDesiredFramebufferSize() = 0;

  /**
   * @brief Check if the application is active and visible
   * to the user. On Windows, this might indicate that the application has focus.
   * On Android, it can indicate that the application is in the "Stopped" lifecycle state.
   *
   * @return true the application is active and visible in the window system
   * @return false the application is hidden by the window system, and rendering can stop
   */
  virtual bool isVisible() = 0;

  /**
   * @brief Poll for events from the window system, and execute any callbacks or operations
   * needed in response to the events
   */
  virtual void pollEvents() = 0;

  virtual void waitEvents() = 0;

  /**
   * @brief Get the list of required extensions for the VkInstance
   *
   * @return Extensions a list of required extensions for this Window System
   */
  virtual Extensions getRequiredVkInstanceExtensions() = 0;

  // TODO: maybe a "block until foreground" function that puts the app to sleep
  // until we are visible again?

  // TODO: maybe a "app has focus" function that tells us if we're the main focus app?
  // On Win32, this would be true if the app is currently the focus
  // On Oculus, this would be true if the system UI isn't open

  virtual VkSurfaceKHR createSurface(const Instance& instance) = 0;
};