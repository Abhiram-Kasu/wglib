//
// Created by Abhiram Kasu on 12/1/25.
//

#pragma once

#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
namespace wglib::render_layers {
struct Vertex {
  glm::vec2 position;
  glm::vec3 color;
};
} // namespace wglib::render_layers
