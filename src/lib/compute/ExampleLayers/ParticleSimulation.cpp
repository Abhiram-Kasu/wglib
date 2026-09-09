#include "ParticleSimulation.hpp"
#include "glm/ext/vector_float2.hpp"
#include "lib/CoreEngine.hpp"
#include "lib/CoreInput.hpp"
#include "lib/CoreUtil.hpp"
#include "webgpu/webgpu_cpp.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <utility>
#include <vector>

namespace wglib::compute
{

ParticleSimulationLayer::ParticleSimulationLayer(uint32_t numBalls, glm::vec2 size, uint32_t circleRadius,
                                                 glm::vec4 ballColor, glm::vec2 startLocation, uint32_t numPerRow,
                                                 float dt, float gravity, float damping, float forceAmp,
                                                 float decayLength)
    : m_numBalls(numBalls), m_size(size), m_circleRadius(circleRadius), m_ballColor(ballColor),
      m_startLocation(startLocation),
      m_initalParticles(genParticlesInSquareFormation(numBalls, m_size, m_startLocation, numPerRow, circleRadius)),
      m_uniforms{ballColor, size, dt, gravity, damping, forceAmp, decayLength, numBalls},
      m_bufferSize{sizeof(Particle) * numBalls}
{
}

auto ParticleSimulationLayer::genParticlesInSquareFormation(uint32_t numBalls, glm::vec2 size, glm::vec2 start,
                                                            uint32_t numPerRow, float ballRadius)
    -> std::vector<Particle>
{
    const auto ballSize = ballRadius * 2;
    const auto totalLength = numPerRow * ballSize;
    assert(start.x + totalLength <= size.x && "balls per row would exceed size");

    const auto numRows = util::divCeil<uint32_t>(numBalls, numPerRow);
    const auto totalHeight = numRows * ballSize;
    assert(start.y + totalHeight <= size.y && "too many rows, would exceed the size");

    std::vector<Particle> particles{};
    particles.reserve(numBalls);
    const auto offset = glm::vec2{ballRadius};

    for (uint32_t row = 0; row < numRows; row++)
    {
        for (uint32_t col = 0; col < numPerRow; col++)
        {
            if (particles.size() >= numBalls)
            {
                return particles;
            }
            auto coordinates = start + glm::vec2{col * ballSize, row * ballSize} + offset;
            particles.emplace_back(glm::vec2{0}, coordinates, ballRadius);
        }
    }
    return particles;
}

auto ParticleSimulationLayer::InitImpl(wgpu::Device &device) -> void
{
    if (m_initialized)
    {
        return;
    }
    m_initialized = true;

    m_touchActionUniformsBuffer =
        util::createBuffer<TouchActionUniforms, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform>(device, 1,
                                                                                                         true);

    m_touchActionUniformsBuffer.WriteMappedRange(0, &m_touchUniforms, sizeof(TouchActionUniforms));
    m_touchActionUniformsBuffer.Unmap();

    m_circleUniformBuffer =
        util::createBuffer<CircleUniforms, wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst>(device, 1, true);

    m_circleUniformBuffer.WriteMappedRange(0, &m_uniforms, sizeof(m_uniforms));
    m_circleUniformBuffer.Unmap();

    const auto bufferSize = sizeof(Particle) * m_initalParticles.size();
    m_circleBuffer1 = util::createBuffer<Particle, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc |
                                                       wgpu::BufferUsage::CopyDst>(device, m_uniforms.ballCount, true);
    m_circleBuffer1.WriteMappedRange(0, m_initalParticles.data(), bufferSize);
    m_circleBuffer1.Unmap();

    m_circleBuffer2 = util::createBuffer<Particle, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc |
                                                       wgpu::BufferUsage::CopyDst>(device, m_uniforms.ballCount, true);
    m_circleBuffer2.WriteMappedRange(0, m_initalParticles.data(), bufferSize);
    m_circleBuffer2.Unmap();

    m_bufferSize = bufferSize;

    const wgpu::TextureDescriptor textureDesc{
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::StorageBinding |
                 wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst,
        .size = {static_cast<uint32_t>(m_size.x), static_cast<uint32_t>(m_size.y)},
        .format = wgpu::TextureFormat::RGBA8Unorm,
    };
    m_drawTexture = device.CreateTexture(&textureDesc);

    const wgpu::ComputePipelineDescriptor desc{
        .compute = {.module =
                        util::createShaderModuleFromFile("../src/shaders/ParticleSimulation/particle.wgsl", device)}};
    m_computePipeline = device.CreateComputePipeline(&desc);
    createAndSetBindGroups(device);

    readyFlag.store(true, std::memory_order_release);
}

auto ParticleSimulationLayer::createAndSetBindGroups(const wgpu::Device &device) -> void
{
    wgpu::BindGroupEntry entriesSet1[]{
        {.binding = 0, .buffer = m_circleBuffer1, .size = sizeof(Particle) * m_uniforms.ballCount},
        {.binding = 1, .buffer = m_circleBuffer2, .size = sizeof(Particle) * m_uniforms.ballCount},
        {.binding = 2, .buffer = m_circleUniformBuffer, .size = sizeof(CircleUniforms)},
        {.binding = 3, .textureView = m_drawTexture.CreateView()},
        {.binding = 4, .buffer = m_touchActionUniformsBuffer, .size = sizeof(TouchActionUniforms)},
    };
    wgpu::BindGroupEntry entriesSet2[]{
        {.binding = 0, .buffer = m_circleBuffer2, .size = sizeof(Particle) * m_uniforms.ballCount},
        {.binding = 1, .buffer = m_circleBuffer1, .size = sizeof(Particle) * m_uniforms.ballCount},
        {.binding = 2, .buffer = m_circleUniformBuffer, .size = sizeof(CircleUniforms)},
        {.binding = 3, .textureView = m_drawTexture.CreateView()},
        {.binding = 4, .buffer = m_touchActionUniformsBuffer, .size = sizeof(TouchActionUniforms)},
    };

    constexpr auto kEntryCount = sizeof(entriesSet1) / sizeof(wgpu::BindGroupEntry);
    wgpu::BindGroupDescriptor bgDesc1{
        .layout = m_computePipeline.GetBindGroupLayout(0), .entryCount = kEntryCount, .entries = entriesSet1};
    wgpu::BindGroupDescriptor bgDesc2{
        .layout = m_computePipeline.GetBindGroupLayout(0), .entryCount = kEntryCount, .entries = entriesSet2};

    m_bg1 = device.CreateBindGroup(&bgDesc1);
    m_bg2 = device.CreateBindGroup(&bgDesc2);

    readyFlag.store(true, std::memory_order_release);
}
auto ParticleSimulationLayer::getResultImpl() -> std::optional<wgpu::Texture>
{
    if (readyFlag.load(std::memory_order_acquire))
    {
        return m_drawTexture;
    }
    else
    {
        return std::nullopt;
    }
}
auto ParticleSimulationLayer::ComputeImpl(wgpu::CommandEncoder &encoder, wgpu::Queue &queue, Engine &engine) -> void
{

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
    runLogic(engine.Input(), encoder, engine.GetDevice());
    updateUniforms(queue);
    const auto computePass = encoder.BeginComputePass();

    computePass.SetBindGroup(0, m_bg1);
    computePass.SetPipeline(m_computePipeline);
    computePass.DispatchWorkgroups(util::divCeil<uint32_t>(m_uniforms.ballCount, 64));
    computePass.End();

    const auto commandBuffer = encoder.Finish();
    queue.Submit(1, &commandBuffer);

    std::swap(m_bg1, m_bg2);
    m_using_buffer_1 = not m_using_buffer_1;
}
auto ParticleSimulationLayer::updateUniforms(wgpu::Queue &queue) -> void
{
    if (m_touchUniformsDirty)
    {
        queue.WriteBuffer(m_touchActionUniformsBuffer, 0, &m_touchUniforms, sizeof(TouchActionUniforms));
        m_touchUniformsDirty = false;
        util::log("[Particle] Updated Touch Uniforms");
    }

    if (m_uniformsDirty)
    {
        queue.WriteBuffer(m_circleUniformBuffer, 0, &m_uniforms, sizeof(m_uniforms));
        m_uniformsDirty = false;
        util::log("[Particle] Updated Uniforms");
    }
}

auto ParticleSimulationLayer::spawnMoreParticlesAt(glm::vec2 location, size_t num) -> std::vector<Particle>
{
    std::vector<Particle> particles;
    particles.reserve(num);
    if (num == 0)
    {
        return particles;
    }

    const auto cols = std::max<uint32_t>(1u, static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<float>(num)))));
    const auto rows = util::divCeil<uint32_t>(static_cast<uint32_t>(num), cols);
    const auto spacing = static_cast<float>(m_circleRadius * 2u);

    for (uint32_t row = 0; row < rows; ++row)
    {
        for (uint32_t col = 0; col < cols; ++col)
        {
            if (particles.size() >= num)
            {
                return particles;
            }

            particles.emplace_back(glm::vec2{0.0f}, location + glm::vec2{col * spacing, row * spacing},
                                   static_cast<float>(m_circleRadius));
        }
    }

    return particles;
}

