#include "../include/grid_renderer.h"
#include "../include/album_art_cache.h"

// Continue GridRenderer private methods implementation

bool GridRenderer::CreateBrushes() {
    if (!m_context.render_target) return false;
    
    HRESULT hr = S_OK;
    
    // Background brush
    D2D1_COLOR_F bg_color = m_dark_theme ? COLOR_BACKGROUND_DARK : COLOR_BACKGROUND_LIGHT;
    hr = m_context.render_target->CreateSolidColorBrush(bg_color, &m_context.background_brush);
    if (FAILED(hr)) return false;
    
    // Text brush
    D2D1_COLOR_F text_color = m_dark_theme ? COLOR_TEXT_PRIMARY_DARK : COLOR_TEXT_PRIMARY_LIGHT;
    hr = m_context.render_target->CreateSolidColorBrush(text_color, &m_context.text_brush);
    if (FAILED(hr)) return false;
    
    // Selection brush
    D2D1_COLOR_F selection_color = m_dark_theme ? COLOR_SELECTION_DARK : COLOR_SELECTION_LIGHT;
    hr = m_context.render_target->CreateSolidColorBrush(selection_color, &m_context.selection_brush);
    if (FAILED(hr)) return false;
    
    // Hover brush
    D2D1_COLOR_F hover_color = m_dark_theme ? COLOR_HOVER_DARK : COLOR_HOVER_LIGHT;
    hr = m_context.render_target->CreateSolidColorBrush(hover_color, &m_context.hover_brush);
    if (FAILED(hr)) return false;
    
    // Border brush
    D2D1_COLOR_F border_color = m_dark_theme ? COLOR_BORDER_DARK : COLOR_BORDER_LIGHT;
    hr = m_context.render_target->CreateSolidColorBrush(border_color, &m_context.border_brush);
    if (FAILED(hr)) return false;
    
    return true;
}

bool GridRenderer::CreateTextFormats() {
    if (!m_context.dwrite_factory) return false;
    
    HRESULT hr = S_OK;
    
    // Normal text format
    hr = m_context.dwrite_factory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"en-US",
        &m_context.text_format_normal);
    
    if (FAILED(hr)) return false;
    
    m_context.text_format_normal->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_context.text_format_normal->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    m_context.text_format_normal->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    
    // Small text format
    hr = m_context.dwrite_factory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        10.0f,
        L"en-US",
        &m_context.text_format_small);
    
    if (FAILED(hr)) return false;
    
    m_context.text_format_small->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_context.text_format_small->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    m_context.text_format_small->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    
    return true;
}

ID2D1Bitmap* GridRenderer::CreateDefaultArtBitmap(int size) {
    if (!m_context.render_target) return nullptr;
    
    // Create compatible bitmap
    D2D1_SIZE_U bitmap_size = D2D1::SizeU(size, size);
    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    
    ID2D1Bitmap* bitmap = nullptr;
    HRESULT hr = m_context.render_target->CreateBitmap(bitmap_size, nullptr, 0, &props, &bitmap);
    if (FAILED(hr)) return nullptr;
    
    // Create a compatible render target to draw on the bitmap
    ID2D1BitmapRenderTarget* bitmap_rt = nullptr;
    hr = m_context.render_target->CreateCompatibleRenderTarget(
        D2D1::SizeF((float)size, (float)size), &bitmap_rt);
    
    if (FAILED(hr)) {
        bitmap->Release();
        return nullptr;
    }
    
    // Draw default album art
    bitmap_rt->BeginDraw();
    
    // Background
    ID2D1SolidColorBrush* bg_brush;
    D2D1_COLOR_F bg_color = m_dark_theme ? 
        D2D1::ColorF(0.12f, 0.12f, 0.12f, 1.0f) : D2D1::ColorF(0.92f, 0.92f, 0.92f, 1.0f);
    bitmap_rt->CreateSolidColorBrush(bg_color, &bg_brush);
    
    if (bg_brush) {
        D2D1_RECT_F rect = D2D1::RectF(0, 0, (float)size, (float)size);
        bitmap_rt->FillRectangle(&rect, bg_brush);
        bg_brush->Release();
    }
    
    // Music note icon
    ID2D1SolidColorBrush* icon_brush;
    D2D1_COLOR_F icon_color = m_dark_theme ? 
        D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f) : D2D1::ColorF(0.7f, 0.7f, 0.7f, 1.0f);
    bitmap_rt->CreateSolidColorBrush(icon_color, &icon_brush);
    
    if (icon_brush) {
        float center_x = size / 2.0f;
        float center_y = size / 2.0f;
        float note_size = size / 4.0f;
        
        // Draw simple music note shape
        D2D1_ELLIPSE note_head = D2D1::Ellipse(
            D2D1::Point2F(center_x - note_size/2, center_y + note_size/2), 
            note_size/3, note_size/4);
        bitmap_rt->FillEllipse(&note_head, icon_brush);
        
        // Note stem
        D2D1_RECT_F stem = D2D1::RectF(
            center_x - note_size/2 + note_size/3 - 2,
            center_y - note_size/2,
            center_x - note_size/2 + note_size/3 + 2,
            center_y + note_size/2);
        bitmap_rt->FillRectangle(&stem, icon_brush);
        
        icon_brush->Release();
    }
    
    bitmap_rt->EndDraw();
    
    // Get the bitmap from the render target
    ID2D1Bitmap* result_bitmap;
    hr = bitmap_rt->GetBitmap(&result_bitmap);
    
    bitmap_rt->Release();
    bitmap->Release();
    
    return SUCCEEDED(hr) ? result_bitmap : nullptr;
}

