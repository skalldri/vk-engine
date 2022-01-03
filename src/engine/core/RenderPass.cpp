#include "RenderPass.hpp"

#include <fmtlog/Log.hpp>

RenderPass::RenderPass(const LogicalDevice& device,
                       uint32_t width,
                       uint32_t height)
    : device_(device),
      width_(width),
      height_(height) {}

const Attachment& RenderPass::createAttachment(const ImageView& imageView) {
  attachments_.push_back(Attachment(imageView, attachments_.size()));
  return attachments_.back();
}

const Subpass& RenderPass::createSubpass(
    std::vector<std::reference_wrapper<const Attachment>> colorAttachments) {
  // TODO: validate that all attachments are owned by this render pass

  subpasses_.push_back(Subpass(*this, subpasses_.size(), colorAttachments));
  return subpasses_.back();
}

void RenderPass::finalize() {
  std::vector<VkAttachmentDescription> vkAttachmentDescriptions;
  vkAttachmentDescriptions.resize(attachments_.size());

  for (size_t i = 0; i < attachments_.size(); i++) {
    // Sanity check: i must equal the attachmentIndex_ in the attachment
    if (i != attachments_[i].getIndex()) {
      LOG_F("Assert failed: i != attachment index");
    }

    vkAttachmentDescriptions[i].format = attachments_[i].getFormat();

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
  }

  std::vector<VkSubpassDependency> vkSubpassDependencies;
  std::vector<VkSubpassDescription> vkSubpasses;
  vkSubpasses.resize(subpasses_.size());
  for (size_t i = 0; i < subpasses_.size(); i++) {

    vkSubpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    vkSubpasses[i].colorAttachmentCount = subpasses_[i].getColorAttachmentReferences().size();
    vkSubpasses[i].pColorAttachments = subpasses_[i].getColorAttachmentReferences().data();

    // TODO: support other types of attachments

    // Depth / stencil attachments are always write-only, output attachments, from the ????? shader pipeline stage (probably between the vertex and fragment shaders)
    // subpass.pDepthStencilAttachment = ???;

    // Input attachments are what they say on the box, input attachments, and can be either depth or color
    // subpass.inputAttachmentCount = ????;
    // subpass.pInputAttachments = ???; // What attachments are read by this subpass

    // Handle all dependencies for this Subpass
    for (const auto& dep : subpasses_[i].getDependencies()) {
      vkSubpassDependencies.push_back(dep);
    }
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
}

Subpass::Subpass(
    const RenderPass& parent,
    uint32_t index,
    std::vector<std::reference_wrapper<const Attachment>> colorAttachments)
    : colorAttachments_(colorAttachments),
      subpassIndex_(index),
      renderPass_(parent) {
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
  // We don't want to start rendering the next frame until the color attachment of the 
  // previous frame (VK_SUBPASS_EXTERNAL) has been written.
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;

  // Destination of our dependency is the color attachment for the current
  // frame. We are not allowed to write to the color attachment until it's
  // complete. We are allowed to read it, for example.
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  dependencies_.push_back(dependency);
}

Attachment::Attachment(const ImageView& imageView, uint32_t index)
    : imageView_(imageView),
      attachmentIndex_(index) {}