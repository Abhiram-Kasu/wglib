#pragma once
#include "GLFW/glfw3.h"
#include "glm/ext/vector_float2.hpp"

namespace wglib
{
class InputManager
{
  private:
    GLFWwindow *m_window_handle;

  public:
    InputManager(GLFWwindow *window) : m_window_handle(window)
    {
    }

    auto get_cursor_pos() const noexcept -> glm::vec2;

    enum class MouseButton : int
    {
        Right = GLFW_MOUSE_BUTTON_RIGHT,
        Left = GLFW_MOUSE_BUTTON_LEFT
    };

    auto get_cursor_down(MouseButton mouse_button) const noexcept -> bool;
    auto get_cursor_up(MouseButton mouse_button) const noexcept -> bool;
};

} // namespace wglib
