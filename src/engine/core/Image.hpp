#pragma once

#include <vulkan/vulkan.h>

#include <engine/core/Device.hpp>

/**
 * @brief Simple wrapper enabling RAII around a VkImage
 *
 */
class Image {
  friend class Swapchain;
  friend class ImageView;

 public:
  Image(Image& other) = delete;

  Image(Image&& other);

  ~Image();

  uint32_t getMipLevels() const { return mipLevels_; }

  VkFormat getFormat() const { return format_; }

  VkExtent2D getExtent() const { return extent_; }

  bool isPresentable() const { return presentable_; }

  operator VkImage() const { return image_; }

 protected:
  // Only for use when generating a VkImage from a Swapchain
  Image(const LogicalDevice& device,
        VkImage image,
        uint32_t mipLevels,
        VkFormat format,
        VkExtent2D extent,
        bool presentable = false);

  VkDevice getAssociatedDevice() const { return device_; }

 private:
  const LogicalDevice& device_;  // The device that this image is bound to
  bool presentable_ = false;     // Only true for VkImages that are derived from a Swapchain
  VkImage image_;
  uint32_t mipLevels_;
  VkFormat format_;
  VkExtent2D extent_;
};

/**
 * @brief Simple wrapper enabling RAII around a VkImageView
 *
 */
class ImageView {
 public:
  ImageView() = delete;

  ImageView(ImageView& other) = delete;

  ImageView(ImageView&& other);

  ImageView(const Image& image);

  ~ImageView();

  const Image& getImage() const { return image_; }

  operator VkImageView() const { return imageView_; }

 private:
  const Image& image_;
  VkImageView imageView_;
};