#pragma once

#include <engine/core/Device.hpp>
#include <engine/core/Image.hpp>

#include <vulkan/vulkan.h>

class Subpass;
class Attachment;

/**
 * @brief The RenderPass class. This is where a lot of the magic happens.
 * A RenderPass describes a sequence of SubPasses, the attachments that those SubPasses will use,
 * as well as the dependencies between SubPasses.
 * 
 * A SubPass is bound to a single graphics pipeline, which in itself is composed of a sequence of shaders.
 * A SubPass can be thought of as a single sequence of shaders executing on input data. For a very simple
 * rendering engine, we might only have a single SubPass that is executed and draws the entire scene in a single pass.
 * 
 * A more complex engine might have an initial SubPass that generates shadowmaps and writes them into an attachment, 
 * followed by a second subpass that renders the scene using the shadowmaps as well as the static / dynamic geometry. 
 * This is where SubPass dependencies come in: in this example, the Shadowmap SubPass outputs are needed by the Rendering SubPass,
 * which we might describe as:
 * - The ShadowMap subpass (the first subpass in the RenderPass) depends on the VK_SUBPASS_EXTERNAL subpass, 
 *   to indicate that it needs to start after the previous set of frame commands has finished.
 * - The ShadowMap subpass outputs into a depth attachment
 * - The Rendering subpass would depend on the previous subpass
 * - The Rendering subpass would depend on the previous src stage pipeline stage VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT (which encompasses the depth attachment output)
 * - The Rendering subpass dst stage would be 
 * 
 * RenderPasses have a fixed resolution. All SubPasses within a RenderPass must have the same resolution.
 */
class RenderPass {
 public:
  RenderPass(RenderPass& other) = delete;
  RenderPass() = delete;

  RenderPass(const LogicalDevice& device, uint32_t width, uint32_t height);

  /**
   * @brief Create a Attachment that is compatible / bound to the specified ImageView
   * 
   * @param imageView 
   * @return const Attachment& 
   */
  const Attachment& createAttachment(const ImageView& imageView);

  /**
   * @brief Create a Subpass within this Render pass. Ordering happens later
   * 
   * @param colorAttachments the number of color attachments this subpass will produce
   * @return const SubPass& 
   */
  // TODO: bind a graphics pipeline to the subpass at this step?
  const Subpass& createSubpass(std::vector<std::reference_wrapper<const Attachment>> colorAttachments);

  /**
   * @brief Indicate that we have finished creating the render pass, so
   * runtime checks on all subpasses, dependency resolution, and Vulkan object creation can occur
   */
  void finalize();

 private:
  const LogicalDevice& device_;
  std::vector<Subpass> subpasses_;
  std::vector<Attachment> attachments_;
  uint32_t width_;
  uint32_t height_;
  bool finalized_ = false;
  VkRenderPass renderPass_;
};

/**
 * @brief 
 * 
 */
class Subpass {
 public:
  Subpass() = delete;

  /**
   * @brief Construct a new Subpass object
   * Things it needs:
   * - width and height of any output attachments
   * - shader modules to bind to the GFX pipeline
   * - attachments that subpass needs as inputs, and that it outputs
   * - a reference to the parent renderpass for certain VK operations
   */
  Subpass(const RenderPass& parent, uint32_t index, std::vector<std::reference_wrapper<const Attachment>> colorAttachments);

  const std::vector<VkAttachmentReference>& getColorAttachmentReferences() const { return colorAttachmentReferences_; }

  uint32_t getIndex() const { return subpassIndex_; }

  const std::vector<VkSubpassDependency>& getDependencies() { return dependencies_; }

  void addStartExternalDependency();

  void addEndExternalDependency();

  void dependOn(const Subpass& other);

 private:
  const RenderPass& renderPass_;
  uint32_t subpassIndex_;
  std::vector<std::reference_wrapper<const Attachment>> colorAttachments_;
  std::vector<VkAttachmentReference> colorAttachmentReferences_;
  std::vector<VkSubpassDependency> dependencies_;
};

/**
 * Vulkan seems to have a lot of duplicate data structures.
 * 
 * When creating a RenderPass, we fill it with SubPasses. Each subpass consumes a number of attachments, 
 * which are the buffers that pass between SubPasses within a render pass, or are buffers supplied 
 * to the Shader, or buffers emitted by the Shader to the display system.
 * 
 * Attachments are later described as part of a VkFramebuffer object. There needs to be a 1:1 mapping
 * between Attachments provided to the subpass and the VkFramebuffer that gets bound to the pipeline.
 * 
 * It seems like the idea is that we can generate multiple compatible VkFramebuffer objects and round-robbin
 * swap them as we cycle through frames. This allows us to have multiple frames in flight. 
 * 
 * At a high level, in Vulkan we:
 * - Start recording a command buffer
 * - Bind a framebuffer to the command buffer
 * - Bind vertex buffer to the command buffer
 * - Bind image samplers to the command buffer
 * - Submit to the GPU
 */

/**
 * @brief Represents an attachment
 * 
 */
class Attachment {
 public:
  Attachment(const ImageView& imageView, uint32_t index);

  VkFormat getFormat() const { return imageView_.getImage().getFormat(); }

  VkExtent2D getExtent() const { return imageView_.getImage().getExtent(); }

  uint32_t getIndex() const { return attachmentIndex_; } 

 private:
  const ImageView& imageView_; // The image view associated with this attachment
  uint32_t attachmentIndex_; // The index of the attachment in the RenderPass attachment list
};

