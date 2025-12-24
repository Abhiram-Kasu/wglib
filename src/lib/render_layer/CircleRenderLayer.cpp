//
// Created by Abhiram Kasu on 12/10/25.
//

#include "CircleRenderLayer.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "lib/Util.hpp"
#include "webgpu/webgpu_cpp.h"
#include <__ostream/print.h>
#include <cassert>
#include <ranges>

namespace wglib::render_layers {

std::optional<wgpu::RenderPipeline> CircleRenderLayer::m_render_pipeline{
    std::nullopt};

CircleRenderLayer::CircleRenderLayer(glm::vec2 origin, float radius,
                                     glm::vec3 color, uint32_t resolution)
    : m_origin(origin), m_radius(radius), m_color(color),
      m_resolution(resolution) {
  assert(m_resolution >= 3 && "Resolution must be at least 3 to form a circle");
  calculateVertices();
}

auto CircleRenderLayer::Render(wgpu::RenderPassEncoder &renderPassEncoder) const
    -> void {
  renderPassEncoder.SetPipeline(m_render_pipeline.value());
  renderPassEncoder.SetVertexBuffer(0, m_vertex_buffer);
  renderPassEncoder.SetIndexBuffer(m_index_buffer, wgpu::IndexFormat::Uint32, 0,
                                   m_indices.size() * sizeof(uint32_t));
  renderPassEncoder.DrawIndexed(static_cast<uint32_t>(m_indices.size()));
}

auto CircleRenderLayer::initRenderPipeline(
    const wgpu::Device &device, wgpu::TextureFormat format,
    const wgpu::BindGroupLayout &bindGroupLayout) -> void {
  if (m_render_pipeline.has_value())
    return;
  const auto shaderCode = wglib::util::readFile("../src/shaders/default.wgsl");
  wgpu::ShaderSourceWGSL wgsl{{.code = shaderCode.c_str()}};
  wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};
  wgpu::ShaderModule shaderModule =
      device.CreateShaderModule(&shaderModuleDescriptor);

  wgpu::VertexAttribute attributes[2]{
      {
          .format = wgpu::VertexFormat::Float32x2,
          .offset = 0,
          .shaderLocation = 0,
      },
      {
          .format = wgpu::VertexFormat::Float32x3,
          .offset = sizeof(float) * 2,
          .shaderLocation = 1,
      },
  };
  wgpu::VertexBufferLayout vertexBufferLayout{
      .stepMode = wgpu::VertexStepMode::Vertex,
      .arrayStride = sizeof(Vertex),
      .attributeCount = 2,

      .attributes = attributes,
  };
  wgpu::ColorTargetState colorTargetState{.format = format};

  wgpu::FragmentState fragmentState{
      .module = shaderModule, .targetCount = 1, .targets = &colorTargetState};

  // Create pipeline layout using the bind group layout
  wgpu::PipelineLayoutDescriptor layoutDesc{
      .bindGroupLayoutCount = 1, .bindGroupLayouts = &bindGroupLayout};
  wgpu::PipelineLayout pipelineLayout =
      device.CreatePipelineLayout(&layoutDesc);

  wgpu::RenderPipelineDescriptor descriptor{
      .layout = pipelineLayout,
      .vertex =
          {
              .module = shaderModule,
              .bufferCount = 1,
              .buffers = &vertexBufferLayout,
          },
      .fragment = &fragmentState};
  m_render_pipeline =
      std::make_optional(device.CreateRenderPipeline(&descriptor));
  util::log("Created render pipeline");
}

