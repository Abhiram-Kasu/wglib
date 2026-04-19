#include "TriangleRenderLayer.hpp"
#include "lib/CoreUtil.hpp"
#include "lib/render_layer/Vertex.hpp"
#include "webgpu/webgpu_cpp.h"

namespace wglib::render_layers
{
TriangleRenderLayer::TriangleRenderLayer(std::array<Vertex, 3> vertices) : m_vertices(vertices), m_is_dirty(false)
{
}

auto TriangleRenderLayer::InitRes(const wgpu::Device &device, wgpu::TextureFormat format,
                                  const wgpu::BindGroupLayout &bindGroupLayout) -> void
{
    m_vertex_buffer =
        util::createBuffer<Vertex, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex>(device, 3, true);
    m_vertex_buffer.WriteMappedRange(0, &m_vertices, sizeof(m_vertices));
    m_vertex_buffer.Unmap();
    // TODO remove
    util::log("Created Buffer and wrote to it");

    auto shader_module = util::createShaderModuleFromFile("../src/shaders/default.wgsl", device);
    auto vertex_buffer_layout = Vertex::getVertexBufferLayout();
    auto colorTargetState = wgpu::ColorTargetState{.format = format};

    auto fragmentState = wgpu::FragmentState{.module = shader_module, .targetCount = 1, .targets = &colorTargetState};

    auto layoutDesc = wgpu::PipelineLayoutDescriptor{.bindGroupLayoutCount = 1, .bindGroupLayouts = &bindGroupLayout};
    auto pipelineLayout = device.CreatePipelineLayout(&layoutDesc);

    auto descriptor = wgpu::RenderPipelineDescriptor{.layout = pipelineLayout,
                                                     .vertex =
                                                         {
                                                             .module = shader_module,
                                                             .bufferCount = 1,
                                                             .buffers = &vertex_buffer_layout,
                                                         },
                                                     .fragment = &fragmentState};
    m_render_pipeline = device.CreateRenderPipeline(&descriptor);
}

auto TriangleRenderLayer::UpdateRes(const wgpu::Device &device) const -> void
{
    if (m_is_dirty)
    {
        device.GetQueue().WriteBuffer(m_vertex_buffer, 0, &m_vertices, sizeof(m_vertices));
        m_is_dirty = false;
    }
}

auto TriangleRenderLayer::Render(wgpu::RenderPassEncoder &encoder) const -> void
{

    encoder.SetPipeline(m_render_pipeline);
    encoder.SetVertexBuffer(0, m_vertex_buffer);
    encoder.Draw(3, 1);
}

} // namespace wglib::render_layers
