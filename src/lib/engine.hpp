#pragma once
#include "lib/render_layer/render_layer.hpp"
#include "window_manager.hpp"
#include <concepts>
#include <cstdint>
#include <memory>
#include <string_view>
#include <webgpu/webgpu_cpp.h>

#include "glm/vec2.hpp"
#include "renderer.hpp"

namespace wglib {
  class Engine {
    std::unique_ptr<WindowManager> m_window_manager;
    wgpu::Instance m_instance;
    wgpu::Device m_device;
    wgpu::Adapter m_adapter;
    glm::vec2 m_window_size;
    std::unique_ptr<Renderer> m_renderer;
    std::function<void(double)> m_update_function;
    double m_last_frame_time{0.0f};

    auto render() -> void;

  public:
    Engine(glm::vec2 size, std::string_view title);

    ~Engine();

    auto Start() -> void;

    auto OnUpdate(std::function<void(float)> &&function) -> void {
      m_update_function = function;
    }

    auto Draw(std::derived_from<render_layers::RenderLayer> auto &renderLayer)
      -> void {
      m_renderer->pushRenderLayer(renderLayer);
    }
  };
} // namespace wglib