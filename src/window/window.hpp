#pragma once

#include "surface.hpp"

#include "../common.hpp"
#include "../fwd.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace vke {

struct MouseInputData {
    float xpos, ypos;
    float delta_x, delta_y; // movement from the last frame
};



class Window : public IRenderTargetSize {
public:
    u32 width() const override { return m_width; }
    u32 height() const override { return m_height; }
    inline bool is_open() const { return m_is_open; }

    inline Surface* surface() { return m_surface.get(); }

    virtual void init_surface() = 0;
    virtual void poll_events() = 0;

    bool is_key_pressed(u32 keycode) { return m_key_states[keycode]; };
    const MouseInputData& get_mouse_input() const { return m_mouse_input; }

    void on_mouse_down(u32 mouse_button, std::function<void()>&& function) { m_mouse_callbacks[mouse_button] = function; }
    void on_key_down(u32 keycode, std::function<void()>&& function) { m_key_callbacks[keycode] = function; }

    virtual void lock_mouse() {};
    virtual void unlock_mouse() {};

    virtual ~Window();

protected:
    u32 m_width = 0, m_height = 0;
    std::string m_title;
    bool m_is_open      = true;
    bool m_mouse_locked = false;

    std::unique_ptr<Surface> m_surface;

    MouseInputData m_mouse_input;
    std::unordered_map<u32, bool> m_key_states;
    std::unordered_map<u32, std::function<void()>> m_key_callbacks;
    std::unordered_map<u32, std::function<void()>> m_mouse_callbacks;
};
} // namespace vke