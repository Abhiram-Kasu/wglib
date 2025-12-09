//
// Created by Abhiram Kasu on 12/1/25.
//

#pragma once
#include "Vertex.hpp"
#include "glm/vec2.hpp"
#include "render_layer.hpp"
#include "webgpu/webgpu_cpp.h"

namespace wglib::render_layers {
    class RectangleRenderLayer : public RenderLayer {
    private:
        static std::optional<wgpu::RenderPipeline> m_render_pipeline;

        glm::vec2 m_size;
        glm::vec2 m_position;
        glm::vec3 m_color;

        wgpu::Buffer m_vertex_buffer;
        wgpu::Buffer m_index_buffer;

        Vertex m_vertices[6];

        auto initVertices() -> void;

        static auto initRenderPipeline(const wgpu::Device &,
                                       wgpu::TextureFormat,
                                       const wgpu::BindGroupLayout &) -> void;

    public:
        RectangleRenderLayer(glm::vec2 position, glm::vec2 size, glm::vec3 color);

        auto initRes(const wgpu::Device &device, wgpu::TextureFormat format,
                     const wgpu::BindGroupLayout &bindGroupLayout) -> void override;

        auto Render(wgpu::RenderPassEncoder &) -> void override;

        ~RectangleRenderLayer() override;
    };
} // namespace wglib::render_layers