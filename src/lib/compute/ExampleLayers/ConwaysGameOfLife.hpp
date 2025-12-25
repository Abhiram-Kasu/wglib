#include "glm/ext/vector_float2.hpp"
#include "lib/compute/ComputeLayer.hpp"
#include "webgpu/webgpu_cpp.h"
#include <vector>
namespace wglib::compute {

class ConwaysGameOfLifeComputeLayer : public ComputeLayer {

  struct alignas(16) Uniform {
    uint32_t width, height;
  };

private:
  glm::vec2 m_size;
  wgpu::Buffer m_firstBuffer, m_secondBuffer, m_uniformBuffer;
  wgpu::Texture m_texture;
  wgpu::TextureView m_textureView;
  wgpu::Buffer *m_currBufferPointer, *m_secBufferPointer;
  wgpu::BindGroup m_bindGroup;

  bool m_init{false};
  std::vector<uint32_t> m_initalData;
  wgpu::ComputePipeline m_computePipeline;

public:
  auto Swap() -> void;
  ConwaysGameOfLifeComputeLayer(glm::vec2);

protected:
  auto getResultImpl() -> const void * override;
  auto InitImpl(wgpu::Device &device) -> void override;
  auto ComputeImpl(wgpu::CommandEncoder &e, wgpu::Queue &q) -> void override;
};

} // namespace wglib::compute
