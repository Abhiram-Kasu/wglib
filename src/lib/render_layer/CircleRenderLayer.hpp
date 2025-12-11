//
// Created by Abhiram Kasu on 12/10/25.
//
#pragma once

#include "render_layer.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

namespace wglib::render_layers
{
    class CircleRenderLayer : public render_layers::RenderLayer
    {
    private:
        glm::vec2 m_origin;
        float m_radius;
        glm::vec3 m_color;

        static std::optional<wgpu::RenderPipeline> pipeline;

    public:
        CircleRenderLayer(glm::vec2 origin, float radius, glm::vec3 color);

        auto initRes(const wgpu::Device& device, wgpu::TextureFormat format,
                     const wgpu::BindGroupLayout& bindGroupLayout) -> void override;

        auto Render(wgpu::RenderPassEncoder& renderPassEncoder,
                    wgpu::CommandEncoder commandEncoder) const -> void override;

        auto getOrigin() const -> glm::vec2;

        auto setOrigin(glm::vec2 origin) -> void;
        auto getRadius() const -> float;

        auto setRadius(float radius) -> void;

        ~CircleRenderLayer() override;
    };
}