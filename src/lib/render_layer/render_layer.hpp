//
// Created by Abhiram Kasu on 11/29/25.
//

#pragma once
#include <vector>

#include <webgpu/webgpu_cpp.h>

namespace wglib::render_layers {
class RenderLayer {
public:
  RenderLayer() = default;

  virtual auto Render(wgpu::RenderPassEncoder &encoder) const -> void = 0;

  virtual auto initRes(const wgpu::Device &device, wgpu::TextureFormat format,
                       const wgpu::BindGroupLayout &bindGroupLayout)
      -> void = 0;

  virtual ~RenderLayer();
};
} // namespace wglib::render_layers
