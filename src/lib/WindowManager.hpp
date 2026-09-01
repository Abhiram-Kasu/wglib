#pragma once

#include "glm/detail/qualifier.hpp"
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/matrix_uint2x2_sized.hpp>
#include <glm/glm.hpp>
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
    // framebuffer (physical) size in pixels
    glm::vec<2, uint32_t> m_window_size;
    // logical size the app uses (in logical units)
    glm::vec<2, uint32_t> m_logical_size;

    // 3x3 affine transform: logical -> physical (framebuffer)
    glm::mat3 m_transform{};
    // inverse: physical -> logical
    glm::mat3 m_inverse_transform{};

    std::string_view m_title;
    GLFWwindow *m_window = nullptr;
    wgpu::Surface m_surface;
    wgpu::TextureFormat m_format;

    // stored device/adapter so surface can be reconfigured on resize
    wgpu::Device m_device;
    wgpu::Adapter m_adapter;

    // Aspect ratio locking
    bool m_aspect_locked = false;
    uint32_t m_aspect_numer = 0;
    uint32_t m_aspect_denom = 0;

    auto configureSurface() -> void;
    auto recomputeTransforms() -> void;

  public:
    auto SetAspectRatio(uint32_t numer, uint32_t denom) -> void;
    auto IsAspectLocked() const -> bool;

  private:

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

    // Transform getters
    auto GetLogicalToPhysical() const -> glm::mat3;
    auto GetPhysicalToLogical() const -> glm::mat3;

    // Convenience mapping helpers
    auto PhysicalToLogical(const glm::vec2 &p) const -> glm::vec2;
    auto LogicalToPhysical(const glm::vec2 &l) const -> glm::vec2;
};
} // namespace wglib
