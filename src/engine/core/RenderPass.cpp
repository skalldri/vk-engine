#include "RenderPass.hpp"

#include <engine/utils/vk.hpp>
#include <fmtlog/Log.hpp>

RenderPass::RenderPass(const LogicalDevice& device,
                       uint32_t width,
                       uint32_t height)
    : device_(device),
      width_(width),
      height_(height) {}

RenderPass::~RenderPass() {
  framebuffers_.clear();
  attachments_.clear();
  subpasses_.clear();

  vkDestroyRenderPass(device_, renderPass_, nullptr);
}

const Attachment& RenderPass::createAttachment(VkFormat format) {
  if (finalized_) {
    LOG_F("cannot modify a render pass after it is finalized");
  }

  attachments_.push_back(
      Attachment(*this, {width_, height_}, format, attachments_.size()));
  return attachments_.back();
}

Subpass& RenderPass::createSubpass(
    std::vector<std::reference_wrapper<const Attachment>> colorAttachments) {
  if (finalized_) {
    LOG_F("cannot modify a render pass after it is finalized");
  }

  // TODO: validate that all attachments are owned by this render pass

  subpasses_.push_back(Subpass(*this, subpasses_.size(), colorAttachments));
  return subpasses_.back();
}

void RenderPass::finalize() {
  if (finalized_) {
    LOG_F("cannot finalize a render pass a second time");
  }

  std::vector<VkAttachmentDescription> vkAttachmentDescriptions;
  vkAttachmentDescriptions.resize(attachments_.size());

  size_t i = 0;
  for (const auto& attachment : attachments_) {
    // Sanity check: i must equal the attachmentIndex_ in the attachment
    if (i != attachment.getIndex()) {
      LOG_F("Assert failed: i != attachment index");
    }

    vkAttachmentDescriptions[i].format = attachment.getFormat();

    // TODO: make these non-generic
    vkAttachmentDescriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
    vkAttachmentDescriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    vkAttachmentDescriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    vkAttachmentDescriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vkAttachmentDescriptions[i].stencilStoreOp =
        VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vkAttachmentDescriptions[i].initialLayout =
        VK_IMAGE_LAYOUT_UNDEFINED;  // Initial layout of our VkImage is going to
                                    // be undefined, since this is the default
                                    // state of all VkImages that come from the
                                    // Swapchain.
    vkAttachmentDescriptions[i].finalLayout =
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Final layout of the color
                                          // attachment is one that can be
                                          // presented to the screen
    i++;
  }

  std::vector<VkSubpassDependency> vkSubpassDependencies;
  std::vector<VkSubpassDescription> vkSubpasses;
  vkSubpasses.resize(subpasses_.size());
  i = 0;

  for (const auto& subpass : subpasses_) {
    // Sanity check: i must equal the attachmentIndex_ in the attachment
    if (i != subpass.getIndex()) {
      LOG_F("Assert failed: i != subpass index");
    }

    vkSubpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    vkSubpasses[i].colorAttachmentCount =
        subpass.getColorAttachmentReferences().size();
    vkSubpasses[i].pColorAttachments =
        subpass.getColorAttachmentReferences().data();

    // TODO: support other types of attachments

    // Depth / stencil attachments are always write-only, output attachments,
    // from the ????? shader pipeline stage (probably between the vertex and
    // fragment shaders) subpass.pDepthStencilAttachment = ???;

    // Input attachments are what they say on the box, input attachments, and
    // can be either depth or color subpass.inputAttachmentCount = ????;
    // subpass.pInputAttachments = ???; // What attachments are read by this
    // subpass

    // Handle all dependencies for this Subpass
    for (const auto& dep : subpass.getDependencies()) {
      vkSubpassDependencies.push_back(dep);
    }

    i++;
  }

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

  renderPassInfo.attachmentCount = vkAttachmentDescriptions.size();
  renderPassInfo.pAttachments = vkAttachmentDescriptions.data();

  renderPassInfo.subpassCount = vkSubpasses.size();
  renderPassInfo.pSubpasses = vkSubpasses.data();

  renderPassInfo.dependencyCount = vkSubpassDependencies.size();
  renderPassInfo.pDependencies = vkSubpassDependencies.data();

  if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) !=
      VK_SUCCESS) {
    LOG_F("failed to create render pass!");
  }

  finalized_ = true;
}

size_t RenderPass::getAttachmentCount() const {
  if (!finalized_) {
    LOG_F("Cannot get attachment count before finalization");
  }

  return attachments_.size();
}

const Framebuffer& RenderPass::createFramebuffer(
    std::vector<FramebufferBinding> attachmentBindings) {
  if (!finalized_) {
    LOG_F("Cannot create framebuffer before finalization");
  }

  framebuffers_.emplace_back(*this, attachmentBindings);
  return framebuffers_.back();
}

Subpass::Subpass(
    const RenderPass& parent,
    uint32_t index,
    std::vector<std::reference_wrapper<const Attachment>> colorAttachments)
    : colorAttachments_(colorAttachments),
      subpassIndex_(index),
      parent_(parent) {
  // TODO: support other kinds of attachments
  colorAttachmentReferences_.resize(colorAttachments_.size());
  for (size_t i = 0; i < colorAttachments_.size(); i++) {
    colorAttachmentReferences_[i].attachment =
        colorAttachments_[i].get().getIndex();
    colorAttachmentReferences_[i].layout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }
}

