#pragma once
#include <concepts>
#include <functional>
#include <memory>
#include <webgpu/webgpu_cpp.h>

#include "glm/ext/vector_float2.hpp"
#include "render_layer/RenderLayer.hpp"

namespace wglib
{

struct alignas(16) Uniforms
{
    glm::vec2 screen_size;
};

template <typename T>
concept RenderableLayer = std::derived_from<T, render_layers::RenderLayer>;

class Renderer
{
  public:
    template <RenderableLayer Layer> struct Ref
    {
        auto operator<=>(const Ref &) const = default;

        Layer *operator->()
        {
            return layer.get();
        }
        Layer &operator*()
        {
            return *layer;
        }
        Layer *get()
        {
            return layer.get();
        }

        explicit operator bool() const
        {
            return layer != nullptr;
        }

      private:
        friend Renderer;

        const auto getLayer() const
        {
            return layer;
        }
        std::shared_ptr<Layer> layer;
        explicit Ref(std::shared_ptr<Layer> l) : layer(std::move(l))
        {
        }
    };

  private:
    const wgpu::Instance &m_instance;
    const wgpu::Adapter &m_adapter;
    const wgpu::Device &m_device;
    const wgpu::TextureFormat m_format;

    std::vector<std::shared_ptr<const render_layers::RenderLayer>> m_render_layers{};

    Uniforms m_uniforms;
    bool m_uniforms_dirty;
    wgpu::Buffer m_uniform_buffer;
    wgpu::BindGroupLayout m_bind_group_layout;

    auto UpdateUniformBuffer() -> void;

    auto CreateAndInitUniformBuffer() -> void;

    auto CreateBindGroupLayout() -> void;

  public:
    Renderer(const wgpu::Instance &instance, wgpu::Adapter &adapter, wgpu::Device &device, wgpu::TextureFormat format,
             glm::vec2 screenSize);
    template <std::derived_from<render_layers::RenderLayer> Layer> auto pushRenderLayer(Ref<Layer> &renderLayer) -> void
    {
        m_render_layers.push_back(renderLayer.getLayer());
    }

    auto Render(wgpu::SurfaceTexture &) -> void;

    template <RenderableLayer Layer, typename... Args> auto CreateRenderLayer(Args &&...args) const -> Ref<Layer>
    {
        auto layer = std::make_shared<Layer>(std::forward<Args>(args)...);
        layer->InitRes(m_device, m_format, m_bind_group_layout);
        return Ref<Layer>(layer);
    }

    auto SetUniforms(const Uniforms &value) -> void
    {
        m_uniforms = value;
        m_uniforms_dirty = true;
    }

    ~Renderer();
};
} // namespace wglib
