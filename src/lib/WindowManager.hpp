#pragma once

#include <cstdint>
#include <glm/common.hpp>
#include <string_view>
#include <webgpu/webgpu_cpp.h>

#ifndef __EMSCRIPTEN__
#include "GLFW/glfw3.h"
#else
// Forward declaration for GLFWwindow when using Emscripten
#endif
struct GLFWwindow;

namespace wglib
{
class WindowManager
{
  private:
    glm::vec<2, uint32_t> m_window_size;
    std::string_view m_title;
    GLFWwindow *m_window = nullptr;
    wgpu::Surface m_surface;
    wgpu::TextureFormat m_format;

    auto configureSurface(wgpu::Device &device, wgpu::Adapter &adapter) -> void;

    static auto onClick(int, int, int) -> void;
    auto onMouseMove() -> void;

    static auto onWindowSizeChanged(GLFWwindow *window, int width, int height) -> void;

  public:
    WindowManager(uint32_t width, uint32_t height, std::string_view title, wgpu::Instance &instance,
                  wgpu::Device &device, wgpu::Adapter &adapter);

    auto title() const -> std::string_view;

    auto surface() const -> const wgpu::Surface &;

    auto window() const -> GLFWwindow *;

    auto format() const -> wgpu::TextureFormat;
};
} // namespace wglib
