//
// Created by Abhiram Kasu on 12/10/25.
//
#pragma once

#include "RenderLayer.hpp"
#include "lib/render_layer/Vertex.hpp"
#include "webgpu/webgpu_cpp.h"

namespace wglib::render_layers {
class CircleRenderLayer : public render_layers::RenderLayer {
public:
  constexpr static auto DEFAULT_RESOLUTION = 50u;

private:
  glm::vec2 m_origin;
  float m_radius;
  glm::vec3 m_color;
  uint32_t m_resolution;

  mutable wgpu::Buffer m_vertex_buffer;
  mutable bool m_vertex_buffer_dirty{false};

  mutable wgpu::Buffer m_index_buffer;
  mutable bool m_index_buffer_dirty{false};

  std::vector<uint32_t> m_indices;

  std::vector<Vertex> m_vertices;

  bool m_isInitialized{false};

  auto calculateVertices() -> void;
  static std::optional<wgpu::RenderPipeline> m_render_pipeline;
  static auto initRenderPipeline(const wgpu::Device &, wgpu::TextureFormat,
                                 const wgpu::BindGroupLayout &) -> void;

public:
  CircleRenderLayer(glm::vec2 origin, float radius, glm::vec3 color,
                    uint32_t resolution = DEFAULT_RESOLUTION);

  auto InitRes(const wgpu::Device &device, wgpu::TextureFormat format,
               const wgpu::BindGroupLayout &bindGroupLayout) -> void override;

  auto UpdateRes(const wgpu::Device &device) const -> void override;

  auto Render(wgpu::RenderPassEncoder &renderPassEncoder) const
      -> void override;

  auto getOrigin() const -> glm::vec2;

  auto setOrigin(glm::vec2 origin) -> void;
  auto getRadius() const -> float;

  auto setRadius(float radius) -> void;

  auto getResolution() const -> uint32_t;

  auto setResolution(uint32_t resolution) -> void;

  auto getColor() const -> glm::vec3;

  auto setColor(glm::vec3 color) -> void;

  ~CircleRenderLayer() override;
};
} // namespace wglib::render_layers