ID2D1Bitmap* GridRenderer::BitmapFromHBITMAP(HBITMAP hbitmap) {
    if (!hbitmap || !m_context.render_target) return nullptr;
    
    // Get bitmap info
    BITMAP bm;
    if (GetObject(hbitmap, sizeof(BITMAP), &bm) == 0) return nullptr;
    
    // Create compatible bitmap
    D2D1_SIZE_U size = D2D1::SizeU(bm.bmWidth, bm.bmHeight);
    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    
    // Get bitmap data
    HDC hdc = CreateCompatibleDC(nullptr);
    HDC mem_dc = CreateCompatibleDC(hdc);
    
    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = bm.bmWidth;
    bi.bmiHeader.biHeight = -bm.bmHeight; // Top-down DIB
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    
    BYTE* pixel_data = new BYTE[bm.bmWidth * bm.bmHeight * 4];
    
    HBITMAP old_bmp = (HBITMAP)SelectObject(mem_dc, hbitmap);
    int result = GetDIBits(mem_dc, hbitmap, 0, bm.bmHeight, pixel_data, &bi, DIB_RGB_COLORS);
    SelectObject(mem_dc, old_bmp);
    
    DeleteDC(mem_dc);
    DeleteDC(hdc);
    
    if (result == 0) {
        delete[] pixel_data;
        return nullptr;
    }
    
    // Create D2D bitmap
    ID2D1Bitmap* d2d_bitmap = nullptr;
    HRESULT hr = m_context.render_target->CreateBitmap(
        size, pixel_data, bm.bmWidth * 4, &props, &d2d_bitmap);
    
    delete[] pixel_data;
    
    return SUCCEEDED(hr) ? d2d_bitmap : nullptr;
}

void GridRenderer::DrawTextWithShadow(const wchar_t* text, const RECT& rect, 
                                     ID2D1SolidColorBrush* brush, IDWriteTextFormat* format) {
    if (!m_context.render_target || !text || !brush || !format) return;
    
    D2D1_RECT_F d2d_rect = GeometryUtils::RectToD2DRect(rect);
    
    // Draw shadow (offset by 1 pixel)
    ID2D1SolidColorBrush* shadow_brush;
    D2D1_COLOR_F shadow_color = m_dark_theme ? 
        D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.8f) : D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.8f);
    
    m_context.render_target->CreateSolidColorBrush(shadow_color, &shadow_brush);
    if (shadow_brush) {
        D2D1_RECT_F shadow_rect = D2D1::RectF(
            d2d_rect.left + 1, d2d_rect.top + 1, 
            d2d_rect.right + 1, d2d_rect.bottom + 1);
        
        m_context.render_target->DrawTextW(text, (UINT32)wcslen(text), format, 
                                          &shadow_rect, shadow_brush);
        shadow_brush->Release();
    }
    
    // Draw main text
    m_context.render_target->DrawTextW(text, (UINT32)wcslen(text), format, 
                                      &d2d_rect, brush);
}

D2D1_COLOR_F GridRenderer::ColorFromRGB(BYTE r, BYTE g, BYTE b, float alpha) {
    return D2D1::ColorF((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, alpha);
}

// Animation helper implementations
float AnimationHelper::EaseOutCubic(float t) {
    t = t - 1.0f;
    return t * t * t + 1.0f;
}

float AnimationHelper::EaseInOutCubic(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        t = t - 1.0f;
        return 1.0f + 4.0f * t * t * t;
    }
}

D2D1_COLOR_F AnimationHelper::InterpolateColor(const D2D1_COLOR_F& from, const D2D1_COLOR_F& to, float t) {
    return D2D1::ColorF(
        from.r + (to.r - from.r) * t,
        from.g + (to.g - from.g) * t,
        from.b + (to.b - from.b) * t,
        from.a + (to.a - from.a) * t
    );
}

float AnimationHelper::InterpolateFloat(float from, float to, float t) {
    return from + (to - from) * t;
}

// Geometry utilities implementations
namespace GeometryUtils {
    D2D1_RECT_F RectToD2DRect(const RECT& rect) {
        return D2D1::RectF((float)rect.left, (float)rect.top, 
                          (float)rect.right, (float)rect.bottom);
    }
    
    RECT D2DRectToRect(const D2D1_RECT_F& rect) {
        RECT result;
        result.left = (LONG)rect.left;
        result.top = (LONG)rect.top;
        result.right = (LONG)rect.right;
        result.bottom = (LONG)rect.bottom;
        return result;
    }
    
    bool RectIntersects(const RECT& a, const RECT& b) {
        return !(a.right <= b.left || b.right <= a.left || 
                a.bottom <= b.top || b.bottom <= a.top);
    }
    
    RECT InflateRect(const RECT& rect, int dx, int dy) {
        RECT result = rect;
        result.left -= dx;
        result.top -= dy;
        result.right += dx;
        result.bottom += dy;
        return result;
    }
    
    POINT RectCenter(const RECT& rect) {
        POINT center;
        center.x = (rect.left + rect.right) / 2;
        center.y = (rect.top + rect.bottom) / 2;
        return center;
    }
}