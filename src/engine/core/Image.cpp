#include "Image.hpp"

#include <fmtlog/Log.hpp>

Image::Image(Image&& other) : device_(other.device_) {
  presentable_ = other.presentable_;
  image_ = other.image_;
  mipLevels_ = other.mipLevels_;
  format_ = other.format_;
  extent_ = other.extent_;

  // Invalidate other
  other.image_ = nullptr;
  other.extent_ = {0, 0};
  other.format_ = VK_FORMAT_UNDEFINED;
}

Image::Image(const LogicalDevice& device,
             VkImage image,
             uint32_t mipLevels,
             VkFormat format,
             VkExtent2D extent,
             bool presentable)
    : device_(device),
      image_(image),
      mipLevels_(mipLevels),
      extent_(extent),
      format_(format),
      presentable_(presentable) {}

Image::~Image() {}

ImageView::ImageView(ImageView&& other) : image_(other.image_), imageView_(other.imageView_) {
  other.imageView_ = nullptr;
}

ImageView::ImageView(const Image& image) : image_(image) {
  VkImageViewCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = image_;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // Treat the image as a 2D array of pixels
  createInfo.format = image_.getFormat();       // Match the format of the underlying image

  // Allows us to swizzle one color channel into another, but for now
  // we just stick with the default
  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  // ??? not sure what this does ???
  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

  // Tell Vulkan there is only a single MIP level, MIP0, because
  // the final rendered image that we are going to display to the user
  // only has a single image
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = image_.getMipLevels();

  // Since this is currently a 2D application, we only need one layer per
  // VkImage but for a VR application, we might request a VkImage with
  // multiple layers, and then bind each layer of a single VkImage into
  // multiple VkImageViews
  //
  // TODO: VR support?
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(image_.getAssociatedDevice(), &createInfo, nullptr, &imageView_) !=
      VK_SUCCESS) {
    LOG_F("failed to create image view!");
  }
}

ImageView::~ImageView() { vkDestroyImageView(image_.getAssociatedDevice(), imageView_, nullptr); }