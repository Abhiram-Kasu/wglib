#include "WindowManager.hpp"

#include "Util.hpp"
#include <iostream>

#ifndef __EMSCRIPTEN__
#include "GLFW/glfw3.h"
#include "webgpu/webgpu_glfw.h"
#endif
#ifndef __EMSCRIPTEN__
#include <webgpu/webgpu_cpp_print.h>
#endif

namespace wglib {
WindowManager::WindowManager(uint32_t width, uint32_t height,
                             std::string_view title, wgpu::Instance &instance,
                             wgpu::Device &device, wgpu::Adapter &adapter)
    : m_width(width), m_height(height), m_title(title) {
#ifndef __EMSCRIPTEN__
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  m_window = glfwCreateWindow(width, height, "WebGPU window", nullptr, nullptr);
  m_surface = wgpu::glfw::CreateSurfaceForWindow(instance, m_window);
  if (!m_surface) {
    util::log("Failed to create surface");
    exit(0);
  }
  configureSurface(device, adapter);
#else
  // For Emscripten, we get the surface from the canvas
  wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
  canvasDesc.selector = "#canvas";

  wgpu::SurfaceDescriptor surfaceDesc{};
  surfaceDesc.nextInChain = &canvasDesc;
  m_surface = instance.CreateSurface(&surfaceDesc);

  if (!m_surface) {
    util::log("Failed to create surface from canvas");
    exit(0);
  }
  configureSurface(device, adapter);
#endif
}

auto WindowManager::configureSurface(wgpu::Device &device,
                                     wgpu::Adapter &adapter) -> void {
  wgpu::SurfaceCapabilities capabilities;
  m_surface.GetCapabilities(adapter, &capabilities);
  m_format = capabilities.formats[0];
#ifndef __EMSCRIPTEN__

  util::log("Using format: {}", m_format);
#else
  util::log("Using format (Emscripten)");
#endif

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

auto WindowManager::window() const -> GLFWwindow * {
#ifndef __EMSCRIPTEN__
  return m_window;
#else
  return nullptr;
#endif
}

auto WindowManager::format() const -> wgpu::TextureFormat { return m_format; }
} // namespace wglib
