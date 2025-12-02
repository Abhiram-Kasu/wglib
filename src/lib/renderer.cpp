//
// Created by Abhiram Kasu on 11/27/25.
//

#include "renderer.hpp"

namespace wglib {
Renderer::Renderer(const wgpu::Instance &instance, wgpu::Adapter &adapter,
                   wgpu::Device &device, wgpu::TextureFormat format)
    : m_instance(instance), m_adapter(adapter), m_device(device),
      m_format(format) {}

auto Renderer::Render(wgpu::SurfaceTexture &surfaceTexture) -> void {

  // Create render pass
  wgpu::RenderPassColorAttachment attachment{
      .view = surfaceTexture.texture.CreateView(),
      .loadOp = wgpu::LoadOp::Clear,
      .storeOp = wgpu::StoreOp::Store};

  wgpu::RenderPassDescriptor renderPassDesc{.colorAttachmentCount = 1,
                                            .colorAttachments = &attachment};

  auto encoder = m_device.CreateCommandEncoder();
  auto renderPass = encoder.BeginRenderPass(&renderPassDesc);

  for (auto &layer : m_render_layers) {
    layer->Render(renderPass);
  }

  renderPass.End();
  const auto encoderFinish = encoder.Finish();

  m_device.GetQueue().Submit(1, &encoderFinish);
}

Renderer::~Renderer() {}
} // namespace wglib
