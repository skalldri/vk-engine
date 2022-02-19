#pragma once

#include <vulkan/vulkan.h>

#include <engine/core/Device.hpp>
#include <engine/core/GraphicsPipeline.hpp>
#include <engine/core/RenderPass.hpp>
#include <engine/core/Buffer.hpp>

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

  CommandBuffer allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

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
    // TODO: figure out how to validate that the currently bound pipeline is compatible with this input
    VkBuffer vertexBuffers[] = {buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer_, 0 /* first binding */, 1 /* num buffers */, vertexBuffers, offsets);
  }

  void draw(uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertexIndex,
            uint32_t firstInstanceIndex);

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