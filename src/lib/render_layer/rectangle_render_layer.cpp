//
// Created by Abhiram Kasu on 12/1/25.
//

#include "rectangle_render_layer.hpp"

#include <print>

#include "Vertex.hpp"
#include "lib/util.hpp"

namespace wglib::render_layers {
std::optional<wgpu::RenderPipeline> RectangleRenderLayer::m_render_pipeline{
    std::nullopt};

RectangleRenderLayer::RectangleRenderLayer(const glm::vec2 position,
                                           const glm::vec2 size,
                                           const glm::vec3 color)
    : m_size(size), m_position(position), m_color(color), m_vertices{} {
  calculateVertices();
}

// TODO use index buffers to make it more efficient
auto RectangleRenderLayer::InitRes(const wgpu::Device &device,
                                   const wgpu::TextureFormat format,
                                   const wgpu::BindGroupLayout &bindGroupLayout)
    -> void {
  if (isInitialized)
    return;
  constexpr wgpu::BufferDescriptor bufferDesc{
      .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
      .size = sizeof(Vertex) * 6,
      .mappedAtCreation = true,

  };
  m_vertex_buffer = device.CreateBuffer(&bufferDesc);

  if (m_vertex_buffer.WriteMappedRange(0, m_vertices, sizeof(Vertex) * 6)
          .status != wgpu::Status::Success) {
    std::println("Failed to write to vertex buffer");
    exit(0);
  }
  m_vertex_buffer.Unmap();

  if (!m_render_pipeline)
    initRenderPipeline(device, format, bindGroupLayout);

  isInitialized = true;
}

auto RectangleRenderLayer::calculateVertices() -> void {
  const float x = m_position.x;
  const float y = m_position.y;
  const float w = m_size.x;
  const float h = m_size.y;
  m_vertices[0] = {{x, y}, m_color};         // Bottom-left
  m_vertices[1] = {{x + w, y}, m_color};     // Bottom-right
  m_vertices[2] = {{x + w, y + h}, m_color}; // Top-right
  m_vertices[3] = {{x, y}, m_color};         // Bottom-left
  m_vertices[4] = {{x + w, y + h}, m_color}; // Top-right
  m_vertices[5] = {{x, y + h}, m_color};     // Top-left
}

auto RectangleRenderLayer::updateVertexBuffer() -> void {
  m_vertex_buffer_dirty = true;
}

auto RectangleRenderLayer::initRenderPipeline(
    const wgpu::Device &device, wgpu::TextureFormat format,
    const wgpu::BindGroupLayout &bindGroupLayout) -> void {
  // Create render pipeline
  const auto shaderCode = util::readFile("../src/shaders/default.wgsl");
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
}

auto RectangleRenderLayer::Render(
    wgpu::RenderPassEncoder &renderPassEncoder) const -> void {
  assert(m_render_pipeline.has_value() && "Render pipeline not initialized");
  renderPassEncoder.SetPipeline(m_render_pipeline.value());
  renderPassEncoder.SetVertexBuffer(0, m_vertex_buffer);
  renderPassEncoder.Draw(6);
}

auto RectangleRenderLayer::UpdateRes(wgpu::CommandEncoder &commandEncoder,
                                     const wgpu::Device &device) const -> void {
  if (m_vertex_buffer_dirty) {
    commandEncoder.WriteBuffer(m_vertex_buffer, 0,
                               reinterpret_cast<const uint8_t *>(m_vertices),
                               sizeof(Vertex) * 6);
  }
}

auto RectangleRenderLayer::getPosition() const -> glm::vec2 {
  return m_position;
}

auto RectangleRenderLayer::setPosition(glm::vec2 pos) -> void {
  m_position = pos;
  calculateVertices();
  updateVertexBuffer();
}

auto RectangleRenderLayer::getSize() const -> glm::vec2 { return m_size; }

auto RectangleRenderLayer::setSize(glm::vec2 size) -> void {
  m_size = size;
  calculateVertices();
  updateVertexBuffer();
}

RectangleRenderLayer::~RectangleRenderLayer() {}
} // namespace wglib::render_layers
