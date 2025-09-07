#include "../include/grid_renderer.h"
#include "../include/foo_albumart_grid.h"
#include <cmath>

// Color constants (Dark theme)
const D2D1_COLOR_F GridRenderer::COLOR_BACKGROUND_DARK = D2D1::ColorF(0.08f, 0.08f, 0.08f, 1.0f);
const D2D1_COLOR_F GridRenderer::COLOR_TEXT_PRIMARY_DARK = D2D1::ColorF(0.95f, 0.95f, 0.95f, 1.0f);
const D2D1_COLOR_F GridRenderer::COLOR_TEXT_SECONDARY_DARK = D2D1::ColorF(0.7f, 0.7f, 0.7f, 1.0f);
const D2D1_COLOR_F GridRenderer::COLOR_SELECTION_DARK = D2D1::ColorF(0.0f, 0.48f, 1.0f, 0.3f);
const D2D1_COLOR_F GridRenderer::COLOR_HOVER_DARK = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f);
const D2D1_COLOR_F GridRenderer::COLOR_BORDER_DARK = D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f);

// Color constants (Light theme)
const D2D1_COLOR_F GridRenderer::COLOR_BACKGROUND_LIGHT = D2D1::ColorF(0.98f, 0.98f, 0.98f, 1.0f);
const D2D1_COLOR_F GridRenderer::COLOR_TEXT_PRIMARY_LIGHT = D2D1::ColorF(0.1f, 0.1f, 0.1f, 1.0f);
const D2D1_COLOR_F GridRenderer::COLOR_TEXT_SECONDARY_LIGHT = D2D1::ColorF(0.4f, 0.4f, 0.4f, 1.0f);
const D2D1_COLOR_F GridRenderer::COLOR_SELECTION_LIGHT = D2D1::ColorF(0.0f, 0.48f, 1.0f, 0.3f);
const D2D1_COLOR_F GridRenderer::COLOR_HOVER_LIGHT = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.05f);
const D2D1_COLOR_F GridRenderer::COLOR_BORDER_LIGHT = D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f);

GridRenderer::GridRenderer()
    : m_dark_theme(true)
    , m_initialized(false)
    , m_last_render_time(0)
    , m_loading_rotation(0.0f)
{
}

GridRenderer::~GridRenderer() {
    Cleanup();
}

bool GridRenderer::Initialize(ID2D1HwndRenderTarget* render_target, IDWriteFactory* dwrite_factory) {
    if (m_initialized) {
        Cleanup();
    }
    
    m_context.render_target = render_target;
    m_context.dwrite_factory = dwrite_factory;
    
    if (!CreateBrushes() || !CreateTextFormats()) {
        Cleanup();
        return false;
    }
    
    m_context.default_art_bitmap = CreateDefaultArtBitmap(256);
    m_initialized = true;
    return true;
}

void GridRenderer::Cleanup() {
    if (m_context.text_brush) { m_context.text_brush->Release(); m_context.text_brush = nullptr; }
    if (m_context.background_brush) { m_context.background_brush->Release(); m_context.background_brush = nullptr; }
    if (m_context.selection_brush) { m_context.selection_brush->Release(); m_context.selection_brush = nullptr; }
    if (m_context.hover_brush) { m_context.hover_brush->Release(); m_context.hover_brush = nullptr; }
    if (m_context.border_brush) { m_context.border_brush->Release(); m_context.border_brush = nullptr; }
    if (m_context.text_format_normal) { m_context.text_format_normal->Release(); m_context.text_format_normal = nullptr; }
    if (m_context.text_format_small) { m_context.text_format_small->Release(); m_context.text_format_small = nullptr; }
    if (m_context.default_art_bitmap) { m_context.default_art_bitmap->Release(); m_context.default_art_bitmap = nullptr; }
    
    m_initialized = false;
}

