//
// Created by Abhiram Kasu on 12/1/25.
//

#include "rectangle_render_layer.hpp"

#include <print>

#include "Vertex.hpp"
#include "lib/util.hpp"

namespace wglib::render_layers {
    RectangleRenderLayer::RectangleRenderLayer(const glm::vec2 position,
                                               const glm::vec2 size,
                                               const glm::vec3 color)
        : m_size(size), m_position(position), m_color(color), m_vertices{} {
        initVertices();
    }

    // TODO use index buffers to make it more efficient
    auto RectangleRenderLayer::initRes(const wgpu::Device &device,
                                       const wgpu::TextureFormat format) -> void {
        constexpr wgpu::BufferDescriptor bufferDesc{
            .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
            .size = sizeof(Vertex) * 6,
            .mappedAtCreation = true,

        };
        m_vertex_buffer = device.CreateBuffer(&bufferDesc);


        if (m_vertex_buffer.WriteMappedRange(0, m_vertices, sizeof(Vertex) * 6)
            .status != wgpu::Status::Success) {
            std::println("Failed to write to vertex buffer");
            exit(0);
        }
        m_vertex_buffer.Unmap();

        // Create render pipeline
        const auto shaderCode = util::readFile("../src/shaders/default.wgsl");
        wgpu::ShaderSourceWGSL wgsl{{.code = shaderCode.c_str()}};
        wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};
        wgpu::ShaderModule shaderModule =
                device.CreateShaderModule(&shaderModuleDescriptor);

        wgpu::VertexAttribute attributes[2]{
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
        wgpu::VertexBufferLayout vertexBufferLayout{
            .stepMode = wgpu::VertexStepMode::Vertex,
            .arrayStride = sizeof(Vertex),
            .attributeCount = 2,

            .attributes = attributes,
        };
        wgpu::ColorTargetState colorTargetState{.format = format};

        wgpu::FragmentState fragmentState{
            .module = shaderModule, .targetCount = 1, .targets = &colorTargetState
        };
        wgpu::RenderPipelineDescriptor descriptor{
            .vertex =
            {
                .module = shaderModule,
                .bufferCount = 1,
                .buffers = &vertexBufferLayout,
            },
            .fragment = &fragmentState
        };
        m_render_pipeline = device.CreateRenderPipeline(&descriptor);
    }

    auto RectangleRenderLayer::initVertices() -> void {
        const float x = m_position.x;
        const float y = m_position.y;
        const float w = m_size.x;
        const float h = m_size.y;
        m_vertices[0] = {{x, y}, m_color}; // Bottom-left
        m_vertices[1] = {{x + w, y}, m_color}; // Bottom-right
        m_vertices[2] = {{x + w, y + h}, m_color}; // Top-right
        m_vertices[3] = {{x, y}, m_color}; // Bottom-left
        m_vertices[4] = {{x + w, y + h}, m_color}; // Top-right
        m_vertices[5] = {{x, y + h}, m_color}; // Top-left
    }

    auto RectangleRenderLayer::Render(wgpu::RenderPassEncoder &renderPass) -> void {
        renderPass.SetPipeline(m_render_pipeline);
        renderPass.SetVertexBuffer(0, m_vertex_buffer);
        renderPass.Draw(6);
    }

    RectangleRenderLayer::~RectangleRenderLayer() {
    }
} // namespace wglib::render_layers