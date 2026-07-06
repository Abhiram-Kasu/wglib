#include "CoreInput.hpp"
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
    auto point = glm::vec<2, double>{};
    glfwGetCursorPos(m_window_handle, &point.x, &point.y);
    return point;
}

auto InputManager::get_cursor_down(MouseButton mouse_button) const noexcept -> bool
{
    return glfwGetMouseButton(m_window_handle, static_cast<int>(mouse_button)) == GLFW_PRESS;
}

} // namespace wglib