void GridRenderer::UpdateTheme(bool dark_theme) {
    if (m_dark_theme == dark_theme) return;
    
    m_dark_theme = dark_theme;
    
    if (m_initialized) {
        // Recreate brushes with new colors
        if (m_context.text_brush) { m_context.text_brush->Release(); m_context.text_brush = nullptr; }
        if (m_context.background_brush) { m_context.background_brush->Release(); m_context.background_brush = nullptr; }
        if (m_context.selection_brush) { m_context.selection_brush->Release(); m_context.selection_brush = nullptr; }
        if (m_context.hover_brush) { m_context.hover_brush->Release(); m_context.hover_brush = nullptr; }
        if (m_context.border_brush) { m_context.border_brush->Release(); m_context.border_brush = nullptr; }
        
        CreateBrushes();
        
        // Recreate default album art
        if (m_context.default_art_bitmap) {
            m_context.default_art_bitmap->Release();
            m_context.default_art_bitmap = CreateDefaultArtBitmap(256);
        }
    }
}

void GridRenderer::CalculateLayout(const RECT& client_rect, GridSize grid_size, int item_count, GridLayout& layout) {
    layout.client_rect = client_rect;
    layout.spacing = 8;
    layout.art_size = (int)grid_size - 16; // Leave room for padding
    layout.text_height = 40;
    layout.item_width = (int)grid_size;
    layout.item_height = (int)grid_size + layout.text_height;
    
    int client_width = client_rect.right - client_rect.left;
    int client_height = client_rect.bottom - client_rect.top;
    
    // Calculate items per row
    layout.items_per_row = max(1, (client_width + layout.spacing) / (layout.item_width + layout.spacing));
    
    // Calculate total rows
    layout.total_rows = item_count > 0 ? (item_count + layout.items_per_row - 1) / layout.items_per_row : 0;
    
    // Calculate visible rows
    layout.visible_rows = (client_height + layout.item_height - 1) / layout.item_height;
}

RECT GridRenderer::GetItemRect(const GridLayout& layout, int item_index, int scroll_pos) {
    int row = item_index / layout.items_per_row;
    int col = item_index % layout.items_per_row;
    
    RECT rect;
    rect.left = col * (layout.item_width + layout.spacing) + layout.spacing;
    rect.top = row * layout.item_height - scroll_pos + layout.spacing;
    rect.right = rect.left + layout.item_width;
    rect.bottom = rect.top + layout.item_height;
    
    return rect;
}

int GridRenderer::GetItemAtPoint(const GridLayout& layout, POINT pt, int scroll_pos) {
    if (layout.items_per_row == 0 || layout.item_height == 0) return -1;
    
    int col = (pt.x - layout.spacing) / (layout.item_width + layout.spacing);
    int row = (pt.y + scroll_pos - layout.spacing) / layout.item_height;
    
    if (col < 0 || col >= layout.items_per_row || row < 0) return -1;
    
    int item_index = row * layout.items_per_row + col;
    return item_index;
}

void GridRenderer::BeginRender() {
    if (!m_context.render_target) return;
    
    m_context.render_target->BeginDraw();
    m_last_render_time = GetTickCount();
}

void GridRenderer::EndRender() {
    if (!m_context.render_target) return;
    
    HRESULT hr = m_context.render_target->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        // Need to recreate resources
        Cleanup();
    }
}

void GridRenderer::RenderBackground(const GridLayout& layout) {
    if (!m_context.render_target || !m_context.background_brush) return;
    
    D2D1_RECT_F rect = GeometryUtils::RectToD2DRect(layout.client_rect);
    m_context.render_target->FillRectangle(&rect, m_context.background_brush);
}

