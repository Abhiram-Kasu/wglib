#include "Engine.hpp"

#include <iostream>
#include <memory>
#ifndef __EMSCRIPTEN__
#include <webgpu/webgpu_cpp_print.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "Renderer.hpp"
#include "webgpu/webgpu_cpp.h"

namespace wglib {
Engine::Engine(glm::vec2 size, std::string_view title) : m_window_size(size) {
  constexpr auto K_TIMED_WAIT_ANY = wgpu::InstanceFeatureName::TimedWaitAny;
  const wgpu::InstanceDescriptor instanceDesc{
      .requiredFeatureCount = 1, .requiredFeatures = &K_TIMED_WAIT_ANY};
  m_instance = wgpu::CreateInstance(&instanceDesc);

  const auto f1 = m_instance.RequestAdapter(
      nullptr, wgpu::CallbackMode::WaitAnyOnly,
      [&](wgpu::RequestAdapterStatus status, wgpu::Adapter a,
          wgpu::StringView message) {
        if (status != wgpu::RequestAdapterStatus::Success) {
          std::cout << "RequestAdapter: "
                    << std::string(message.data, message.length) << "\n";
          exit(0);
        }
        m_adapter = std::move(a);
      });
  if (const auto res = m_instance.WaitAny(f1, UINT64_MAX);
      res != wgpu::WaitStatus::Success) {
    std::cout << "Failed to wait for adapter request" << std::endl;
    exit(0);
  };

  wgpu::DeviceDescriptor desc{};
  desc.SetUncapturedErrorCallback([](const wgpu::Device &,
                                     wgpu::ErrorType errorType,
                                     wgpu::StringView message) {
    std::cout << "Error: " << static_cast<int>(errorType)
              << " - message: " << std::string(message.data, message.length)
              << "\n";
  });

  wgpu::Future f2 = m_adapter.RequestDevice(
      &desc, wgpu::CallbackMode::WaitAnyOnly,
      [&](wgpu::RequestDeviceStatus status, wgpu::Device d,
          wgpu::StringView message) {
        if (status != wgpu::RequestDeviceStatus::Success) {
          std::cout << "RequestDevice: "
                    << std::string(message.data, message.length) << "\n";
          exit(0);
        }
        m_device = std::move(d);
      });
  if (const auto res = m_instance.WaitAny(f2, UINT64_MAX);
      res != wgpu::WaitStatus::Success) {
    ;
    std::cout << "Failed to wait for device request" << std::endl;
    exit(0);
  };

  // Create Window
  this->m_window_manager = std::make_unique<WindowManager>(
      size.x, size.y, title, m_instance, m_device, m_adapter);

  // Pass to Renderer

  this->m_renderer = std::make_unique<Renderer>(
      m_instance, m_adapter, m_device, m_window_manager->format(), size);
}

auto Engine::Start() -> void {
#if defined(__EMSCRIPTEN__)
  // For Emscripten, we need a lambda wrapper since render() is a member
  // function
  emscripten_set_main_loop_arg(
      [](void *arg) {
        auto *engine = static_cast<Engine *>(arg);
        engine->render();
        engine->m_instance.ProcessEvents();
        if (engine->m_update_function) {
          static double last_time = emscripten_get_now() / 1000.0;
          double current_time = emscripten_get_now() / 1000.0;
          double delta = current_time - last_time;
          last_time = current_time;
          engine->m_update_function(delta);
        }
      },
      this, 0, true);
#else
  while (!glfwWindowShouldClose(m_window_manager->window())) {
    const auto currTime = glfwGetTime();
    const auto delta = currTime - m_last_frame_time;
    m_last_frame_time = currTime;
    glfwPollEvents();
    render();
    m_instance.ProcessEvents();
    if (m_update_function) {
      m_update_function(delta);
    }
  }
#endif
}

auto Engine::render() -> void {
  wgpu::SurfaceTexture surfaceTexture;
  m_window_manager->surface().GetCurrentTexture(&surfaceTexture);
  m_renderer->Render(surfaceTexture);
#ifndef __EMSCRIPTEN__
  // Emscripten handles presentation automatically via requestAnimationFrame
  m_window_manager->surface().Present();
#endif
}

Engine::~Engine() {}
} // namespace wglib
