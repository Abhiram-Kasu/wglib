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
    : m_title(title), m_window_size({width, height}), m_logical_size({width, height}), m_device(device),
      m_adapter(adapter)
{

#ifndef __EMSCRIPTEN__
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);

    // Lock aspect ratio to the logical size using GLFW when available
    SetAspectRatio(m_logical_size.x, m_logical_size.y);

    m_surface = wgpu::glfw::CreateSurfaceForWindow(instance, m_window);
    if (!m_surface)
    {
        util::log("Failed to create surface");
        exit(0);
    }
    // configure surface with initial sizes; recompute transforms afterwards
    configureSurface();
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

    // Keep aspect ratio metadata on web; the canvas sizing is controlled in the shell HTML/JS.
    SetAspectRatio(m_logical_size.x, m_logical_size.y);

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
    configureSurface();
#endif

    // recompute transforms using actual framebuffer size
    recomputeTransforms();

    glfwSetWindowUserPointer(m_window, this);
    // listen for window size changes
    glfwSetWindowSizeCallback(m_window, onWindowSizeChanged);
}

auto WindowManager::configureSurface() -> void
{
    wgpu::SurfaceCapabilities capabilities;
    m_surface.GetCapabilities(m_adapter, &capabilities);
    m_format = capabilities.formats[0];
#ifndef __EMSCRIPTEN__

    util::log("Using format: {}", m_format);
#else
    util::log("Using format (Emscripten)");
#endif

    wgpu::SurfaceConfiguration config{
        .device = m_device, .format = m_format, .width = m_window_size.x, .height = m_window_size.y};
    m_surface.Configure(&config);
}

auto WindowManager::recomputeTransforms() -> void
{
    if (!m_window)
        return;

    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(m_window, &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0)
        return;

    m_window_size = {static_cast<uint32_t>(fbw), static_cast<uint32_t>(fbh)};

    // scale to fit while preserving aspect
    const auto sw = static_cast<float>(fbw) / static_cast<float>(m_logical_size.x);
    const auto sh = static_cast<float>(fbh) / static_cast<float>(m_logical_size.y);
    const auto s = std::min(sw, sh);

    const auto ox = (static_cast<float>(fbw) - static_cast<float>(m_logical_size.x) * s) * 0.5f;
    const auto oy = (static_cast<float>(fbh) - static_cast<float>(m_logical_size.y) * s) * 0.5f;

    // Construct matrix in homogeneous coordinates (column-major):
    // [ s 0 tx ]
    // [ 0 s ty ]
    // [ 0 0  1 ]
    glm::mat3 S(1.0f);
    S[0][0] = s;
    S[1][1] = s;

    glm::mat3 T(1.0f);
    T[2][0] = ox; // tx
    T[2][1] = oy; // ty

    m_transform = T * S;
    m_inverse_transform = glm::inverse(m_transform);

    // Reconfigure surface with new framebuffer size
    configureSurface();
}

auto WindowManager::SetAspectRatio(uint32_t numer, uint32_t denom) -> void
{
    if (denom == 0)
        return;

    m_aspect_locked = true;
    m_aspect_numer = numer;
    m_aspect_denom = denom;

#ifndef __EMSCRIPTEN__
    if (m_window)
    {
        glfwSetWindowAspectRatio(m_window, static_cast<int>(m_aspect_numer), static_cast<int>(m_aspect_denom));
    }
#endif
}

auto WindowManager::IsAspectLocked() const -> bool
{
    return m_aspect_locked;
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

auto WindowManager::GetLogicalToPhysical() const -> glm::mat3
{
    return m_transform;
}

auto WindowManager::GetPhysicalToLogical() const -> glm::mat3
{
    return m_inverse_transform;
}

auto WindowManager::PhysicalToLogical(const glm::vec2 &p) const -> glm::vec2
{
    glm::vec3 v{p.x, p.y, 1.0f};
    glm::vec3 r = m_inverse_transform * v;
    return glm::vec2{r.x, r.y};
}

auto WindowManager::LogicalToPhysical(const glm::vec2 &l) const -> glm::vec2
{
    glm::vec3 v{l.x, l.y, 1.0f};
    glm::vec3 r = m_transform * v;
    return glm::vec2{r.x, r.y};
}

auto WindowManager::onWindowSizeChanged(GLFWwindow *window, int width, int height) -> void
{
    util::log("New Window Size: {} {}", width, height);
    if (auto *instance = static_cast<WindowManager *>(glfwGetWindowUserPointer(window)))
    {
        // Recompute using the framebuffer size (handles HiDPI scaling)
        instance->recomputeTransforms();
    }
    else
    {
        util::log("ERROR: unable to get WindowUserPointer from GLFWwindow");
    }
}
} // namespace wglib
