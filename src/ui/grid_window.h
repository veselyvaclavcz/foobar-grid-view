#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../foundation/lifecycle_manager.h"
#include "../core/album_data_model.h"
#include <Windows.h>
#include <gdiplus.h>
#include <memory>
#include <vector>
#include <atomic>

namespace albumart_grid {

// Grid layout configuration
struct GridLayout {
    int columns = 0;           // 0 = auto-fill
    int thumbnail_size = 150;  // Size of thumbnails
    int spacing = 5;           // Space between items
    int text_height = 40;      // Height for text area
    bool auto_fill = true;     // Auto-fill mode
};

// Grid window class
class GridWindow : public ILifecycleAware {
public:
    GridWindow();
    ~GridWindow();
    
    // ILifecycleAware implementation
    void on_initialize() override;
    void on_shutdown() override;
    bool is_valid() const override;
    
    // Window management
    HWND create(HWND parent);
    void destroy();
    HWND get_hwnd() const { return m_hwnd; }
    
    // Data model
    void set_model(std::shared_ptr<AlbumDataModel> model);
    
    // Layout configuration
    void set_columns(int count);
    void set_thumbnail_size(int size);
    void set_auto_fill(bool enable);
    GridLayout get_layout() const { return m_layout; }
    
    // Selection
    metadb_handle_list get_selection() const;
    void clear_selection();
    
    // Scrolling
    void scroll_to_item(size_t index, bool center = false);
    void scroll_to_now_playing();
    
    // Refresh
    void invalidate_all();
    void invalidate_item(size_t index);
    
protected:
    // Window procedure
    static LRESULT CALLBACK window_proc_static(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT window_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
    
    // Message handlers
    void on_create();
    void on_destroy();
    void on_paint(HDC dc);
    void on_size(int width, int height);
    void on_vscroll(int code, int pos);
    void on_mouse_wheel(int delta, int keys, int x, int y);
    void on_mouse_move(int x, int y, int keys);
    void on_lbutton_down(int x, int y, int keys);
    void on_lbutton_up(int x, int y, int keys);
    void on_lbutton_dblclk(int x, int y, int keys);
    void on_rbutton_down(int x, int y, int keys);
    void on_key_down(UINT key, int repeat, int flags);
    void on_key_up(UINT key, int repeat, int flags);
    void on_char(UINT ch, int repeat, int flags);
    void on_context_menu(int x, int y);
    void on_timer(UINT_PTR id);
    
private:
    // Layout calculations
    void calculate_layout();
    void update_scroll_info();
    int calculate_columns() const;
    int calculate_total_height() const;
    
    // Drawing
    void draw_background(Gdiplus::Graphics& graphics, const Gdiplus::Rect& rect);
    void draw_item(Gdiplus::Graphics& graphics, size_t index, const Gdiplus::Rect& rect);
    void draw_thumbnail(Gdiplus::Graphics& graphics, size_t index, const Gdiplus::Rect& rect);
    void draw_text(Gdiplus::Graphics& graphics, const album_item& item, const Gdiplus::Rect& rect);
    void draw_selection(Gdiplus::Graphics& graphics, const Gdiplus::Rect& rect);
    void draw_now_playing(Gdiplus::Graphics& graphics, const Gdiplus::Rect& rect);
    
    // Hit testing
    int hit_test(int x, int y) const;
    Gdiplus::Rect get_item_rect(size_t index) const;
    bool is_item_visible(size_t index) const;
    
    // Selection management
    void select_item(size_t index, bool toggle, bool range);
    void select_all();
    void handle_keyboard_selection(UINT key);
    
    // Mouse tracking
    void track_mouse_leave();
    void update_hover(int index);
    
    // Context menu
    void show_context_menu(int x, int y);
    
    // Double-click action
    void handle_double_click(size_t index);
    
    // Data model callback
    void on_model_changed();
    
private:
    // Window handle
    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    
    // Layout
    GridLayout m_layout;
    int m_client_width = 0;
    int m_client_height = 0;
    int m_scroll_y = 0;
    int m_total_height = 0;
    int m_visible_rows = 0;
    int m_items_per_row = 1;
    
    // Data model
    std::shared_ptr<AlbumDataModel> m_model;
    
    // Selection state
    std::vector<bool> m_selection;
    size_t m_focus_index = SIZE_MAX;
    size_t m_anchor_index = SIZE_MAX;
    
    // Mouse state
    int m_hover_index = -1;
    bool m_tracking_mouse = false;
    bool m_dragging = false;
    POINT m_drag_start = {0, 0};
    
    // Drawing resources
    std::unique_ptr<Gdiplus::Bitmap> m_back_buffer;
    Gdiplus::Font* m_font = nullptr;
    Gdiplus::Font* m_font_bold = nullptr;
    
    // Colors (will be updated based on theme)
    Gdiplus::Color m_color_background{255, 30, 30, 30};
    Gdiplus::Color m_color_text{255, 200, 200, 200};
    Gdiplus::Color m_color_selection{100, 0, 120, 215};
    Gdiplus::Color m_color_hover{50, 128, 128, 128};
    Gdiplus::Color m_color_now_playing{255, 0, 120, 215};
    
    // State
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_shutting_down{false};
    
    // Timers
    static constexpr UINT_PTR TIMER_REFRESH = 1;
    static constexpr UINT_PTR TIMER_SCROLL = 2;
};

} // namespace albumart_grid