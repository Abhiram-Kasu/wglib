//
// Created by Abhiram Kasu on 11/27/25.
//

#include "CoreRenderer.hpp"

#include <ranges>

#include "CoreUtil.hpp"
#include "webgpu/webgpu_cpp.h"

namespace wglib {
Renderer::Renderer(const wgpu::Instance &instance, wgpu::Adapter &adapter,
                   wgpu::Device &device, wgpu::TextureFormat format,
                   glm::vec2 screenSize)
    : m_instance(instance), m_adapter(adapter), m_device(device),
      m_format(format), m_uniforms(screenSize) {
  createBindGroupLayout();
  updateUniformBuffer();
}

auto Renderer::createBindGroupLayout() -> void {
  wgpu::BindGroupLayoutEntry entry{
      .binding = 0,
      .visibility = wgpu::ShaderStage::Vertex,
      .buffer = {.type = wgpu::BufferBindingType::Uniform,
                 .minBindingSize = sizeof(Uniforms)}};

  wgpu::BindGroupLayoutDescriptor layoutDesc{.entryCount = 1,
                                             .entries = &entry};

  m_bind_group_layout = m_device.CreateBindGroupLayout(&layoutDesc);
}

auto Renderer::updateUniformBuffer() -> void {
  wgpu::BufferDescriptor bufferDesc{.mappedAtCreation = true,
                                    .usage = wgpu::BufferUsage::CopyDst |
                                             wgpu::BufferUsage::Uniform,
                                    .size = sizeof(Uniforms)};
  m_uniform_buffer = m_device.CreateBuffer(&bufferDesc);
  m_uniform_buffer.WriteMappedRange(0, &m_uniforms, sizeof(Uniforms));
  m_uniform_buffer.Unmap();
}

auto Renderer::Render(wgpu::SurfaceTexture &surfaceTexture) -> void {
  for (auto &layer : m_render_layers) {
    layer.get().UpdateRes(m_device);
  }

  wgpu::BindGroupEntry entry{.binding = 0,
                             .buffer = m_uniform_buffer,
                             .offset = 0,
                             .size = sizeof(Uniforms)};
  wgpu::BindGroupDescriptor desc{
      .layout = m_bind_group_layout,
      .entryCount = 1,
      .entries = &entry,
  };
  auto m_bind_group = m_device.CreateBindGroup(&desc);

  wgpu::RenderPassColorAttachment attachment{
      .view = surfaceTexture.texture.CreateView(),
      .loadOp = wgpu::LoadOp::Clear,
      .storeOp = wgpu::StoreOp::Store};

  wgpu::RenderPassDescriptor renderPassDesc{.colorAttachmentCount = 1,
                                            .colorAttachments = &attachment};

  auto encoder = m_device.CreateCommandEncoder();
  auto renderPass = encoder.BeginRenderPass(&renderPassDesc);

  renderPass.SetBindGroup(0, m_bind_group, 0, 0);

  for (auto &layer : m_render_layers) {
    layer.get().Render(renderPass);
  }

  renderPass.End();

  const auto encoderFinish = encoder.Finish();
  m_device.GetQueue().Submit(1, &encoderFinish);

  m_render_layers.clear();
}

auto Renderer::pushRenderLayer(render_layers::RenderLayer &renderLayer)
    -> void {
  renderLayer.InitRes(m_device, m_format, m_bind_group_layout);
  std::reference_wrapper renderLayerRef{renderLayer};
  m_render_layers.emplace_back(renderLayerRef);
}

Renderer::~Renderer() = default;
} // namespace wglib
