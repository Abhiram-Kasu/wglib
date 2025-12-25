#pragma once

#include "RenderLayer.hpp"
#include <webgpu/webgpu_cpp.h>

namespace wglib::render_layers {

class TextureRenderLayer : public RenderLayer {
public:
  TextureRenderLayer(wgpu::Texture *texture, float width, float height);
  TextureRenderLayer(float width, float height);
  ~TextureRenderLayer() override;

  auto Render(wgpu::RenderPassEncoder &renderPassEncoder) const
      -> void override;

  auto InitRes(const wgpu::Device &device, wgpu::TextureFormat format,
               const wgpu::BindGroupLayout &bindGroupLayout) -> void override;

  auto UpdateRes(const wgpu::Device &device) const -> void override;

  void setTexture(wgpu::Texture *texture);
  [[nodiscard]] auto getTexture() const -> wgpu::Texture *;

private:
  wgpu::Texture *m_texture;
  float m_width;
  float m_height;

  wgpu::RenderPipeline m_pipeline = nullptr;
  wgpu::Buffer m_vertexBuffer = nullptr;
  mutable wgpu::BindGroup m_bindGroup = nullptr;
  wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
  mutable wgpu::TextureView m_textureView = nullptr;
  mutable wgpu::Sampler m_sampler = nullptr;

  mutable bool m_isDirty = true;
};

} // namespace wglib::render_layers
