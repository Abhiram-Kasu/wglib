#pragma once
#include "lib/CoreUtil.hpp"
#include "lib/compute/ComputeLayer.hpp"
#include "webgpu/webgpu_cpp.h"
#include <atomic>
#include <ranges>
#include <span>
#include <vector>

namespace wglib::compute {

template <size_t numItems>
class ExampleLayer
    : public ComputeLayer<std::optional<std::span<const float, numItems>>> {

  struct alignas(16) Uniforms {
    float multiplier;
  };

private:
  wgpu::Buffer m_initalBuffer, m_multiplierBuffer, m_ResultBuffer,
      m_stagingBuffer;
  wgpu::ComputePipeline m_computePipeline;
  wgpu::BindGroup m_bindGroup;
  uint64_t m_numItems = numItems;
  float m_multiplier;

  std::vector<float> m_items;
  std::atomic<uint8_t> m_resultsReady;

  auto initBuffers(wgpu::Device &) -> void;
  bool init{false};
  auto initBindGroup(wgpu::Device &) -> void;
  auto initComputePipeline(wgpu::Device &) -> void;
  auto initItems() -> void;

public:
  ExampleLayer(float multiplier) : m_multiplier(multiplier) { initItems(); }

  ~ExampleLayer() = default;

protected:
  auto getResultImpl()
      -> std::optional<std::span<const float, numItems>> override {
    if (m_resultsReady) {

      auto *float_ptr =
          static_cast<const float *>(m_stagingBuffer.GetConstMappedRange());

      return std::span<const float, numItems>(float_ptr, numItems);

    } else {
      util::log("Results not ready");
      return std::nullopt;
    }
  }
  auto InitImpl(wgpu::Device &) -> void override;
  auto ComputeImpl(wgpu::CommandEncoder &e, wgpu::Queue &) -> void override;
};

template <size_t numItems> auto ExampleLayer<numItems>::initItems() -> void {
  m_items = std::move(std::ranges::views::iota(0uz, m_numItems) |
                      std::ranges::to<std::vector<float>>());
}

template <size_t numItems>
auto ExampleLayer<numItems>::initBuffers(wgpu::Device &device) -> void {
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

template <size_t numItems>
auto ExampleLayer<numItems>::initComputePipeline(wgpu::Device &device) -> void {
  const auto shaderModule =
      util::createShaderModuleFromFile("../src/shaders/example.wgsl", device);
  const wgpu::ComputePipelineDescriptor desc{
      .compute = {.module = shaderModule}};
  m_computePipeline = device.CreateComputePipeline(&desc);
}

template <size_t numItems>
auto ExampleLayer<numItems>::initBindGroup(wgpu::Device &device) -> void {
  wgpu::BindGroupEntry entries[3]{{.binding = 0, .buffer = m_multiplierBuffer},
                                  {.binding = 1, .buffer = m_initalBuffer},
                                  {.binding = 2, .buffer = m_ResultBuffer}};
  wgpu::BindGroupDescriptor desc{.layout =
                                     m_computePipeline.GetBindGroupLayout(0),
                                 .entryCount = 3,
                                 .entries = entries};
  m_bindGroup = device.CreateBindGroup(&desc);
}

template <size_t numItems>
auto ExampleLayer<numItems>::InitImpl(wgpu::Device &device) -> void {
  if (init)
    return;
  initBuffers(device);
  initComputePipeline(device);
  initBindGroup(device);
  init = true;
}

template <size_t numItems>
auto ExampleLayer<numItems>::ComputeImpl(wgpu::CommandEncoder &e,
                                         wgpu::Queue &queue) -> void {
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

} // namespace wglib::compute
