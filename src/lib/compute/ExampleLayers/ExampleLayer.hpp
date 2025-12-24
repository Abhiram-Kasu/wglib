
#include "../ComputeLayer.hpp"
#include "webgpu/webgpu_cpp.h"
#include <atomic>
namespace wglib::compute {
class ExampleLayer : public ComputeLayer {

  struct alignas(16) Uniforms {
    float multiplier;
  };

private:
  wgpu::Buffer m_initalBuffer, m_multiplierBuffer, m_ResultBuffer,
      m_stagingBuffer;
  wgpu::ComputePipeline m_computePipeline;
  wgpu::BindGroup m_bindGroup;
  uint64_t m_numItems;
  float m_multiplier;

  std::vector<float> m_items;
  std::atomic<uint8_t> m_resultsReady;

  auto initBuffers(wgpu::Device &) -> void;
  bool init{false};
  auto initBindGroup(wgpu::Device &) -> void;
  auto initComputePipeline(wgpu::Device &) -> void;
  auto initItems() -> void;

public:
  ExampleLayer(uint64_t numItems, float multiplier);

protected:
  auto getResultImpl() -> const void * override;
  auto InitImpl(wgpu::Device &) -> void override;
  auto ComputeImpl(wgpu::CommandEncoder &e, wgpu::Queue &) -> void override;
};
} // namespace wglib::compute