void Subpass::addStartExternalDependency() {
  VkSubpassDependency dependency{};

  // VK_SUBPASS_EXTERNAL refers to the implicit subpass that occurrs before our
  // current renderPass. Effectively, this refers to the previous frame.
  // To be technical, using VK_SUBPASS_EXTERNAL requires that all commands
  // queued before the call to vkCmdBeginRenderPass() are complete.
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;

  // dstSubpass == our eventual index in the pSubpasses array
  // We were told our eventual index in that array when we were
  // constructed
  dependency.dstSubpass = subpassIndex_;

  // The "source" of our dependency is, effectively, the rendering pipeline step
  // where the color attachment gets written.
  // We don't want to start rendering the next frame until the color attachment
  // of the previous frame (VK_SUBPASS_EXTERNAL) has been written.
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;

  // Destination of our dependency is the color attachment for the current
  // frame. We are not allowed to write to the color attachment until it's
  // complete. We are allowed to read it, for example.
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  dependencies_.push_back(dependency);
}

Attachment::Attachment(const RenderPass& parent,
                       VkExtent2D extent,
                       VkFormat format,
                       uint32_t index)
    : parent_(parent),
      extent_(extent),
      format_(format),
      attachmentIndex_(index) {}

Framebuffer::Framebuffer(const RenderPass& parent,
                         std::vector<FramebufferBinding> attachmentBindings)
    : parent_(parent) {
  // First, ensure that the parent render pass has been finalized
  if (!parent_.isFinalized()) {
    LOG_F("Render pass must be finalized before adding framebuffers");
  }

  // Use pointers here because the STL doesn't handle
  // references as part of std::set very well. Since we're just
  // using this to help de-duplicate the list, this is fine
  std::set<const Attachment*> providedAttachments;
  std::set<const ImageView*> providedImageViews;

  // We can save a few compute cycles by assuming that the input is correct,
  // and starting to construct the final `attachments` list up front.
  // We will crash the program if we determine that this assumption was wrong,
  // so there's no harm in starting early.
  std::vector<VkImageView> attachments;
  attachments.resize(attachmentBindings.size());

  for (const FramebufferBinding& binding : attachmentBindings) {
    // Ensure this attachment belongs to our parent RenderPass
    if (!binding.first.isParent(parent_)) {
      LOG_F("Attachment is not from this RenderPass!");
    }

    // Make sure we've not seen this attachment before
    if (providedAttachments.count(&binding.first) != 0) {
      LOG_F("Got a duplicate attachment in input!");
    }

    // Make sure we've not seen this ImageView before
    if (providedImageViews.count(&binding.second) != 0) {
      LOG_F("Got a duplicate ImageView in input!");
    }

    // Make sure the ImageView for this Attachment has the same dimensions
    if (binding.first.getExtent() != binding.second.getImage().getExtent()) {
      LOG_F("ImageView and Attachment have mismatched extent");
    }

    // Make sure the ImageView for this Attachment has the same format
    if (binding.first.getFormat() != binding.second.getImage().getFormat()) {
      LOG_F("ImageView and Attachment have mismatched format");
    }

    // Track that we've seen this attachment already
    providedAttachments.insert(&binding.first);

    // Track that we've seen this image view already
    providedImageViews.insert(&binding.second);

    // Insert this ImageView into the `attachments` list at the correct location
    // based on the binding to the attachment
    if (binding.first.getIndex() >= attachments.size()) {
      LOG_F("Attachment index is out of range");
    }

    attachments[binding.first.getIndex()] = binding.second;
  }

  // We've verified that all the provided attachments belong to our parent
  // render pass, and we've verified there are no duplicates. Therefore, if
  // numProvidedAttachments == numRenderPassAttachments, then we have a complete
  // list.
  if (providedAttachments.size() != parent_.getAttachmentCount()) {
    LOG_F("Incorrect number of attachments for this render pass");
  }

  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = parent_;
  framebufferInfo.attachmentCount = attachments.size();
  framebufferInfo.pAttachments = attachments.data();
  framebufferInfo.width = parent_.getExtent().width;
  framebufferInfo.height = parent_.getExtent().height;
  framebufferInfo.layers = 1;  // TODO: ???

  if (vkCreateFramebuffer(parent_.getDevice(),
                          &framebufferInfo,
                          nullptr,
                          &framebuffer_) != VK_SUCCESS) {
    LOG_F("failed to create framebuffer!");
  }
}

Framebuffer::~Framebuffer() {
  if (framebuffer_) {
    vkDestroyFramebuffer(parent_.getDevice(), framebuffer_, nullptr);
    framebuffer_ = nullptr;
  }
}

Framebuffer::Framebuffer(Framebuffer&& other)
    : parent_(other.parent_),
      framebuffer_(other.framebuffer_) {
  other.framebuffer_ = nullptr;
}
