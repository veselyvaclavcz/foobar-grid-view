#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <functional>
#include <set>

namespace albumart_grid {

// Handles keyboard and mouse input for the grid
class InputHandler {
public:
    using SelectionCallback = std::function<void(int index, bool ctrl, bool shift)>;
    using ScrollCallback = std::function<void(int delta)>;
    using DoubleClickCallback = std::function<void(int index)>;
    using ContextMenuCallback = std::function<void(int x, int y, int index)>;
    using KeyboardCallback = std::function<void(UINT key, bool ctrl, bool shift, bool alt)>;
    
    InputHandler();
    ~InputHandler();
    
    // Set callbacks
    void set_selection_callback(SelectionCallback cb) { m_selection_cb = cb; }
    void set_scroll_callback(ScrollCallback cb) { m_scroll_cb = cb; }
    void set_double_click_callback(DoubleClickCallback cb) { m_double_click_cb = cb; }
    void set_context_menu_callback(ContextMenuCallback cb) { m_context_menu_cb = cb; }
    void set_keyboard_callback(KeyboardCallback cb) { m_keyboard_cb = cb; }
    
    // Mouse events
    void on_mouse_move(int x, int y);
    void on_mouse_down(int x, int y, bool left, bool right);
    void on_mouse_up(int x, int y, bool left, bool right);
    void on_mouse_double_click(int x, int y);
    void on_mouse_wheel(int delta);
    void on_mouse_leave();
    
    // Keyboard events
    void on_key_down(UINT key);
    void on_key_up(UINT key);
    void on_char(WCHAR ch);
    
    // Get current mouse position
    POINT get_mouse_pos() const { return m_mouse_pos; }
    
    // Check if dragging
    bool is_dragging() const { return m_is_dragging; }
    
    // Check modifier keys
    bool is_ctrl_down() const { return m_ctrl_down; }
    bool is_shift_down() const { return m_shift_down; }
    bool is_alt_down() const { return m_alt_down; }
    
private:
    void update_modifiers();
    void start_drag(int x, int y);
    void update_drag(int x, int y);
    void end_drag(int x, int y);
    
private:
    SelectionCallback m_selection_cb;
    ScrollCallback m_scroll_cb;
    DoubleClickCallback m_double_click_cb;
    ContextMenuCallback m_context_menu_cb;
    KeyboardCallback m_keyboard_cb;
    
    POINT m_mouse_pos;
    POINT m_drag_start;
    bool m_is_dragging;
    bool m_left_down;
    bool m_right_down;
    
    bool m_ctrl_down;
    bool m_shift_down;
    bool m_alt_down;
    
    DWORD m_last_click_time;
    POINT m_last_click_pos;
    
    std::set<UINT> m_pressed_keys;
};

} // namespace albumart_grid