auto CircleRenderLayer::InitRes(const wgpu::Device &device,
                                wgpu::TextureFormat format,
                                const wgpu::BindGroupLayout &bindGroupLayout)
    -> void {
  if (m_isInitialized)
    return;

  const wgpu::BufferDescriptor vertexBufferDesc{
      .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
      .size = sizeof(Vertex) * m_vertices.size(),
      .mappedAtCreation = true};

  m_vertex_buffer = device.CreateBuffer(&vertexBufferDesc);
  if (m_vertex_buffer.WriteMappedRange(0, m_vertices.data(),
                                       sizeof(Vertex) * m_vertices.size()) !=
      wgpu::Status::Success) {
    util::log("Failed to write vertex buffer data");
    exit(0);
  }
  m_vertex_buffer.Unmap();

  const wgpu::BufferDescriptor indexBufferDesc{
      .usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst,
      .size = sizeof(uint32_t) * m_indices.size(),
      .mappedAtCreation = true};
  m_index_buffer = device.CreateBuffer(&indexBufferDesc);
  if (m_index_buffer.WriteMappedRange(0, m_indices.data(),
                                      sizeof(uint32_t) * m_indices.size()) !=
      wgpu::Status::Success) {
    util::log("Failed to write index buffer data");
    exit(0);
  }
  m_index_buffer.Unmap();

  if (not m_render_pipeline)
    initRenderPipeline(device, format, bindGroupLayout);

  m_isInitialized = true;
}
auto CircleRenderLayer::UpdateRes(wgpu::CommandEncoder &commandEncoder,
                                  const wgpu::Device &device) const -> void {
  auto queue = device.GetQueue();

  if (m_vertex_buffer.GetSize() < sizeof(Vertex) * m_vertices.size() or
      m_index_buffer.GetSize() < sizeof(uint32_t) * m_indices.size()) {

    util::log("Realloc vertex and index buffer to {}b from {}b",
              m_vertices.size(), m_vertex_buffer.GetSize() / sizeof(Vertex));
    m_vertex_buffer.Destroy();
    const wgpu::BufferDescriptor descriptor{
        .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
        .size = sizeof(Vertex) * m_vertices.size(),
        .mappedAtCreation = false

    };
    m_vertex_buffer = device.CreateBuffer(&descriptor);

    m_index_buffer.Destroy();
    const wgpu::BufferDescriptor indexBufferDesc{
        .usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst,
        .size = sizeof(uint32_t) * m_indices.size(),
        .mappedAtCreation = false

    };
    m_index_buffer = device.CreateBuffer(&indexBufferDesc);
  }
  if (m_vertex_buffer_dirty) {

    queue.WriteBuffer(m_vertex_buffer, 0, m_vertices.data(),
                      sizeof(Vertex) * m_vertices.size());
    m_vertex_buffer_dirty = false;
  }
  if (m_index_buffer_dirty) {

    queue.WriteBuffer(m_index_buffer, 0, m_indices.data(),
                      sizeof(uint32_t) * m_indices.size());
    m_index_buffer_dirty = false;
  }
}

auto CircleRenderLayer::calculateVertices() -> void {

  const size_t vertex_count = m_resolution + 1;
  m_vertices.reserve(vertex_count);
  m_vertices.clear();

  m_vertices.push_back({m_origin, m_color});

  for (auto i : std::ranges::views::iota(0u, m_resolution)) {
    float angle =
        (i / static_cast<float>(m_resolution)) * 2.0f * glm::pi<float>();

    float x = m_radius * cos(angle) + m_origin.x;
    float y = m_radius * sin(angle) + m_origin.y;
    m_vertices.push_back({glm::vec2(x, y), m_color});
  }

  m_indices.reserve(m_resolution * 3);
  m_indices.clear();

  const size_t center_index = 0;

  for (auto i = 0; i < m_resolution; ++i) {

    const size_t p1_index = i + 1;

    const size_t p2_index = (i + 1) % m_resolution + 1;

    m_indices.push_back(center_index);
    m_indices.push_back(p1_index);
    m_indices.push_back(p2_index);
  }
}

auto CircleRenderLayer::getOrigin() const -> glm::vec2 { return m_origin; }

auto CircleRenderLayer::setOrigin(glm::vec2 origin) -> void {
  m_origin = origin;
  m_vertex_buffer_dirty = true;
  calculateVertices();
}

auto CircleRenderLayer::getRadius() const -> float { return m_radius; }

auto CircleRenderLayer::setRadius(float radius) -> void {
  m_radius = radius;
  m_vertex_buffer_dirty = true;
  calculateVertices();
}

auto CircleRenderLayer::getResolution() const -> uint32_t {
  return m_resolution;
}

auto CircleRenderLayer::setResolution(uint32_t resolution) -> void {
  assert(resolution >= 3 && "There must be atleast 3 triangles");
  m_resolution = resolution;
  m_index_buffer_dirty = m_vertex_buffer_dirty = true;
  calculateVertices();
}

auto CircleRenderLayer::getColor() const -> glm::vec3 { return m_color; }
auto CircleRenderLayer::setColor(glm::vec3 color) -> void {
  m_color = color;
  for (auto &v : m_vertices) {
    v.color = m_color;
  }
  m_vertex_buffer_dirty = true;
}

CircleRenderLayer::~CircleRenderLayer() = default;
} // namespace wglib::render_layers
