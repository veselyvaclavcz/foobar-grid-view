#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <gdiplus.h>
#include <memory>
#include <string>
#include <vector>

namespace albumart_grid {

class AlbumDataModel;
class ThumbnailCache;

// Handles all rendering for the grid
class GridRenderer {
public:
    struct RenderConfig {
        int thumbnail_size = 150;
        int padding = 5;
        bool show_labels = true;
        bool show_play_count = true;
        COLORREF background_color = RGB(30, 30, 30);
        COLORREF text_color = RGB(255, 255, 255);
        COLORREF selection_color = RGB(100, 100, 200);
        COLORREF hover_color = RGB(70, 70, 70);
    };
    
    struct GridItem {
        std::string key;
        std::string title;
        std::string subtitle;
        int play_count = 0;
        bool is_selected = false;
        bool is_hovered = false;
        bool is_playing = false;
        Gdiplus::Bitmap* thumbnail = nullptr;
    };
    
    GridRenderer();
    ~GridRenderer();
    
    // Set configuration
    void set_config(const RenderConfig& config);
    
    // Main render function
    void render(HDC hdc, const RECT& client_rect,
               AlbumDataModel* model,
               ThumbnailCache* cache,
               int scroll_offset);
    
    // Get item at position
    int get_item_at_pos(int x, int y, int scroll_offset) const;
    
    // Get visible range
    std::pair<int, int> get_visible_range(int scroll_offset, int view_height) const;
    
    // Calculate total height
    int calculate_total_height(int item_count, int view_width) const;
    
private:
    void render_background(Gdiplus::Graphics& g, const RECT& rect);
    void render_grid_items(Gdiplus::Graphics& g, const RECT& rect,
                          const std::vector<GridItem>& items,
                          int scroll_offset);
    void render_item(Gdiplus::Graphics& g, const GridItem& item,
                    const Gdiplus::Rect& item_rect);
    void render_thumbnail(Gdiplus::Graphics& g, Gdiplus::Bitmap* bmp,
                         const Gdiplus::Rect& rect);
    void render_text(Gdiplus::Graphics& g, const std::string& text,
                    const Gdiplus::Rect& rect, bool is_title);
    
    int calculate_columns(int view_width) const;
    Gdiplus::Rect get_item_rect(int index, int columns, int scroll_offset) const;
    
private:
    RenderConfig m_config;
    std::unique_ptr<Gdiplus::Font> m_font_title;
    std::unique_ptr<Gdiplus::Font> m_font_subtitle;
    std::unique_ptr<Gdiplus::SolidBrush> m_brush_bg;
    std::unique_ptr<Gdiplus::SolidBrush> m_brush_text;
    std::unique_ptr<Gdiplus::SolidBrush> m_brush_selection;
    std::unique_ptr<Gdiplus::SolidBrush> m_brush_hover;
    
    mutable int m_last_columns = 0;
    mutable int m_last_view_width = 0;
};

} // namespace albumart_grid