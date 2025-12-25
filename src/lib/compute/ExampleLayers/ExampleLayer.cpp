#include "ExampleLayer.hpp"
#include "lib/Util.hpp"
#include "webgpu/webgpu_cpp.h"
#include <ranges>
#include <vector>

namespace wglib::compute {
ExampleLayer::ExampleLayer(uint64_t numItems, float multiplier)
    : m_numItems(numItems), m_multiplier(multiplier) {
  initItems();
}

auto ExampleLayer::initItems() -> void {
  m_items = std::move(std::ranges::views::iota(0uz, m_numItems) |
                      std::ranges::to<std::vector<float>>());
}

auto ExampleLayer::initBuffers(wgpu::Device &device) -> void {
  m_initalBuffer = util::createBuffer < float,
  wgpu::BufferUsage::Storage |
      wgpu::BufferUsage::CopyDst > (device, m_items.size());
  m_ResultBuffer = util::createBuffer < float,
  wgpu::BufferUsage::Storage |
      wgpu::BufferUsage::CopySrc > (device, m_items.size());
  m_stagingBuffer = util::createBuffer < float,
  wgpu::BufferUsage::MapRead |
      wgpu::BufferUsage::CopyDst > (device, m_items.size());
  m_multiplierBuffer = util::createBuffer < Uniforms,
  wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst > (device, 1);
}

auto ExampleLayer::initComputePipeline(wgpu::Device &device) -> void {
  const auto shaderModule =
      util::createShaderModuleFromFile("../src/shaders/example.wgsl", device);
  const wgpu::ComputePipelineDescriptor desc{
      .compute = {.module = shaderModule}};
  m_computePipeline = device.CreateComputePipeline(&desc);
}

auto ExampleLayer::initBindGroup(wgpu::Device &device) -> void {
  wgpu::BindGroupEntry entries[3]{{.binding = 0, .buffer = m_multiplierBuffer},
                                  {.binding = 1, .buffer = m_initalBuffer},
                                  {.binding = 2, .buffer = m_ResultBuffer}};
  wgpu::BindGroupDescriptor desc{.entryCount = 3,
                                 .entries = entries,
                                 .layout =
                                     m_computePipeline.GetBindGroupLayout(0)};
  m_bindGroup = device.CreateBindGroup(&desc);
}

auto ExampleLayer::InitImpl(wgpu::Device &device) -> void {
  if (init)
    return;
  initBuffers(device);
  initComputePipeline(device);
  initBindGroup(device);
  init = true;
}

auto ExampleLayer::ComputeImpl(wgpu::CommandEncoder &e, wgpu::Queue &queue)
    -> void {
  // Upload input data to buffer
  queue.WriteBuffer(m_initalBuffer, 0, m_items.data(),
                    m_items.size() * sizeof(float));

  // Upload multiplier to uniform buffer
  Uniforms uniforms{m_multiplier};
  queue.WriteBuffer(m_multiplierBuffer, 0, &uniforms, sizeof(Uniforms));

  auto computePass = e.BeginComputePass();
  computePass.SetPipeline(m_computePipeline);
  computePass.SetBindGroup(0, m_bindGroup);
  computePass.DispatchWorkgroups(util::divCeil(m_items.size(), 64uz));
  computePass.End();

  // Copy result buffer to staging buffer for CPU readback
  e.CopyBufferToBuffer(m_ResultBuffer, 0, m_stagingBuffer, 0,
                       m_items.size() * sizeof(float));

  const auto commandBuffer = e.Finish();
  queue.Submit(1, &commandBuffer);

  m_stagingBuffer.MapAsync(
      wgpu::MapMode::Read, 0, m_stagingBuffer.GetSize(),
      wgpu::CallbackMode::AllowSpontaneous,
      [&](wgpu::MapAsyncStatus status, wgpu::StringView error) {
        if (status == wgpu::MapAsyncStatus::Success) {
          m_resultsReady = true;
          util::log("Done mapping buffer, ready to be read");
        } else {
          util::log("Failed to map result buffer: {}", error.data);
        }
      });
}

auto ExampleLayer::getResultImpl() -> const void * {
  if (m_resultsReady) {

    return m_stagingBuffer.GetConstMappedRange(0);

  } else {
    util::log("Results not ready");
    return nullptr;
  }
}
} // namespace wglib::compute
