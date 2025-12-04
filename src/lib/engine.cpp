#include "engine.hpp"

#include <iostream>
#include <memory>
#include <webgpu/webgpu_cpp_print.h>

#include "renderer.hpp"
#include "webgpu/webgpu_cpp.h"

namespace wglib {
  Engine::Engine(glm::vec2 size, std::string_view title) : m_window_size(size) {
    constexpr auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    const wgpu::InstanceDescriptor instanceDesc{
      .requiredFeatureCount = 1, .requiredFeatures = &kTimedWaitAny
    };
    m_instance = wgpu::CreateInstance(&instanceDesc);

    const auto f1 = m_instance.RequestAdapter(
      nullptr, wgpu::CallbackMode::WaitAnyOnly,
      [&](wgpu::RequestAdapterStatus status, wgpu::Adapter a,
          wgpu::StringView message) {
        if (status != wgpu::RequestAdapterStatus::Success) {
          std::cout << "RequestAdapter: " << message << "\n";
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
      std::cout << "Error: " << errorType << " - message: " << message << "\n";
    });

    wgpu::Future f2 = m_adapter.RequestDevice(
      &desc, wgpu::CallbackMode::WaitAnyOnly,
      [&](wgpu::RequestDeviceStatus status, wgpu::Device d,
          wgpu::StringView message) {
        if (status != wgpu::RequestDeviceStatus::Success) {
          std::cout << "RequestDevice: " << message << "\n";
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
    emscripten_set_main_loop(render, 0, false);
#else
    while (!glfwWindowShouldClose(m_window_manager->window())) {
      m_last_frame_time = glfwGetTime() - m_last_frame_time;
      glfwPollEvents();
      render();
      m_instance.ProcessEvents();
      if (m_update_function) {
        m_update_function(m_last_frame_time);
      }
    }
#endif
  }

  auto Engine::render() -> void {
    wgpu::SurfaceTexture surfaceTexture;
    m_window_manager->surface().GetCurrentTexture(&surfaceTexture);
    m_renderer->Render(surfaceTexture);
    m_window_manager->surface().Present();
  }


  Engine::~Engine() {
  }
} // namespace wglib