#include "CoreInput.hpp"
#include "WindowManager.hpp"
#include "GLFW/glfw3.h"
#include "glm/detail/qualifier.hpp"
#include "glm/ext/vector_float2.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace wglib
{

auto InputManager::get_cursor_pos() const noexcept -> glm::vec2
{
    // Get cursor in window coordinates (double)
    double wx = 0.0, wy = 0.0;
    glfwGetCursorPos(m_window_handle, &wx, &wy);

    // Convert window coords to framebuffer (physical) coords to handle HiDPI
    int winW = 0, winH = 0;
    int fbW = 0, fbH = 0;
    glfwGetWindowSize(m_window_handle, &winW, &winH);
    glfwGetFramebufferSize(m_window_handle, &fbW, &fbH);

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (winW > 0 && winH > 0)
    {
        scaleX = static_cast<float>(fbW) / static_cast<float>(winW);
        scaleY = static_cast<float>(fbH) / static_cast<float>(winH);
    }

    const glm::vec2 physical{static_cast<float>(wx * scaleX), static_cast<float>(wy * scaleY)};

    // If WindowManager is available, map physical framebuffer coordinates to logical coordinates
    if (auto *wm = static_cast<WindowManager *>(glfwGetWindowUserPointer(m_window_handle)))
    {
        return wm->PhysicalToLogical(physical);
    }

    // Fallback: return physical coords as float
    return physical;
}

auto InputManager::get_cursor_down(MouseButton mouse_button) const noexcept -> bool
{
    return glfwGetMouseButton(m_window_handle, static_cast<int>(mouse_button)) == GLFW_PRESS;
}

} // namespace wglib
