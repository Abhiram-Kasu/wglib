#include "ConwaysGameOfLife.hpp"
#include "lib/Util.hpp"
#include "webgpu/webgpu_cpp.h"
#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include <utility>

namespace wglib::compute {
ConwaysGameOfLifeComputeLayer::ConwaysGameOfLifeComputeLayer(glm::vec2 size)
    : m_size(size) {
  m_initalData.reserve(m_size.x * m_size.y);
  for (auto i{0uz}; i < m_size.x * m_size.y; ++i) {
    m_initalData.push_back(rand() % 2);
  }
}

auto ConwaysGameOfLifeComputeLayer::InitImpl(wgpu::Device &device) -> void {

  if (not m_init) {

    m_firstBuffer = util::createBuffer < uint32_t,
    wgpu::BufferUsage::CopySrc |
        wgpu::BufferUsage::Storage > (device, m_size.x * m_size.y, true);

    m_firstBuffer.WriteMappedRange(0, m_initalData.data(),
                                   m_initalData.size() * sizeof(uint32_t));

    m_firstBuffer.Unmap();

    m_secondBuffer = util::createBuffer < uint32_t,
    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc |
        wgpu::BufferUsage::Storage > (device, m_size.x * m_size.y);

    m_currBufferPointer = &m_firstBuffer;
    m_secBufferPointer = &m_secondBuffer;

    m_uniformBuffer = util::createBuffer < Uniform,
    wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Uniform > (device, 1, true);
    {
      Uniform uniform{static_cast<uint32_t>(m_size.x),
                      static_cast<uint32_t>(m_size.y)};
      m_uniformBuffer.WriteMappedRange(0, &uniform, sizeof(Uniform));
      m_uniformBuffer.Unmap();
    }

    // Create texture
    const wgpu::TextureDescriptor texDesc{
        .dimension = wgpu::TextureDimension::e2D,
        .size = {static_cast<uint32_t>(m_size.x),
                 static_cast<uint32_t>(m_size.y), 1},
        .format = wgpu::TextureFormat::RGBA8Unorm,
        .usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc |
                 wgpu::TextureUsage::TextureBinding |
                 wgpu::TextureUsage::StorageBinding,
        .label = "ConwaysGameOfLifeTexture"};
    m_texture = device.CreateTexture(&texDesc);
    m_textureView = m_texture.CreateView();

    // Set up pipeline and shaderModule

    wgpu::ComputePipelineDescriptor desc{
        .compute = {
            .module = util::createShaderModuleFromFile(
                "../src/shaders/ConwaysGameOfLife/compute.wgsl", device)}};
    m_computePipeline = device.CreateComputePipeline(&desc);
    m_init = true;
  }

  wgpu::BindGroupEntry entries[4]{
      {.binding = 0, .buffer = *m_currBufferPointer},
      {.binding = 1, .buffer = *m_secBufferPointer},
      {.binding = 2, .textureView = m_textureView},
      {.binding = 3, .buffer = m_uniformBuffer}};
  wgpu::BindGroupDescriptor desc{.entries = entries,
                                 .entryCount = 4,
                                 .layout =
                                     m_computePipeline.GetBindGroupLayout(0)

  };
  m_bindGroup = device.CreateBindGroup(&desc);
} // namespace wglib::compute::example_layers

auto ConwaysGameOfLifeComputeLayer::ComputeImpl(wgpu::CommandEncoder &encoder,
                                                wgpu::Queue &queue) -> void {

  auto computePass = encoder.BeginComputePass();
  computePass.SetPipeline(m_computePipeline);
  computePass.SetBindGroup(0, m_bindGroup);

  computePass.DispatchWorkgroups(
      util::divCeil(static_cast<size_t>(m_size.x), 8uz),
      util::divCeil(static_cast<size_t>(m_size.y), 8uz));
  computePass.End();
  const auto commandBuffer = encoder.Finish();
  queue.Submit(1, &commandBuffer);
  Swap();
}

auto ConwaysGameOfLifeComputeLayer::getResultImpl() -> const void * {

  return &m_texture;
}

auto ConwaysGameOfLifeComputeLayer::Swap() -> void {
  std::swap(m_currBufferPointer, m_secBufferPointer);
}

} // namespace wglib::compute
