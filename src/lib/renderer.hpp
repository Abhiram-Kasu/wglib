#pragma once
#include <memory>
#include <webgpu/webgpu_cpp.h>

#include "render_layer/render_layer.hpp"

namespace wglib {
template <typename T>
concept RenderableLayer = std::derived_from<T, render_layers::RenderLayer>;

class Renderer {
private:
  const wgpu::Instance &m_instance;
  const wgpu::Adapter &m_adapter;
  const wgpu::Device &m_device;
  const wgpu::TextureFormat m_format;

  std::vector<std::unique_ptr<render_layers::RenderLayer>> m_render_layers{};

public:
  Renderer(const wgpu::Instance &instance, wgpu::Adapter &adapter,
           wgpu::Device &device, wgpu::TextureFormat format);

  template <RenderableLayer L, typename... T>
    requires std::constructible_from<L, T...>
  auto pushRenderLayer(T &&...args) {
    auto ptr = std::make_unique<L>(std::forward<T>(args)...);
    ptr->initRes(m_device, m_format);
    m_render_layers.push_back(std::move(ptr));
  }

  auto Render(wgpu::SurfaceTexture &) -> void;

  ~Renderer();
};
} // namespace wglib
