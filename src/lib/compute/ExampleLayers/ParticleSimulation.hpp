
#include "glm/vec2.hpp"
#include <span>
#include "glm/vec4.hpp"
#include "lib/CoreInput.hpp"
#include "lib/compute/ComputeLayer.hpp"
#include "webgpu/webgpu_cpp.h"
#include <atomic>
namespace wglib::compute {
class ParticleSimulationLayer
    : public ComputeLayer<std::optional<wgpu::Texture>> {
  struct alignas(8) Particle {
    glm::vec2 velocity; // 8 bytes at offset 0
    glm::vec2 position; // 8 bytes at offset 8
    float radius;       // 4 bytes at offset 16
    float
        _padding; // 4 bytes padding to make struct 24 bytes (WGSL array stride)
  };

  struct alignas(16) CircleUniforms {
    glm::vec4 color;
    glm::uvec2 size;
    float dt;
    float gravity;
    float damping;
    float forceAmp;
    float decayLength;
    uint32_t ballCount;
  };

  struct alignas(16) TouchActionUniforms {
    glm::vec2 touchPosition;
    float radius;
    float touchPower;
    bool operator ==(const TouchActionUniforms&) const = default;
  };

private:
  static constexpr auto kBufferMultiplier = 1.5f;
  uint32_t m_numBalls, m_circleRadius;
  glm::vec2 m_size;
  glm::vec4 m_ballColor;
  glm::vec2 m_startLocation;
  std::vector<Particle> m_initalParticles;
  wgpu::ComputePipeline m_computePipeline;
  wgpu::BindGroup m_bg1, m_bg2;
  std::atomic<bool> readyFlag{false};
  bool m_initialized{false};
  bool m_using_buffer_1{true};

  wgpu::Buffer m_circleUniformBuffer, m_circleBuffer1, m_circleBuffer2, m_touchActionUniformsBuffer;
  wgpu::Texture m_drawTexture;

  CircleUniforms m_uniforms;
  TouchActionUniforms m_touchUniforms;
  bool m_touchUniformsDirty{false};
  bool m_uniformsDirty{false};
  size_t m_bufferSize;

  static auto genParticlesInSquareFormation(uint32_t numBalls, glm::vec2 size,
                                            glm::vec2 start, uint32_t numPerRow,
                                            float ballRadius)
      -> std::vector<Particle>;
  auto createAndSetBindGroups(const wgpu::Device& device) -> void;
  auto spawnMoreParticlesAt(glm::vec2 location, size_t num) -> std::vector<Particle>;
  auto runLogic(const InputManager& manager, wgpu::CommandEncoder& encoder, const wgpu::Device& device) -> void;
  auto updateUniforms(wgpu::Queue& encoder) -> void; 
  auto reallocBuffers(const wgpu::Device& device, const wgpu::CommandEncoder& commandEncoder,
                      const std::span<Particle> newData) -> void;
  

public:
  ParticleSimulationLayer(uint32_t numBalls, glm::vec2 size,
                          uint32_t circleRadius, glm::vec4 ballColor,
                          glm::vec2 startLocation, uint32_t numPerRow, float dt,
                          float gravity, float damping, float forceAmp,
                          float decayLength);

protected:
  virtual auto getResultImpl() -> std::optional<wgpu::Texture>;
  virtual auto InitImpl(wgpu::Device &) -> void;
  virtual auto ComputeImpl(wgpu::CommandEncoder &e, wgpu::Queue &, Engine& engine) -> void;
};
} // namespace wglib::compute
