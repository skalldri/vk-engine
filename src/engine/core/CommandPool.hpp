#pragma once

#include <vulkan/vulkan.h>

#include <engine/core/Buffer.hpp>
#include <engine/core/Device.hpp>
#include <engine/core/GraphicsPipeline.hpp>
#include <engine/core/RenderPass.hpp>
#include <typeinfo>

class CommandBuffer;

class CommandPool {
 public:
  CommandPool() = delete;
  CommandPool(CommandPool& other) = delete;
  CommandPool(const CommandPool& other) = delete;

  CommandPool(CommandPool&& other);
  CommandPool(const LogicalDevice& device, const QueueFamilyRequest& queue);

  ~CommandPool();

  operator VkCommandPool() const { return commandPool_; }

  const LogicalDevice& getDevice() const { return device_; }

  const QueueFamilyRequest& getQueue() const { return queue_; }

  CommandBuffer allocateCommandBuffer(
      VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

 private:
  const LogicalDevice& device_;
  const QueueFamilyRequest& queue_;
  VkCommandPool commandPool_;
};

class CommandBuffer {
  friend class CommandPool;

 public:
  CommandBuffer() = delete;
  CommandBuffer(CommandBuffer& other) = delete;
  CommandBuffer(CommandBuffer&& other);
  ~CommandBuffer();

  // TODO: maybe express the lifetime of the recording using a special "recording" object that
  // ends the recording when the object goes out of scope?
  void begin(VkCommandBufferUsageFlags flags = 0);
  void end();

  void beginRenderPass(const RenderPass& renderPass, const Framebuffer& framebuffer);
  void endRenderPass();

  template <class InputType>
  void bindPipeline(const GraphicsPipeline<InputType>& pipeline) {
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  }

  template <class InputType>
  void bindVertexBuffers(const Buffer<InputType>& buffer) {
    // TODO: figure out how to validate that the currently bound pipeline is compatible with this
    // input
    VkBuffer vertexBuffers[] = {buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer_,
                           0 /* first binding */,
                           1 /* num buffers */,
                           vertexBuffers,
                           offsets);
  }

  template <class InputType>
  void bindIndexBuffer(const Buffer<InputType>& buffer);

  // Template specialization for UINT16 index buffers
  template <>
  void bindIndexBuffer<uint16_t>(const Buffer<uint16_t>& buffer) {
    vkCmdBindIndexBuffer(commandBuffer_, buffer, 0 /* offset */, VK_INDEX_TYPE_UINT16);
  }

  void draw(uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertexIndex,
            uint32_t firstInstanceIndex);

  void drawIndexed(uint32_t vertexCount,
                   uint32_t instanceCount,
                   uint32_t firstIndexOffset,
                   uint32_t indexValueOffset,
                   uint32_t instanceOffset);

  template <class InputType>
  void copyBuffer(const Buffer<InputType>& src, const Buffer<InputType>& dst) {
    if (src.getBufferSize() != dst.getBufferSize()) {
      LOG_F("Src and dst buffer sizes don't match!");
    }

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = dst.getBufferSize();

    vkCmdCopyBuffer(commandBuffer_, src, dst, 1 /* num copy regions */, &copyRegion);
  }

  operator VkCommandBuffer() const { return commandBuffer_; }

 protected:
  CommandBuffer(const LogicalDevice& device,
                const CommandPool& parent,
                VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

 private:
  VkCommandBuffer commandBuffer_;
  const CommandPool& parent_;
  const LogicalDevice& device_;

  bool started_ = false;
};