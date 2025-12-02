#pragma once
#include "window_manager.hpp"
#include <cstdint>
#include <memory>
#include <string_view>
#include <webgpu/webgpu_cpp.h>

#include "glm/vec2.hpp"
#include "renderer.hpp"

namespace wglib {

struct Uniforms {
  glm::vec2 dimensions;
};
class Engine {
private:
  std::unique_ptr<WindowManager> m_window_manager;
  wgpu::Instance m_instance;
  wgpu::Device m_device;
  wgpu::Adapter m_adapter;
  glm::vec2 m_window_size;
  std::unique_ptr<Renderer> m_renderer;

  auto render() -> void;

public:
  Engine(glm::vec2 size, std::string_view title);

  template <typename L, typename... Args>
    requires std::constructible_from<L, Args...>
  auto Draw(Args &&...args) -> void {
    m_renderer->pushRenderLayer<L>(std::forward<Args>(args)...);
  }

  ~Engine();

  auto Start() -> void;
};

} // namespace wglib
