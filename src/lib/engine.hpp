#pragma once
#include "window_manager.hpp"
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
    std::function<void(float)> m_update_function;
    float m_last_frame_time{0.0f};


    auto render() -> void;

  public:
    Engine(glm::vec2 size, std::string_view title);


    ~Engine();

    auto Start() -> void;

    auto OnUpdate(std::function<void(float)> &&function) -> void {
      m_update_function = function;
    }


    template<typename L, typename... Args>
      requires std::constructible_from<L, Args...>
    auto Draw(Args &&... args) -> void {
      m_renderer->pushRenderLayer<L>(std::forward<Args>(args)...);
    }
  };
} // namespace wglib