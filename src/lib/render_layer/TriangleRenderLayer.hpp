
#pragma once
#include "lib/render_layer/RenderLayer.hpp"
#include "lib/render_layer/Vertex.hpp"
#include "webgpu/webgpu_cpp.h"
namespace wglib::render_layers
{
class TriangleRenderLayer : public RenderLayer
{

  public:
    TriangleRenderLayer(std::array<Vertex, 3> vertices);

    auto Render(wgpu::RenderPassEncoder &renderPassEncoder) const -> void override;

    auto InitRes(const wgpu::Device &device, wgpu::TextureFormat format, const wgpu::BindGroupLayout &bindGroupLayout)
        -> void override;

    auto UpdateRes(const wgpu::Device &device) const -> void override;

    auto getVertices() -> const std::array<Vertex, 3> &
    {
        return m_vertices;
    }
    auto setVertices(const std::array<Vertex, 3> &vertices) -> void
    {
        m_vertices = vertices;
        m_is_dirty = true;
    }
    template <uint8_t Index>
        requires(Index < 3)
    auto setVertex(const Vertex &vertex) -> void
    {
        m_vertices[Index] = vertex;
        m_is_dirty = true;
    }

  private:
    wgpu::Buffer m_vertex_buffer;
    wgpu::RenderPipeline m_render_pipeline;

    std::array<Vertex, 3> m_vertices;
    mutable bool m_is_dirty;
};
} // namespace wglib::render_layers