void GridRenderer::RenderItem(const GridLayout& layout, const GridItem& item, int item_index, 
                             int scroll_pos, bool selected, bool hovered, bool playing) {
    if (!m_context.render_target) return;
    
    RECT item_rect = GetItemRect(layout, item_index, scroll_pos);
    
    // Check if item is visible
    if (item_rect.bottom < layout.client_rect.top || item_rect.top > layout.client_rect.bottom) {
        return;
    }
    
    D2D1_RECT_F d2d_rect = GeometryUtils::RectToD2DRect(item_rect);
    
    // Render selection/hover background
    if (selected && m_context.selection_brush) {
        D2D1_ROUNDED_RECT rounded_rect = D2D1::RoundedRect(d2d_rect, 4.0f, 4.0f);
        m_context.render_target->FillRoundedRectangle(&rounded_rect, m_context.selection_brush);
    } else if (hovered && m_context.hover_brush) {
        D2D1_ROUNDED_RECT rounded_rect = D2D1::RoundedRect(d2d_rect, 4.0f, 4.0f);
        m_context.render_target->FillRoundedRectangle(&rounded_rect, m_context.hover_brush);
    }
    
    // Calculate album art area
    RECT art_rect;
    art_rect.left = item_rect.left + 8;
    art_rect.top = item_rect.top + 8;
    art_rect.right = art_rect.left + layout.art_size;
    art_rect.bottom = art_rect.top + layout.art_size;
    
    // Render album art
    ID2D1Bitmap* art_bitmap = nullptr;
    
    if (item.album_art && !item.art_loading) {
        art_bitmap = BitmapFromHBITMAP(item.album_art);
    } else {
        art_bitmap = m_context.default_art_bitmap;
        if (art_bitmap) art_bitmap->AddRef();
    }
    
    if (art_bitmap) {
        D2D1_RECT_F art_d2d_rect = GeometryUtils::RectToD2DRect(art_rect);
        m_context.render_target->DrawBitmap(art_bitmap, &art_d2d_rect, 1.0f,
                                          D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        art_bitmap->Release();
    }
    
    // Show loading indicator if needed
    if (item.art_loading) {
        RenderLoadingIndicator(art_rect);
    }
    
    // Render text overlay
    RECT text_rect;
    text_rect.left = item_rect.left + 4;
    text_rect.top = art_rect.bottom + 4;
    text_rect.right = item_rect.right - 4;
    text_rect.bottom = item_rect.bottom - 4;
    
    // Primary text (album/artist/etc based on grouping)
    pfc::string8 primary_text = item.album;
    pfc::string8 secondary_text = item.artist;
    
    // Convert to wide string
    pfc::stringcvt::string_wide_from_utf8 primary_wide(primary_text.get_ptr());
    pfc::stringcvt::string_wide_from_utf8 secondary_wide(secondary_text.get_ptr());
    
    // Draw text with shadow for better readability
    RECT primary_rect = text_rect;
    primary_rect.bottom = primary_rect.top + 16;
    
    RECT secondary_rect = text_rect;
    secondary_rect.top = primary_rect.bottom + 2;
    secondary_rect.bottom = secondary_rect.top + 14;
    
    if (m_context.text_format_normal) {
        DrawTextWithShadow(primary_wide.get_ptr(), primary_rect, m_context.text_brush, 
                          m_context.text_format_normal);
    }
    
    if (m_context.text_format_small) {
        DrawTextWithShadow(secondary_wide.get_ptr(), secondary_rect, m_context.text_brush, 
                          m_context.text_format_small);
    }
    
    // Draw border for selected items
    if (selected && m_context.border_brush) {
        D2D1_ROUNDED_RECT border_rect = D2D1::RoundedRect(d2d_rect, 4.0f, 4.0f);
        m_context.render_target->DrawRoundedRectangle(&border_rect, m_context.border_brush, 2.0f);
    }
    
    // Draw playing indicator
    if (playing) {
        // Draw small play icon or pulsing effect
        D2D1_ELLIPSE play_indicator = D2D1::Ellipse(
            D2D1::Point2F((float)art_rect.right - 16, (float)art_rect.top + 16), 8.0f, 8.0f);
        
        if (m_context.selection_brush) {
            m_context.render_target->FillEllipse(&play_indicator, m_context.selection_brush);
        }
    }
}

void GridRenderer::RenderScrollbar(const GridLayout& layout, int scroll_pos, int total_height) {
    if (!m_context.render_target || !m_context.border_brush) return;
    
    int client_height = layout.client_rect.bottom - layout.client_rect.top;
    if (total_height <= client_height) return;
    
    // Calculate scrollbar dimensions
    float scrollbar_width = 8.0f;
    float scrollbar_height = (float)client_height;
    float thumb_height = max(20.0f, scrollbar_height * client_height / total_height);
    float thumb_pos = scrollbar_height * scroll_pos / total_height;
    
    // Draw scrollbar track
    D2D1_RECT_F track_rect = D2D1::RectF(
        (float)layout.client_rect.right - scrollbar_width - 2,
        (float)layout.client_rect.top + 2,
        (float)layout.client_rect.right - 2,
        (float)layout.client_rect.bottom - 2
    );
    
    ID2D1SolidColorBrush* track_brush;
    m_context.render_target->CreateSolidColorBrush(
        m_dark_theme ? D2D1::ColorF(0.15f, 0.15f, 0.15f) : D2D1::ColorF(0.85f, 0.85f, 0.85f),
        &track_brush);
    
    if (track_brush) {
        m_context.render_target->FillRectangle(&track_rect, track_brush);
        track_brush->Release();
    }
    
    // Draw scrollbar thumb
    D2D1_RECT_F thumb_rect = D2D1::RectF(
        track_rect.left + 1,
        track_rect.top + thumb_pos,
        track_rect.right - 1,
        track_rect.top + thumb_pos + thumb_height
    );
    
    ID2D1SolidColorBrush* thumb_brush;
    m_context.render_target->CreateSolidColorBrush(
        m_dark_theme ? D2D1::ColorF(0.4f, 0.4f, 0.4f) : D2D1::ColorF(0.6f, 0.6f, 0.6f),
        &thumb_brush);
    
    if (thumb_brush) {
        D2D1_ROUNDED_RECT rounded_thumb = D2D1::RoundedRect(thumb_rect, 4.0f, 4.0f);
        m_context.render_target->FillRoundedRectangle(&rounded_thumb, thumb_brush);
        thumb_brush->Release();
    }
}

void GridRenderer::RenderLoadingIndicator(const RECT& item_rect) {
    if (!m_context.render_target || !m_context.border_brush) return;
    
    // Update rotation for animation
    m_loading_rotation += 5.0f;
    if (m_loading_rotation >= 360.0f) m_loading_rotation -= 360.0f;
    
    // Draw spinning circle
    POINT center = GeometryUtils::RectCenter(item_rect);
    float radius = 12.0f;
    
    // Create rotation transform
    D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F::Rotation(
        m_loading_rotation, D2D1::Point2F((float)center.x, (float)center.y));
    
    m_context.render_target->SetTransform(rotation);
    
    // Draw loading arcs
    for (int i = 0; i < 8; ++i) {
        float angle = (float)i * 45.0f * (3.14159f / 180.0f);
        float alpha = 1.0f - (float)i / 8.0f;
        
        D2D1_POINT_2F start = D2D1::Point2F(
            center.x + cos(angle) * radius * 0.6f,
            center.y + sin(angle) * radius * 0.6f
        );
        
        D2D1_POINT_2F end = D2D1::Point2F(
            center.x + cos(angle) * radius,
            center.y + sin(angle) * radius
        );
        
        ID2D1SolidColorBrush* arc_brush;
        D2D1_COLOR_F color = m_dark_theme ? COLOR_TEXT_PRIMARY_DARK : COLOR_TEXT_PRIMARY_LIGHT;
        color.a = alpha;
        
        m_context.render_target->CreateSolidColorBrush(color, &arc_brush);
        if (arc_brush) {
            m_context.render_target->DrawLine(start, end, arc_brush, 2.0f);
            arc_brush->Release();
        }
    }
    
    // Reset transform
    m_context.render_target->SetTransform(D2D1::Matrix3x2F::Identity());
}

void GridRenderer::RenderNoResults(const GridLayout& layout, const char* message) {
    if (!m_context.render_target || !m_context.text_brush || !m_context.text_format_normal) return;
    
    pfc::stringcvt::string_wide_from_utf8 wide_message(message);
    
    D2D1_RECT_F text_rect = D2D1::RectF(
        (float)layout.client_rect.left,
        (float)layout.client_rect.top + (layout.client_rect.bottom - layout.client_rect.top) / 2 - 20,
        (float)layout.client_rect.right,
        (float)layout.client_rect.top + (layout.client_rect.bottom - layout.client_rect.top) / 2 + 20
    );
    
    m_context.render_target->DrawTextW(wide_message.get_ptr(), (UINT32)wcslen(wide_message.get_ptr()),
                                      m_context.text_format_normal, &text_rect, m_context.text_brush);
}

// Continue with private methods in the next part...