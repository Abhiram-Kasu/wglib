#include "WindowManager.hpp"

#include <iostream>

#include "GLFW/glfw3.h"
#include "webgpu/webgpu_glfw.h"
#include <webgpu/webgpu_cpp_print.h>

namespace wglib {
WindowManager::WindowManager(uint32_t width, uint32_t height,
                             std::string_view title, wgpu::Instance &instance,
                             wgpu::Device &device, wgpu::Adapter &adapter)
    : m_width(width), m_height(height), m_title(title) {
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  m_window = glfwCreateWindow(width, height, "WebGPU window", nullptr, nullptr);
  m_surface = wgpu::glfw::CreateSurfaceForWindow(instance, m_window);
  if (!m_surface) {
    std::cout << "Failed to create surface" << std::endl;
    exit(0);
  }
  configureSurface(device, adapter);
}

auto WindowManager::configureSurface(wgpu::Device &device,
                                     wgpu::Adapter &adapter) -> void {
  wgpu::SurfaceCapabilities capabilities;
  m_surface.GetCapabilities(adapter, &capabilities);
  m_format = capabilities.formats[0];

  std::cout << "Using format: " << m_format << std::endl;

  wgpu::SurfaceConfiguration config{.device = device,
                                    .format = m_format,
                                    .width = m_width,
                                    .height = m_height};
  m_surface.Configure(&config);
}

auto WindowManager::width() const -> uint32_t { return m_width; }

auto WindowManager::height() const -> uint32_t { return m_height; }

auto WindowManager::title() const -> std::string_view { return m_title; }

auto WindowManager::surface() const -> const wgpu::Surface & {
  return m_surface;
}

auto WindowManager::window() const -> GLFWwindow * { return m_window; }

auto WindowManager::format() const -> wgpu::TextureFormat { return m_format; }
} // namespace wglib