auto ParticleSimulationLayer::reallocBuffers(const wgpu::Device &device, const wgpu::CommandEncoder &commandEncoder,
                                             const std::span<Particle> newData) -> void
{
    const auto oldBallCount = m_uniforms.ballCount;
    const auto newBallCount = oldBallCount + static_cast<uint32_t>(newData.size());
    const auto newSize = newBallCount * sizeof(Particle);
    const auto &currentBuffer = m_using_buffer_1 ? m_circleBuffer1 : m_circleBuffer2;
    const auto &nextBuffer = m_using_buffer_1 ? m_circleBuffer2 : m_circleBuffer1;

    if (newSize <= m_bufferSize)
    {
        util::log("[ParticleSim] No need to alloc, skipping alloc and writing to buffer");

        const auto tempBuffer =
            util::createBuffer<Particle, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc |
                                             wgpu::BufferUsage::CopyDst>(device, newData.size(), true);
        tempBuffer.WriteMappedRange(0, newData.data(), newData.size() * sizeof(Particle));

        tempBuffer.Unmap();

        commandEncoder.CopyBufferToBuffer(currentBuffer, 0, nextBuffer, 0, oldBallCount * sizeof(Particle));
        commandEncoder.CopyBufferToBuffer(tempBuffer, 0, currentBuffer, oldBallCount * sizeof(Particle),
                                          newData.size() * sizeof(Particle));
        commandEncoder.CopyBufferToBuffer(tempBuffer, 0, nextBuffer, oldBallCount * sizeof(Particle),
                                          newData.size() * sizeof(Particle));

        m_uniforms.ballCount = newBallCount;
        m_uniformsDirty = true;
        return;
    }

    const auto allocatedBallCount = [newBallCount, oldBallCount]() {
        auto count = std::max<uint32_t>(1, oldBallCount);
        while (count < newBallCount)
        {
            count = std::max(count + 1, static_cast<uint32_t>(count * kBufferMultiplier));
        }
        return count;
    }();
    const auto allocatedSize = allocatedBallCount * sizeof(Particle);
    util::log("[ParticleSim] Allocating new buffer of size {}", allocatedSize);

    auto newBuffer1 = util::createBuffer<Particle, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc |
                                                       wgpu::BufferUsage::CopyDst>(device, allocatedBallCount, true);
    auto newBuffer2 = util::createBuffer<Particle, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc |
                                                       wgpu::BufferUsage::CopyDst>(device, allocatedBallCount, false);

    m_bufferSize = allocatedSize;
    newBuffer1.WriteMappedRange(0, newData.data(), newData.size() * sizeof(Particle));

    newBuffer1.Unmap();

    commandEncoder.CopyBufferToBuffer(currentBuffer, 0, newBuffer1, newData.size() * sizeof(Particle),
                                      oldBallCount * sizeof(Particle));

    commandEncoder.CopyBufferToBuffer(newBuffer1, 0, newBuffer2, 0, newSize);

    m_circleBuffer1 = newBuffer1;
    m_circleBuffer2 = newBuffer2;

    m_uniforms.ballCount = newBallCount;
    m_uniformsDirty = true;
}

