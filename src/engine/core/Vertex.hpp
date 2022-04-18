#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;  // Which slot is the actual data bound to. Need to be dynamic?
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.resize(2);

    attributeDescriptions[0].binding =
        0;  // Which slot is the actual data bound to. Need to be dynamic?
    attributeDescriptions[0].location = 0;  // inPosition
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding =
        0;  // Which slot is the actual data bound to. Need to be dynamic?
    attributeDescriptions[1].location = 1;  // inColor
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
  }
};
