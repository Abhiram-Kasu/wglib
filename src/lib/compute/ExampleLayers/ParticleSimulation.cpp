#include "ParticleSimulation.hpp"
#include "glm/ext/vector_float2.hpp"
#include "lib/CoreUtil.hpp"
#include "webgpu/webgpu_cpp.h"
#include <atomic>
#include <random>
#include <utility>
#include <vector>

namespace wglib::compute {

ParticleSimulationLayer::ParticleSimulationLayer(
    uint32_t numBalls, glm::vec2 size, uint32_t circleRadius,
    glm::vec4 ballColor, glm::vec2 startLocation, uint32_t numPerRow, float dt,
    float gravity, float damping, float forceAmp, float decayLength)
    : m_numBalls(numBalls), m_size(size), m_circleRadius(circleRadius),
      m_ballColor(ballColor), m_startLocation(startLocation),
      m_initalParticles(genParticlesInSquareFormation(
          m_numBalls, m_size, m_startLocation, numPerRow, circleRadius)),
      m_uniforms{ballColor, size, dt, gravity, damping, forceAmp, decayLength} {

}

auto ParticleSimulationLayer::genParticlesInSquareFormation(
    uint32_t numBalls, glm::vec2 size, glm::vec2 start, uint32_t numPerRow,
    float ballRadius) -> std::vector<Particle> {
  const auto ballSize = ballRadius * 2;
  const auto totalLength = numPerRow * ballSize;
  assert(start.x + totalLength <= size.x && "balls per row would exceed size");

  const auto numRows = util::divCeil<uint32_t>(numBalls, numPerRow);
  const auto totalHeight = numRows * ballSize;
  assert(start.y + totalHeight <= size.y &&
         "too many rows, would exceed the size");

  std::vector<Particle> particles{};
  particles.reserve(numBalls);
  const auto offset = glm::vec2{ballRadius};

  for (uint32_t row = 0; row < numRows; row++) {
    for (uint32_t col = 0; col < numPerRow; col++) {
      if (particles.size() >= numBalls) {
        return particles;
      }
      auto coordinates =
          start + glm::vec2{col * ballSize, row * ballSize} + offset;
      particles.emplace_back(glm::vec2{0}, coordinates, ballRadius);
    }
  }
  return particles;
}

auto ParticleSimulationLayer::InitImpl(wgpu::Device &device) -> void {
  if (m_initialized) {
    return;
  }
  m_initialized = true;

  m_circleUniformBuffer = util::createBuffer < CircleUniforms,
  wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopySrc > (device, 1, true);

  m_circleUniformBuffer.WriteMappedRange(0, &m_uniforms, sizeof(m_uniforms));
  m_circleUniformBuffer.Unmap();

  m_circleBuffer1 = util::createBuffer < Particle,
  wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc |
      wgpu::BufferUsage::CopyDst > (device, m_numBalls, true);
  m_circleBuffer1.WriteMappedRange(0, m_initalParticles.data(),
                                   sizeof(Particle) * m_initalParticles.size());
  m_circleBuffer1.Unmap();

  m_circleBuffer2 = util::createBuffer<Particle, wgpu::BufferUsage::Storage>(
      device, m_numBalls, true);
  m_circleBuffer2.WriteMappedRange(0, m_initalParticles.data(),
                                   sizeof(Particle) * m_initalParticles.size());
  m_circleBuffer2.Unmap();

  const wgpu::TextureDescriptor textureDesc{
      .size = {static_cast<uint32_t>(m_size.x),
               static_cast<uint32_t>(m_size.y)},
      .format = wgpu::TextureFormat::RGBA8Unorm,
      .usage = wgpu::TextureUsage::RenderAttachment |
               wgpu::TextureUsage::StorageBinding |
               wgpu::TextureUsage::TextureBinding |
               wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst,
  };
  m_drawTexture = device.CreateTexture(&textureDesc);

  const wgpu::ComputePipelineDescriptor desc{
      .compute = {
          .module = util::createShaderModuleFromFile(
              "../src/shaders/ParticleSimulation/particle.wgsl", device)}};
  m_computePipeline = device.CreateComputePipeline(&desc);
  wgpu::BindGroupEntry entriesSet1[4]{
      {.binding = 0,
       .buffer = m_circleBuffer1,
       .size = sizeof(Particle) * m_numBalls},
      {.binding = 1,
       .buffer = m_circleBuffer2,
       .size = sizeof(Particle) * m_numBalls},
      {.binding = 2,
       .buffer = m_circleUniformBuffer,
       .size = sizeof(CircleUniforms)},
      {.binding = 3, .textureView = m_drawTexture.CreateView()},
  };
  wgpu::BindGroupEntry entriesSet2[4]{
      {.binding = 0,
       .buffer = m_circleBuffer2,
       .size = sizeof(Particle) * m_numBalls},
      {.binding = 1,
       .buffer = m_circleBuffer1,
       .size = sizeof(Particle) * m_numBalls},
      {.binding = 2,
       .buffer = m_circleUniformBuffer,
       .size = sizeof(CircleUniforms)},
      {.binding = 3, .textureView = m_drawTexture.CreateView()},
  };
  wgpu::BindGroupDescriptor bgDesc1{
      .entries = entriesSet1,
      .entryCount = 4,
      .layout = m_computePipeline.GetBindGroupLayout(0)};
  wgpu::BindGroupDescriptor bgDesc2{
      .entries = entriesSet2,
      .entryCount = 4,
      .layout = m_computePipeline.GetBindGroupLayout(0)};

  m_bg1 = device.CreateBindGroup(&bgDesc1);
  m_bg2 = device.CreateBindGroup(&bgDesc2);

  readyFlag.store(true, std::memory_order_release);
}
auto ParticleSimulationLayer::getResultImpl() -> const void * {
  if (readyFlag.load(std::memory_order_acquire)) {
    return &m_drawTexture;
  } else {
    return nullptr;
  }
}
auto ParticleSimulationLayer::ComputeImpl(wgpu::CommandEncoder &encoder,
                                          wgpu::Queue &queue) -> void {

  // Clear the texture using a render pass
  wgpu::RenderPassColorAttachment colorAttachment{
      .view = m_drawTexture.CreateView(),
      .loadOp = wgpu::LoadOp::Clear,
      .storeOp = wgpu::StoreOp::Store,
      .clearValue = {0.0, 0.0, 0.0, 1.0}, // Clear to black
  };

  wgpu::RenderPassDescriptor renderPassDesc{
      .colorAttachmentCount = 1,
      .colorAttachments = &colorAttachment,
  };

  auto renderPass = encoder.BeginRenderPass(&renderPassDesc);
  renderPass.End();

  // Run compute pass to simulate physics and draw particles
  const auto computePass = encoder.BeginComputePass();
  computePass.SetBindGroup(0, m_bg1);
  computePass.SetPipeline(m_computePipeline);
  computePass.DispatchWorkgroups(util::divCeil<uint32_t>(m_numBalls, 64));
  computePass.End();

  const auto commandBuffer = encoder.Finish();
  queue.Submit(1, &commandBuffer);

  std::swap(m_bg1, m_bg2);
}
} // namespace wglib::compute
