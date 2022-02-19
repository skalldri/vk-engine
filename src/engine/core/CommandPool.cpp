#include "CommandPool.hpp"

#include <fmtlog/Log.hpp>

CommandPool::CommandPool(const LogicalDevice& device,
                         const QueueFamilyRequest& queue)
    : device_(device),
      queue_(queue) {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queue_.family.index;
  poolInfo.flags = 0;  // Optional.
                       // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, Hint that
                       // command buffers are rerecorded with new commands very
                       // often (may change memory allocation behavior)
                       // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow
                       // command buffers to be rerecorded individually, without
                       // this flag they all have to be reset together

  if (vkCreateCommandPool(device.getVkDevice(),
                          &poolInfo,
                          nullptr,
                          &commandPool_) != VK_SUCCESS) {
    LOG_F("failed to create command pool!");
  }
}

CommandPool::~CommandPool() {
  vkDestroyCommandPool(device_.getVkDevice(), commandPool_, nullptr);
}

CommandBuffer CommandPool::allocateCommandBuffer(VkCommandBufferLevel level) const {
  return std::move(CommandBuffer(device_, *this, level));
}

CommandBuffer::CommandBuffer(CommandBuffer&& other)
    : device_(other.device_),
      parent_(other.parent_) {
  commandBuffer_ = other.commandBuffer_;
  other.commandBuffer_ = NULL;
}

CommandBuffer::~CommandBuffer() {
  if (commandBuffer_) {
    vkFreeCommandBuffers(device_, parent_, 1, &commandBuffer_);
  }
}

CommandBuffer::CommandBuffer(const LogicalDevice& device,
                             const CommandPool& parent,
                             VkCommandBufferLevel level)
    : device_(device),
      parent_(parent) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = parent_;
  allocInfo.level = level;  // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can
                            // be submitted to a queue for
                            // execution, but cannot be called from
                            // other command buffers.
                            // VK_COMMAND_BUFFER_LEVEL_SECONDARY:
                            // Cannot be submitted directly, but can
                            // be called from primary command
                            // buffers.

  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer_) !=
      VK_SUCCESS) {
    LOG_F("failed to allocate command buffer!");
  }
}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags) {
  if (started_) {
    LOG_F("Cannot begin() a command buffer that has already started!");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags =
      flags;  // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer
              // will be rerecorded right after executing it once.
              // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a
              // secondary command buffer that will be entirely within a single
              // render pass. VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The
              // command buffer can be resubmitted while it is also already
              // pending execution.
  beginInfo.pInheritanceInfo =
      nullptr;  // Optional, only used for "secondary" command buffers

  if (vkBeginCommandBuffer(commandBuffer_, &beginInfo) != VK_SUCCESS) {
    LOG_F("failed to begin recording command buffer!");
  }

  started_ = true;
}

void CommandBuffer::end() {
  if (vkEndCommandBuffer(commandBuffer_) != VK_SUCCESS) {
    LOG_F("failed to record command buffer!");
  }
}

void CommandBuffer::beginRenderPass(const RenderPass& renderPass,
                                    const Framebuffer& framebuffer) {
  // We need a VkClearValue for every attachment that uses
  // VK_ATTACHMENT_LOAD_OP_CLEAR. For now, that's all our attachments, and we
  // will just make them all clear to black.
  std::vector<VkClearValue> clearColors;
  clearColors.resize(renderPass.getAttachmentCount(),
                     {{{0.0f, 0.0f, 0.0f, 1.0f}}});

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = framebuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = renderPass.getExtent();
  renderPassInfo.clearValueCount = clearColors.size();
  renderPassInfo.pClearValues = clearColors.data();

  // Record this command to the command buffer
  vkCmdBeginRenderPass(commandBuffer_,
                       &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::endRenderPass() { vkCmdEndRenderPass(commandBuffer_); }

void CommandBuffer::draw(uint32_t vertexCount,
                         uint32_t instanceCount,
                         uint32_t firstVertexIndex,
                         uint32_t firstInstanceIndex) {
  vkCmdDraw(commandBuffer_,
            vertexCount,        // Num Vertices to draw
            instanceCount,      // Instance count
            firstVertexIndex,   // First vertex index offset
            firstInstanceIndex  // First instance offset
  );
}