#pragma once

#include <engine/core/Device.hpp>
#include <engine/core/RenderPass.hpp>
#include <engine/core/GraphicsPipeline.hpp>

#include <vulkan/vulkan.h>

class CommandBuffer;

class CommandPool {
 public:
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

  void bindPipeline(const GraphicsPipeline& pipeline);

  void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertexIndex, uint32_t firstInstanceIndex);

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