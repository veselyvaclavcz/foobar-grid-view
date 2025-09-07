#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <atlbase.h>
#include <vector>
#include <memory>

struct GridItem;
enum class GridSize;
struct PluginConfig;

struct RenderContext {
    ID2D1HwndRenderTarget* render_target;
    IDWriteFactory* dwrite_factory;
    IDWriteTextFormat* text_format_normal;
    IDWriteTextFormat* text_format_small;
    ID2D1SolidColorBrush* text_brush;
    ID2D1SolidColorBrush* background_brush;
    ID2D1SolidColorBrush* selection_brush;
    ID2D1SolidColorBrush* hover_brush;
    ID2D1SolidColorBrush* border_brush;
    ID2D1Bitmap* default_art_bitmap;
    
    RenderContext() {
        render_target = nullptr;
        dwrite_factory = nullptr;
        text_format_normal = nullptr;
        text_format_small = nullptr;
        text_brush = nullptr;
        background_brush = nullptr;
        selection_brush = nullptr;
        hover_brush = nullptr;
        border_brush = nullptr;
        default_art_bitmap = nullptr;
    }
};

struct GridLayout {
    int item_width;
    int item_height;
    int items_per_row;
    int total_rows;
    int visible_rows;
    int art_size;
    int text_height;
    int spacing;
    RECT client_rect;
    
    GridLayout() {
        item_width = 0;
        item_height = 0;
        items_per_row = 0;
        total_rows = 0;
        visible_rows = 0;
        art_size = 0;
        text_height = 0;
        spacing = 8;
        SetRectEmpty(&client_rect);
    }
};

class GridRenderer {
public:
    GridRenderer();
    ~GridRenderer();
    
    // Initialization
    bool Initialize(ID2D1HwndRenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void Cleanup();
    void UpdateTheme(bool dark_theme);
    
    // Layout calculations
    void CalculateLayout(const RECT& client_rect, GridSize grid_size, int item_count, GridLayout& layout);
    RECT GetItemRect(const GridLayout& layout, int item_index, int scroll_pos);
    int GetItemAtPoint(const GridLayout& layout, POINT pt, int scroll_pos);
    
    // Rendering
    void BeginRender();
    void EndRender();
    void RenderBackground(const GridLayout& layout);
    void RenderItem(const GridLayout& layout, const GridItem& item, int item_index, 
                   int scroll_pos, bool selected, bool hovered, bool playing);
    void RenderScrollbar(const GridLayout& layout, int scroll_pos, int total_height);
    void RenderLoadingIndicator(const RECT& item_rect);
    void RenderNoResults(const GridLayout& layout, const char* message);
    
private:
    // Private methods
    bool CreateBrushes();
    bool CreateTextFormats();
    ID2D1Bitmap* CreateDefaultArtBitmap(int size);
    ID2D1Bitmap* BitmapFromHBITMAP(HBITMAP hbitmap);
    void DrawTextWithShadow(const wchar_t* text, const RECT& rect, 
                           ID2D1SolidColorBrush* brush, IDWriteTextFormat* format);
    D2D1_COLOR_F ColorFromRGB(BYTE r, BYTE g, BYTE b, float alpha = 1.0f);
    
    // Render context
    RenderContext m_context;
    bool m_dark_theme;
    bool m_initialized;
    
    // Animation state
    DWORD m_last_render_time;
    float m_loading_rotation;
    
    // Colors (Dark theme)
    static const D2D1_COLOR_F COLOR_BACKGROUND_DARK;
    static const D2D1_COLOR_F COLOR_TEXT_PRIMARY_DARK;
    static const D2D1_COLOR_F COLOR_TEXT_SECONDARY_DARK;
    static const D2D1_COLOR_F COLOR_SELECTION_DARK;
    static const D2D1_COLOR_F COLOR_HOVER_DARK;
    static const D2D1_COLOR_F COLOR_BORDER_DARK;
    
    // Colors (Light theme)
    static const D2D1_COLOR_F COLOR_BACKGROUND_LIGHT;
    static const D2D1_COLOR_F COLOR_TEXT_PRIMARY_LIGHT;
    static const D2D1_COLOR_F COLOR_TEXT_SECONDARY_LIGHT;
    static const D2D1_COLOR_F COLOR_SELECTION_LIGHT;
    static const D2D1_COLOR_F COLOR_HOVER_LIGHT;
    static const D2D1_COLOR_F COLOR_BORDER_LIGHT;
};

// Animation utilities
class AnimationHelper {
public:
    static float EaseOutCubic(float t);
    static float EaseInOutCubic(float t);
    static D2D1_COLOR_F InterpolateColor(const D2D1_COLOR_F& from, const D2D1_COLOR_F& to, float t);
    static float InterpolateFloat(float from, float to, float t);
};

// Geometry utilities
namespace GeometryUtils {
    D2D1_RECT_F RectToD2DRect(const RECT& rect);
    RECT D2DRectToRect(const D2D1_RECT_F& rect);
    bool RectIntersects(const RECT& a, const RECT& b);
    RECT InflateRect(const RECT& rect, int dx, int dy);
    POINT RectCenter(const RECT& rect);
}