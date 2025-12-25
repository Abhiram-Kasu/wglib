#include "TextureRenderLayer.hpp"
#include "../Util.hpp"
#include <vector>

namespace wglib::render_layers {

TextureRenderLayer::TextureRenderLayer(wgpu::Texture *texture, float width,
                                       float height)
    : m_texture(texture), m_width(width), m_height(height), m_isDirty(true) {}

TextureRenderLayer::TextureRenderLayer(float width, float height)
    : m_texture(nullptr), m_width(width), m_height(height), m_isDirty(true) {}

TextureRenderLayer::~TextureRenderLayer() {
  if (m_vertexBuffer) {
    m_vertexBuffer.Destroy();
  }
}

void TextureRenderLayer::Render(
    wgpu::RenderPassEncoder &renderPassEncoder) const {
  if (!m_pipeline || !m_bindGroup) {
    return;
  }
  renderPassEncoder.SetPipeline(m_pipeline);
  renderPassEncoder.SetBindGroup(0, m_bindGroup);
  renderPassEncoder.SetVertexBuffer(0, m_vertexBuffer);
  renderPassEncoder.Draw(4, 1, 0, 0);
}

void TextureRenderLayer::InitRes(const wgpu::Device &device,
                                 wgpu::TextureFormat format,
                                 const wgpu::BindGroupLayout &) {
  // Vertex buffer
  const std::vector<float> vertices = {
      -1.0f, -1.0f, // bottom-left
      1.0f,  -1.0f, // bottom-right
      -1.0f, 1.0f,  // top-left
      1.0f,  1.0f,  // top-right
  };

  const auto bufferDesc = wgpu::BufferDescriptor{
      .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
      .size = vertices.size() * sizeof(float),
  };
  m_vertexBuffer = device.CreateBuffer(&bufferDesc);
  device.GetQueue().WriteBuffer(m_vertexBuffer, 0, vertices.data(),
                                bufferDesc.size);

  // Shader
  const auto shaderModule =
      util::createShaderModuleFromFile("../src/shaders/texture.wgsl", device);

  // Bind group layout
  const wgpu::BindGroupLayoutEntry bglEntries[] = {
      {
          .binding = 0,
          .visibility = wgpu::ShaderStage::Fragment,
          .texture =
              {
                  .sampleType = wgpu::TextureSampleType::Float,
                  .viewDimension = wgpu::TextureViewDimension::e2D,
              },
      },
      {
          .binding = 1,
          .visibility = wgpu::ShaderStage::Fragment,
          .sampler =
              {
                  .type = wgpu::SamplerBindingType::Filtering,
              },
      },
  };

  const auto bglDesc = wgpu::BindGroupLayoutDescriptor{
      .entryCount = std::size(bglEntries),
      .entries = bglEntries,
  };
  m_bindGroupLayout = device.CreateBindGroupLayout(&bglDesc);

  // Pipeline layout
  const auto layoutDesc = wgpu::PipelineLayoutDescriptor{
      .bindGroupLayoutCount = 1,
      .bindGroupLayouts = &m_bindGroupLayout,
  };
  const auto pipelineLayout = device.CreatePipelineLayout(&layoutDesc);

  // Pipeline
  const wgpu::VertexAttribute vertexAttrib{
      .format = wgpu::VertexFormat::Float32x2,
      .offset = 0,
      .shaderLocation = 0,
  };

  const wgpu::VertexBufferLayout vertexBufferLayout{
      .arrayStride = 2 * sizeof(float),
      .attributeCount = 1,
      .attributes = &vertexAttrib,
  };

  const wgpu::ColorTargetState colorTarget{
      .format = format,
  };

  const wgpu::FragmentState fragmentState{
      .module = shaderModule,
      .entryPoint = "fs_main",
      .targetCount = 1,
      .targets = &colorTarget,
  };

  const auto pipelineDesc = wgpu::RenderPipelineDescriptor{
      .layout = pipelineLayout,
      .vertex =
          {
              .module = shaderModule,
              .entryPoint = "vs_main",
              .bufferCount = 1,
              .buffers = &vertexBufferLayout,
          },
      .primitive =
          {
              .topology = wgpu::PrimitiveTopology::TriangleStrip,
          },
      .fragment = &fragmentState,
  };

  m_pipeline = device.CreateRenderPipeline(&pipelineDesc);
}

auto TextureRenderLayer::UpdateRes(const wgpu::Device &device) const -> void {
  if (!m_isDirty) {
    return;
  }

  if (m_texture) {
    bool needsBindGroup = false;

    if (!m_sampler) {
      const auto samplerDesc = wgpu::SamplerDescriptor{
          .magFilter = wgpu::FilterMode::Linear,
          .minFilter = wgpu::FilterMode::Linear,
      };
      m_sampler = device.CreateSampler(&samplerDesc);
      needsBindGroup = true;
    }

    if (!m_textureView) {
      m_textureView = m_texture->CreateView();
      needsBindGroup = true;
    }

    if (needsBindGroup) {
      const wgpu::BindGroupEntry bgEntries[] = {
          {
              .binding = 0,
              .textureView = m_textureView,
          },
          {
              .binding = 1,
              .sampler = m_sampler,
          },
      };

      const auto bgDesc = wgpu::BindGroupDescriptor{
          .layout = m_bindGroupLayout,
          .entryCount = 2,
          .entries = bgEntries,
      };
      m_bindGroup = device.CreateBindGroup(&bgDesc);
    }
  } else {
    m_bindGroup = nullptr;
  }

  m_isDirty = false;
}

void TextureRenderLayer::setTexture(wgpu::Texture *texture) {
  m_texture = texture;
  m_isDirty = true;
}

auto TextureRenderLayer::getTexture() const -> wgpu::Texture * {
  return m_texture;
}

} // namespace wglib::render_layers
