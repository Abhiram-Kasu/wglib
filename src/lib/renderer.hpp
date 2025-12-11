#pragma once
#include <functional>
#include <memory>
#include <webgpu/webgpu_cpp.h>

#include "glm/ext/vector_float2.hpp"
#include "render_layer/render_layer.hpp"

namespace wglib
{

struct Uniforms
{
    glm::vec2 screen_size;
};

template <typename T>
concept RenderableLayer = std::derived_from<T, render_layers::RenderLayer>;

class Renderer
{
private:
    const wgpu::Instance &m_instance;
    const wgpu::Adapter &m_adapter;
    const wgpu::Device &m_device;
    const wgpu::TextureFormat m_format;

    std::vector<std::reference_wrapper<const render_layers::RenderLayer> >
    m_render_layers{};

    Uniforms m_uniforms;
    wgpu::Buffer m_uniform_buffer;
    wgpu::BindGroupLayout m_bind_group_layout;

    auto updateUniformBuffer() -> void;

    auto createBindGroupLayout() -> void;

public:
    Renderer(const wgpu::Instance &instance, wgpu::Adapter &adapter,
             wgpu::Device &device, wgpu::TextureFormat format,
             glm::vec2 screenSize);

    auto pushRenderLayer(render_layers::RenderLayer &renderLayer) -> void;

    auto Render(wgpu::SurfaceTexture &) -> void;

    ~Renderer();
};
} // namespace wglib