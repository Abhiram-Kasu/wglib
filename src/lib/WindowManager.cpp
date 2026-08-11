#include "WindowManager.hpp"

#include "CoreUtil.hpp"
#include "GLFW/glfw3.h"
#include <iostream>

#ifndef __EMSCRIPTEN__
#include "webgpu/webgpu_glfw.h"
#endif
#ifndef __EMSCRIPTEN__
#include <webgpu/webgpu_cpp_print.h>
#endif

namespace wglib
{

WindowManager::WindowManager(uint32_t width, uint32_t height, std::string_view title, wgpu::Instance &instance,
                             wgpu::Device &device, wgpu::Adapter &adapter)
    : m_title(title), m_window_size({width, height})
{

#ifndef __EMSCRIPTEN__
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    m_surface = wgpu::glfw::CreateSurfaceForWindow(instance, m_window);
    if (!m_surface)
    {
        util::log("Failed to create surface");
        exit(0);
    }
    configureSurface(device, adapter);
#else
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (!m_window)
    {
        throw std::runtime_error("Failed to create GLFW window");
    }

    // For Emscripten, we get the WGPU surface from the canvas
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
    canvasDesc.selector = "#canvas";

    wgpu::SurfaceDescriptor surfaceDesc{};
    surfaceDesc.nextInChain = &canvasDesc;
    m_surface = instance.CreateSurface(&surfaceDesc);

    if (!m_surface)
    {
        util::log("Failed to create surface from canvas");
        exit(0);
    }
    configureSurface(device, adapter);
#endif
    glfwSetWindowUserPointer(m_window, this);
    // listen for window size changes
    glfwSetWindowSizeCallback(m_window, onWindowSizeChanged);
}

auto WindowManager::configureSurface(wgpu::Device &device, wgpu::Adapter &adapter) -> void
{
    wgpu::SurfaceCapabilities capabilities;
    m_surface.GetCapabilities(adapter, &capabilities);
    m_format = capabilities.formats[0];
#ifndef __EMSCRIPTEN__

    util::log("Using format: {}", m_format);
#else
    util::log("Using format (Emscripten)");
#endif

    wgpu::SurfaceConfiguration config{
        .device = device, .format = m_format, .width = m_window_size.x, .height = m_window_size.y};
    m_surface.Configure(&config);
}

auto WindowManager::title() const -> std::string_view
{
    return m_title;
}

auto WindowManager::surface() const -> const wgpu::Surface &
{
    return m_surface;
}

auto WindowManager::window() const -> GLFWwindow *
{
    return m_window;
}

auto WindowManager::format() const -> wgpu::TextureFormat
{
    return m_format;
}

auto WindowManager::onWindowSizeChanged(GLFWwindow *window, int width, int height) -> void
{
    util::log("New Window Size: {} {}", width, height);
    if (auto *instance = static_cast<WindowManager *>(glfwGetWindowUserPointer(window)))
    {
        instance->m_window_size = {width, height};
    }
    else
    {
        util::log("ERROR: unable to get WindowUserPointer from GLFWwindow");
    }
}
} // namespace wglib