auto ParticleSimulationLayer::runLogic(const InputManager &manager, wgpu::CommandEncoder &encoder,
                                       const wgpu::Device &device) -> void
{
    if (manager.get_cursor_down(InputManager::MouseButton::Left))
    {

        constexpr auto kNumBallsToSpawn = 50uz;
        const auto mouseCoords = manager.get_cursor_pos();
        auto spawnedParticles = spawnMoreParticlesAt(mouseCoords, kNumBallsToSpawn);
        if (spawnedParticles.empty())
        {
            return;
        }

        reallocBuffers(device, encoder, spawnedParticles);

        createAndSetBindGroups(device);
    }
    else if (manager.get_cursor_down(InputManager::MouseButton::Right))
    {
        // apply touch uniform here
        constexpr auto kTouchPower = 10000.0f;
        constexpr auto kRadius = 200.0f;
        // only update if needed:
        auto newTouchUniforms = TouchActionUniforms{
            .touchPosition = manager.get_cursor_pos(), .radius = kRadius, .touchPower = kTouchPower};
        if (newTouchUniforms != m_touchUniforms)
        {
            m_touchUniforms = newTouchUniforms;
            m_touchUniformsDirty = true;
        }

        util::log("Adding Touch Power to particle sim");
    }
    else if (manager.get_cursor_up(InputManager::MouseButton::Right))
    {
        m_touchUniforms.touchPower = 0;
        m_touchUniformsDirty = true;
    }
}

} // namespace wglib::compute
