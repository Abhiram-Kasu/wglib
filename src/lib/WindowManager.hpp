#pragma once

#include <cstdint>
#include <string_view>
#include <webgpu/webgpu_cpp.h>

#ifndef __EMSCRIPTEN__
#include "GLFW/glfw3.h"
#else
// Forward declaration for GLFWwindow when using Emscripten
struct GLFWwindow;
#endif

namespace wglib {
class WindowManager {
private:
  uint32_t m_width, m_height;
  std::string_view m_title;
#ifndef __EMSCRIPTEN__
  GLFWwindow *m_window;
#endif
  wgpu::Surface m_surface;
  wgpu::TextureFormat m_format;

  auto configureSurface(wgpu::Device &device, wgpu::Adapter &adapter) -> void;

public:
  WindowManager(uint32_t width, uint32_t height, std::string_view title,
                wgpu::Instance &instance, wgpu::Device &device,
                wgpu::Adapter &adapter);

  auto width() const -> uint32_t;

  auto height() const -> uint32_t;

  auto title() const -> std::string_view;

  auto surface() const -> const wgpu::Surface &;

  auto window() const -> GLFWwindow *;

  auto format() const -> wgpu::TextureFormat;
};
} // namespace wglib
