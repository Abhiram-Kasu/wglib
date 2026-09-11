//
// Created by Abhiram Kasu on 12/1/25.
//

#pragma once

#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "webgpu/webgpu_cpp.h"
namespace wglib::render_layers
{
struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;

    constexpr static auto getVertexBufferLayout() noexcept
    {
        constexpr static wgpu::VertexAttribute attributes[2]{
            {
                .format = wgpu::VertexFormat::Float32x2,
                .offset = 0,
                .shaderLocation = 0,
            },
            {
                .format = wgpu::VertexFormat::Float32x3,
                .offset = sizeof(float) * 2,
                .shaderLocation = 1,
            },
        };
        return wgpu::VertexBufferLayout{
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(Vertex),
            .attributeCount = 2,

            .attributes = attributes,
        };
    }
};
} // namespace wglib::render_layers
