#include "window.hpp"

namespace vke {

Window::~Window() {};

u32 Window::width() const { return m_width; }
u32 Window::height() const { return m_height; }
} // namespace vke
