// Album Art Grid v8.4 - Simplified Auto-Fill with Column Control
#define FOOBAR2000_TARGET_VERSION 80
#define _WIN32_WINNT 0x0600

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include "SDK-2025-03-07/foobar2000/SDK/coreDarkMode.h"
#include "SDK-2025-03-07/foobar2000/helpers/DarkMode.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <memory>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uxtheme.lib")

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "8.4.0",
    "Album Art Grid v8.4 - Auto-Fill with Column Control\n"
    "Automatically fills container width\n"
    "Ctrl+Mouse Wheel to adjust number of columns\n"
    "Fixed selection alignment issues\n"
    "\n"
    "Created with assistance from Anthropic's Claude"
);

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// GDI+ initialization
class gdiplus_startup {
    ULONG_PTR token;
public:
    gdiplus_startup() {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup(&token, &input, NULL);
    }
    ~gdiplus_startup() {
        Gdiplus::GdiplusShutdown(token);
    }
};

static gdiplus_startup g_gdiplus;

// Configuration
struct grid_config {
    enum group_mode {
        GROUP_BY_FOLDER = 0,
        GROUP_BY_ALBUM,
        GROUP_BY_ARTIST
    };
    
    enum sort_mode {
        SORT_BY_NAME = 0,
        SORT_BY_DATE,
        SORT_BY_TRACK_COUNT
    };
    
    int columns;          // Number of columns (user-controlled)
    int text_lines;       // Number of text lines to show
    bool show_text;
    bool show_track_count;
    int font_size;
    group_mode grouping;
    sort_mode sorting;
    
    grid_config() : columns(5), text_lines(2), show_text(true), show_track_count(false),
                    font_size(9), grouping(GROUP_BY_FOLDER), sorting(SORT_BY_NAME) {}
    
    void load(ui_element_config::ptr cfg) {
        if (cfg.is_valid() && cfg->get_data_size() >= sizeof(grid_config)) {
            memcpy(this, cfg->get_data(), sizeof(grid_config));
            // Validate ranges
            columns = max(2, min(20, columns));
            text_lines = max(1, min(3, text_lines));
            font_size = max(7, min(14, font_size));
        }
    }
    
    ui_element_config::ptr save(const GUID& guid) {
        return ui_element_config::g_create(guid, this, sizeof(grid_config));
    }
};

// Thumbnail cache
struct thumbnail_data {
    Gdiplus::Bitmap* bitmap;
    DWORD last_access;
    bool loading;
    
    thumbnail_data() : bitmap(nullptr), last_access(GetTickCount()), loading(false) {}
    ~thumbnail_data() {
        if (bitmap) delete bitmap;
    }
};

// Album/folder data
struct grid_item {
    pfc::string8 display_name;
    pfc::string8 sort_key;
    metadb_handle_list tracks;
    std::unique_ptr<thumbnail_data> thumbnail;
    t_filetimestamp mod_time;
    
    grid_item() : thumbnail(new thumbnail_data()) {}
};

// Component GUID
static const GUID guid_element = 
{ 0xc26ce458, 0x41b7, 0x4e6e, { 0xa3, 0xf3, 0x12, 0x2f, 0x3c, 0xde, 0x8e, 0x64 } };

class album_grid_instance : public ui_element_instance {
private:
    HWND m_hwnd;
    HWND m_parent;
    std::vector<std::unique_ptr<grid_item>> m_items;
    int m_scroll_pos;
    std::set<int> m_selected_indices;
    int m_last_selected;
    int m_hover_index;
    service_ptr_t<ui_element_instance_callback> m_callback;
    grid_config m_config;
    fb2k::CCoreDarkModeHooks m_dark;
    bool m_tracking;
    
    // Layout calculation
    int m_item_size;  // Calculated item size based on columns and width
    int m_padding;    // Calculated padding
    
    static const int MIN_PADDING = 4;
    static const int MAX_PADDING = 12;
    static const int LINE_HEIGHT = 16;
    
public:
    album_grid_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr callback)
        : m_callback(callback), m_hwnd(NULL), m_parent(NULL), m_scroll_pos(0), 
          m_last_selected(-1), m_hover_index(-1), m_tracking(false),
          m_item_size(120), m_padding(8) {
        m_config.load(config);
    }
    
    ~album_grid_instance() {
        if (m_hwnd && IsWindow(m_hwnd)) {
            DestroyWindow(m_hwnd);
        }
    }
    
    void initialize_window(HWND parent) {
        m_parent = parent;
        
        // Register window class
        static const TCHAR* class_name = TEXT("AlbumGridV8Auto");
        static bool registered = false;
        
        if (!registered) {
            WNDCLASS wc = {};
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
            wc.lpfnWndProc = WndProc;
            wc.hInstance = core_api::get_my_instance();
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.lpszClassName = class_name;
            RegisterClass(&wc);
            registered = true;
        }
        
        // Create window
        m_hwnd = CreateWindow(
            class_name,
            TEXT("Album Grid"),
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 100, 100,
            parent,
            NULL,
            core_api::get_my_instance(),
            this
        );
        
        if (m_hwnd) {
            // Initialize dark mode
            m_dark.AddDialogWithControls(m_hwnd);
            if (m_dark) {
                SetWindowTheme(m_hwnd, L"DarkMode_Explorer", nullptr);
            }
            
            // Load items after a short delay
            SetTimer(m_hwnd, 1, 100, NULL);
        }
    }
    
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        album_grid_instance* instance = nullptr;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lp;
            instance = (album_grid_instance*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)instance);
        } else {
            instance = (album_grid_instance*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        
        if (instance) {
            switch (msg) {
                case WM_PAINT: return instance->on_paint();
                case WM_SIZE: return instance->on_size();
                case WM_VSCROLL: return instance->on_vscroll(LOWORD(wp));
                case WM_MOUSEWHEEL: return instance->on_mousewheel(GET_WHEEL_DELTA_WPARAM(wp), GET_KEYSTATE_WPARAM(wp));
                case WM_LBUTTONDOWN: return instance->on_lbuttondown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), wp);
                case WM_LBUTTONDBLCLK: return instance->on_lbuttondblclk(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_RBUTTONDOWN: return instance->on_rbuttondown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_MOUSEMOVE: return instance->on_mousemove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_MOUSELEAVE: return instance->on_mouseleave();
                case WM_ERASEBKGND: return 1;
                case WM_TIMER: {
                    if (wp == 1) {
                        KillTimer(hwnd, 1);
                        instance->refresh_items();
                    }
                    return 0;
                }
                case WM_KEYDOWN: return instance->on_keydown(wp);
            }
        }
        
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    
    void calculate_layout() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int width = rc.right;
        
        if (width <= 0 || m_config.columns <= 0) {
            m_item_size = 120;
            m_padding = 8;
            return;
        }
        
        // Calculate optimal item size and padding to fill width exactly
        // Total width = columns * item_size + (columns + 1) * padding
        // We want to maximize item_size while keeping reasonable padding
        
        int best_size = 50;
        int best_padding = MIN_PADDING;
        
        // Try different padding values
        for (int pad = MIN_PADDING; pad <= MAX_PADDING; pad++) {
            int available = width - (m_config.columns + 1) * pad;
            if (available > 0) {
                int size = available / m_config.columns;
                if (size > best_size && size <= 300) {
                    best_size = size;
                    best_padding = pad;
                }
            }
        }
        
        m_item_size = best_size;
        m_padding = best_padding;
    }
    
    void refresh_items() {
        m_items.clear();
        m_selected_indices.clear();
        
        // Get media library
        static_api_ptr_t<library_manager> lm;
        metadb_handle_list all_items;
        lm->get_all_items(all_items);
        
        if (all_items.get_count() == 0) {
            InvalidateRect(m_hwnd, NULL, FALSE);
            return;
        }
        
        // Group items
        std::map<pfc::string8, std::unique_ptr<grid_item>> grouped;
        
        for (t_size i = 0; i < all_items.get_count(); i++) {
            metadb_handle_ptr track = all_items[i];
            const char* path = track->get_path();
            
            pfc::string8 key;
            pfc::string8 display_name;
            
            if (m_config.grouping == grid_config::GROUP_BY_FOLDER) {
                pfc::string8 folder = pfc::string_directory(path);
                key = folder;
                const char* folder_name = strrchr(folder, '\\');
                if (!folder_name) folder_name = strrchr(folder, '/');
                display_name = folder_name ? (folder_name + 1) : folder;
            } else if (m_config.grouping == grid_config::GROUP_BY_ALBUM) {
                file_info_impl info;
                if (track->get_info(info)) {
                    const char* album = info.meta_get("album", 0);
                    const char* artist = info.meta_get("artist", 0);
                    if (album) {
                        key = album;
                        if (artist) {
                            key += "|";
                            key += artist;
                            display_name << album << "\n" << artist;
                        } else {
                            display_name = album;
                        }
                    }
                }
            } else {
                file_info_impl info;
                if (track->get_info(info)) {
                    const char* artist = info.meta_get("artist", 0);
                    if (artist) {
                        key = artist;
                        display_name = artist;
                    }
                }
            }
            
            if (key.is_empty()) continue;
            
            auto it = grouped.find(key);
            if (it == grouped.end()) {
                auto item = std::make_unique<grid_item>();
                item->display_name = display_name;
                item->sort_key = display_name;
                item->mod_time = metadb::get_filetimestamp(path);
                grouped[key] = std::move(item);
                it = grouped.find(key);
            }
            
            it->second->tracks.add_item(track);
        }
        
        // Convert to vector
        for (auto& pair : grouped) {
            m_items.push_back(std::move(pair.second));
        }
        
        // Sort items
        sort_items();
        
        // Load album art for first items
        load_visible_artwork();
        
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void sort_items() {
        if (m_config.sorting == grid_config::SORT_BY_NAME) {
            std::sort(m_items.begin(), m_items.end(), 
                [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                    return pfc::stricmp_ascii(a->sort_key.c_str(), b->sort_key.c_str()) < 0;
                });
        } else if (m_config.sorting == grid_config::SORT_BY_DATE) {
            std::sort(m_items.begin(), m_items.end(), 
                [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                    return a->mod_time > b->mod_time;
                });
        } else {
            std::sort(m_items.begin(), m_items.end(), 
                [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                    return a->tracks.get_count() > b->tracks.get_count();
                });
        }
    }
    
    void load_visible_artwork() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        int item_total_height = m_item_size + text_height;
        
        int first_row = m_scroll_pos / (item_total_height + m_padding);
        int visible_rows = (rc.bottom + item_total_height) / (item_total_height + m_padding) + 1;
        
        int first_visible = first_row * m_config.columns;
        int last_visible = min((int)m_items.size() - 1, (first_row + visible_rows + 1) * m_config.columns);
        
        auto art_api = album_art_manager_v2::get();
        abort_callback_dummy abort;
        
        for (int i = max(0, first_visible); i <= last_visible && i < (int)m_items.size(); i++) {
            auto& item = m_items[i];
            
            if (!item->thumbnail->bitmap && !item->thumbnail->loading && item->tracks.get_count() > 0) {
                item->thumbnail->loading = true;
                
                try {
                    auto extractor = art_api->open(
                        pfc::list_single_ref_t<metadb_handle_ptr>(item->tracks[0]),
                        pfc::list_single_ref_t<GUID>(album_art_ids::cover_front),
                        abort
                    );
                    
                    auto art_data = extractor->query(album_art_ids::cover_front, abort);
                    if (art_data.is_valid()) {
                        create_thumbnail(item.get(), art_data);
                    }
                } catch (...) {
                    // No artwork
                }
                
                item->thumbnail->loading = false;
            }
        }
    }
    
    void create_thumbnail(grid_item* item, album_art_data_ptr art_data) {
        if (!art_data.is_valid()) return;
        
        IStream* stream = SHCreateMemStream(
            (const BYTE*)art_data->get_ptr(), 
            art_data->get_size()
        );
        
        if (stream) {
            Gdiplus::Image img(stream);
            if (img.GetLastStatus() == Gdiplus::Ok) {
                // Create thumbnail at current size
                item->thumbnail->bitmap = new Gdiplus::Bitmap(m_item_size, m_item_size);
                Gdiplus::Graphics g(item->thumbnail->bitmap);
                g.SetInterpolationMode(Gdiplus::InterpolationModeBicubic);
                g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
                g.DrawImage(&img, 0, 0, m_item_size, m_item_size);
            }
            stream->Release();
        }
    }
    
    LRESULT on_paint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Create memory DC for double buffering
        HDC memdc = CreateCompatibleDC(hdc);
        HBITMAP membmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP oldbmp = (HBITMAP)SelectObject(memdc, membmp);
        
        // Fill background
        t_ui_color color_back = m_callback->query_std_color(ui_color_background);
        HBRUSH bg_brush = CreateSolidBrush(color_back);
        FillRect(memdc, &rc, bg_brush);
        DeleteObject(bg_brush);
        
        // Setup GDI+
        Gdiplus::Graphics graphics(memdc);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeDefault);
        
        // Calculate layout
        calculate_layout();
        
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        int item_total_height = m_item_size + text_height;
        
        // Calculate grid dimensions
        int cols = m_config.columns;
        int visible_rows = (rc.bottom + item_total_height + m_padding - 1) / (item_total_height + m_padding) + 1;
        int first_row = m_scroll_pos / (item_total_height + m_padding);
        int y_offset = -(m_scroll_pos % (item_total_height + m_padding));
        
        // Get theme colors
        t_ui_color color_text = m_callback->query_std_color(ui_color_text);
        t_ui_color color_selected = m_callback->query_std_color(ui_color_selection);
        
        // Create font
        HFONT hFont = CreateFont(
            -MulDiv(m_config.font_size, GetDeviceCaps(memdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI")
        );
        
        SetBkMode(memdc, TRANSPARENT);
        
        // Calculate starting X position to center the grid
        int total_width = cols * m_item_size + (cols + 1) * m_padding;
        int start_x = (rc.right - total_width) / 2;
        if (start_x < 0) start_x = 0;
        
        // Draw items
        for (int row = 0; row < visible_rows; row++) {
            for (int col = 0; col < cols; col++) {
                size_t index = (first_row + row) * cols + col;
                if (index >= m_items.size()) break;
                
                int x = start_x + col * (m_item_size + m_padding) + m_padding;
                int y = row * (item_total_height + m_padding) + m_padding + y_offset;
                
                if (y + item_total_height > 0 && y < rc.bottom) {
                    draw_item(memdc, graphics, hFont, x, y, index, color_text, color_selected, text_height);
                }
            }
        }
        
        DeleteObject(hFont);
        
        // Draw status if empty
        if (m_items.empty()) {
            SetTextColor(memdc, color_text);
            RECT status_rc = rc;
            DrawText(memdc, TEXT("Loading library..."), -1, &status_rc, 
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        
        // Copy to screen
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memdc, 0, 0, SRCCOPY);
        
        // Cleanup
        SelectObject(memdc, oldbmp);
        DeleteObject(membmp);
        DeleteDC(memdc);
        
        EndPaint(m_hwnd, &ps);
        return 0;
    }
    
    void draw_item(HDC hdc, Gdiplus::Graphics& graphics, HFONT font, int x, int y, 
                   size_t index, COLORREF color_text, COLORREF color_selected, int text_height) {
        
        auto& item = m_items[index];
        
        // Draw selection/hover background
        bool selected = m_selected_indices.find(index) != m_selected_indices.end();
        bool hover = (index == m_hover_index);
        
        if (selected) {
            HBRUSH sel_brush = CreateSolidBrush(color_selected);
            RECT sel_rc = {x - 2, y - 2, x + m_item_size + 2, y + m_item_size + text_height + 2};
            FillRect(hdc, &sel_rc, sel_brush);
            DeleteObject(sel_brush);
        } else if (hover) {
            HBRUSH hover_brush = CreateSolidBrush(RGB(80, 80, 80));
            RECT hover_rc = {x - 1, y - 1, x + m_item_size + 1, y + m_item_size + text_height + 1};
            FillRect(hdc, &hover_rc, hover_brush);
            DeleteObject(hover_brush);
        }
        
        // Draw thumbnail or placeholder
        if (item->thumbnail->bitmap) {
            graphics.DrawImage(item->thumbnail->bitmap, x, y, m_item_size, m_item_size);
        } else {
            // Draw placeholder
            RECT placeholder_rc = {x, y, x + m_item_size, y + m_item_size};
            
            for (int i = 0; i < 3; i++) {
                HBRUSH grad_brush = CreateSolidBrush(RGB(40 + i * 10, 40 + i * 10, 40 + i * 10));
                InflateRect(&placeholder_rc, -i, -i);
                FillRect(hdc, &placeholder_rc, grad_brush);
                DeleteObject(grad_brush);
            }
            
            SetTextColor(hdc, RGB(100, 100, 100));
            HFONT icon_font = CreateFont(m_item_size / 4, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI Symbol"));
            HFONT old_font = (HFONT)SelectObject(hdc, icon_font);
            
            DrawText(hdc, TEXT("🎵"), -1, &placeholder_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            SelectObject(hdc, old_font);
            DeleteObject(icon_font);
        }
        
        // Draw border
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(50, 50, 50));
        HPEN oldpen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, x, y, x + m_item_size, y + m_item_size);
        SelectObject(hdc, oldpen);
        SelectObject(hdc, oldbrush);
        DeleteObject(pen);
        
        // Draw text
        if (m_config.show_text) {
            RECT text_rc = {x, y + m_item_size + 2, x + m_item_size, y + m_item_size + text_height};
            SetTextColor(hdc, color_text);
            SelectObject(hdc, font);
            
            pfc::stringcvt::string_os_from_utf8 display_text(item->display_name.c_str());
            
            if (m_config.show_track_count && item->tracks.get_count() > 0) {
                pfc::string8 text_with_count;
                text_with_count << item->display_name << " (" << item->tracks.get_count() << ")";
                display_text = pfc::stringcvt::string_os_from_utf8(text_with_count.c_str());
            }
            
            UINT format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX;
            if (m_config.text_lines == 1) {
                format = DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX;
            }
            
            DrawText(hdc, display_text, -1, &text_rc, format);
        }
    }
    
    int hit_test(int mx, int my) {
        calculate_layout();
        
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        int item_total_height = m_item_size + text_height;
        
        // Calculate starting X position (same as in paint)
        int total_width = m_config.columns * m_item_size + (m_config.columns + 1) * m_padding;
        int start_x = (rc.right - total_width) / 2;
        if (start_x < 0) start_x = 0;
        
        // Check if click is within grid bounds
        if (mx < start_x + m_padding || mx >= start_x + total_width - m_padding) {
            return -1;
        }
        
        // Calculate column
        int rel_x = mx - start_x - m_padding;
        int col = rel_x / (m_item_size + m_padding);
        
        // Check if click is in padding between columns
        int col_start = col * (m_item_size + m_padding);
        if (rel_x < col_start || rel_x >= col_start + m_item_size) {
            return -1;
        }
        
        if (col >= m_config.columns) {
            return -1;
        }
        
        // Calculate row
        int adjusted_y = my + m_scroll_pos - m_padding;
        if (adjusted_y < 0) {
            return -1;
        }
        
        int row = adjusted_y / (item_total_height + m_padding);
        
        // Check if click is in padding between rows
        int row_start = row * (item_total_height + m_padding);
        if (adjusted_y < row_start || adjusted_y >= row_start + item_total_height) {
            return -1;
        }
        
        int index = row * m_config.columns + col;
        
        if (index >= 0 && index < (int)m_items.size()) {
            return index;
        }
        
        return -1;
    }
    
    LRESULT on_lbuttondown(int x, int y, WPARAM keys) {
        int index = hit_test(x, y);
        
        if (index >= 0) {
            if (keys & MK_CONTROL) {
                // Toggle selection
                if (m_selected_indices.find(index) != m_selected_indices.end()) {
                    m_selected_indices.erase(index);
                } else {
                    m_selected_indices.insert(index);
                }
                m_last_selected = index;
            } else if (keys & MK_SHIFT && m_last_selected >= 0) {
                // Range selection
                m_selected_indices.clear();
                int start = min(m_last_selected, index);
                int end = max(m_last_selected, index);
                for (int i = start; i <= end; i++) {
                    m_selected_indices.insert(i);
                }
            } else {
                // Single selection
                m_selected_indices.clear();
                m_selected_indices.insert(index);
                m_last_selected = index;
            }
        } else {
            // Clear selection
            m_selected_indices.clear();
        }
        
        InvalidateRect(m_hwnd, NULL, FALSE);
        SetFocus(m_hwnd);
        return 0;
    }
    
    LRESULT on_lbuttondblclk(int x, int y) {
        int index = hit_test(x, y);
        if (index >= 0 && index < (int)m_items.size()) {
            // Play the item
            static_api_ptr_t<playlist_manager> pm;
            t_size playlist = pm->get_active_playlist();
            if (playlist != pfc::infinite_size) {
                pm->playlist_clear(playlist);
                
                if (!m_selected_indices.empty() && 
                    m_selected_indices.find(index) != m_selected_indices.end()) {
                    // Play all selected
                    for (int sel_index : m_selected_indices) {
                        if (sel_index < (int)m_items.size()) {
                            pm->playlist_add_items(playlist, m_items[sel_index]->tracks, bit_array_false());
                        }
                    }
                } else {
                    // Play only double-clicked
                    pm->playlist_add_items(playlist, m_items[index]->tracks, bit_array_false());
                }
                
                pm->set_playing_playlist(playlist);
                static_api_ptr_t<playback_control> pc;
                pc->play_start();
            }
        }
        return 0;
    }
    
    LRESULT on_rbuttondown(int x, int y) {
        int index = hit_test(x, y);
        if (index >= 0 && m_selected_indices.find(index) == m_selected_indices.end()) {
            m_selected_indices.clear();
            m_selected_indices.insert(index);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        // Create context menu
        HMENU menu = CreatePopupMenu();
        
        if (!m_selected_indices.empty()) {
            AppendMenu(menu, MF_STRING, 1, TEXT("Play"));
            AppendMenu(menu, MF_STRING, 2, TEXT("Add to Current Playlist"));
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        }
        
        // Group menu
        HMENU group_menu = CreatePopupMenu();
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_FOLDER ? MF_CHECKED : 0), 
                   10, TEXT("By Folder"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_ALBUM ? MF_CHECKED : 0), 
                   11, TEXT("By Album"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_ARTIST ? MF_CHECKED : 0), 
                   12, TEXT("By Artist"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)group_menu, TEXT("Group"));
        
        // Sort menu
        HMENU sort_menu = CreatePopupMenu();
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_NAME ? MF_CHECKED : 0), 
                   20, TEXT("By Name"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_DATE ? MF_CHECKED : 0), 
                   21, TEXT("By Date"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_TRACK_COUNT ? MF_CHECKED : 0), 
                   22, TEXT("By Track Count"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)sort_menu, TEXT("Sort"));
        
        // Column count info
        pfc::string8 col_text;
        col_text << "Columns: " << m_config.columns << " (Ctrl+Wheel to adjust)";
        pfc::stringcvt::string_os_from_utf8 col_text_w(col_text.c_str());
        AppendMenu(menu, MF_STRING | MF_DISABLED, 0, col_text_w);
        
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu, MF_STRING | (m_config.show_text ? MF_CHECKED : 0), 30, TEXT("Show Labels"));
        AppendMenu(menu, MF_STRING, 40, TEXT("Refresh"));
        
        // Show menu
        POINT pt = {x, y};
        ClientToScreen(m_hwnd, &pt);
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, 
            pt.x, pt.y, 0, m_hwnd, NULL);
        
        DestroyMenu(menu);
        
        // Handle commands
        bool config_changed = false;
        
        switch (cmd) {
            case 1: // Play
                if (!m_selected_indices.empty()) {
                    static_api_ptr_t<playlist_manager> pm;
                    t_size playlist = pm->get_active_playlist();
                    if (playlist != pfc::infinite_size) {
                        pm->playlist_clear(playlist);
                        for (int sel_index : m_selected_indices) {
                            if (sel_index < (int)m_items.size()) {
                                pm->playlist_add_items(playlist, m_items[sel_index]->tracks, bit_array_false());
                            }
                        }
                        pm->set_playing_playlist(playlist);
                        static_api_ptr_t<playback_control> pc;
                        pc->play_start();
                    }
                }
                break;
            
            case 2: // Add to playlist
                if (!m_selected_indices.empty()) {
                    static_api_ptr_t<playlist_manager> pm;
                    t_size playlist = pm->get_active_playlist();
                    if (playlist != pfc::infinite_size) {
                        for (int sel_index : m_selected_indices) {
                            if (sel_index < (int)m_items.size()) {
                                pm->playlist_add_items(playlist, m_items[sel_index]->tracks, bit_array_false());
                            }
                        }
                    }
                }
                break;
            
            case 10: case 11: case 12:
                m_config.grouping = (grid_config::group_mode)(cmd - 10);
                config_changed = true;
                refresh_items();
                break;
            
            case 20: case 21: case 22:
                m_config.sorting = (grid_config::sort_mode)(cmd - 20);
                config_changed = true;
                sort_items();
                InvalidateRect(m_hwnd, NULL, FALSE);
                break;
            
            case 30:
                m_config.show_text = !m_config.show_text;
                config_changed = true;
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
                break;
            
            case 40:
                refresh_items();
                break;
        }
        
        if (config_changed) {
            m_callback->on_min_max_info_change();
        }
        
        return 0;
    }
    
    LRESULT on_mousewheel(int delta, int keys) {
        if (keys & MK_CONTROL) {
            // Adjust column count
            int new_columns = m_config.columns;
            if (delta > 0) {
                new_columns = min(20, new_columns + 1);
            } else {
                new_columns = max(2, new_columns - 1);
            }
            
            if (new_columns != m_config.columns) {
                m_config.columns = new_columns;
                
                // Clear thumbnails as they need to be recreated at new size
                for (auto& item : m_items) {
                    if (item->thumbnail->bitmap) {
                        delete item->thumbnail->bitmap;
                        item->thumbnail->bitmap = nullptr;
                    }
                }
                
                calculate_layout();
                load_visible_artwork();
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
                m_callback->on_min_max_info_change();
            }
        } else {
            // Normal scroll
            int lines = 3;
            for (int i = 0; i < lines; i++) {
                on_vscroll(delta > 0 ? SB_LINEUP : SB_LINEDOWN);
            }
        }
        return 0;
    }
    
    LRESULT on_mousemove(int x, int y) {
        if (!m_tracking) {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            TrackMouseEvent(&tme);
            m_tracking = true;
        }
        
        int index = hit_test(x, y);
        if (index != m_hover_index) {
            m_hover_index = index;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    LRESULT on_mouseleave() {
        m_tracking = false;
        if (m_hover_index != -1) {
            m_hover_index = -1;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        return 0;
    }
    
    LRESULT on_keydown(WPARAM key) {
        if (key == VK_F5) {
            refresh_items();
        } else if (key == 'A' && GetKeyState(VK_CONTROL) & 0x8000) {
            // Select all
            m_selected_indices.clear();
            for (size_t i = 0; i < m_items.size(); i++) {
                m_selected_indices.insert(i);
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        return 0;
    }
    
    LRESULT on_size() {
        calculate_layout();
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
        return 0;
    }
    
    LRESULT on_vscroll(int code) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        calculate_layout();
        
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        int item_total_height = m_item_size + text_height;
        int rows = (m_items.size() + m_config.columns - 1) / m_config.columns;
        int total_height = rows * (item_total_height + m_padding) + m_padding;
        
        int old_pos = m_scroll_pos;
        int line_height = 30;
        int page_height = rc.bottom - rc.bottom % line_height;
        
        switch (code) {
            case SB_LINEUP: m_scroll_pos -= line_height; break;
            case SB_LINEDOWN: m_scroll_pos += line_height; break;
            case SB_PAGEUP: m_scroll_pos -= page_height; break;
            case SB_PAGEDOWN: m_scroll_pos += page_height; break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION: {
                SCROLLINFO si = {};
                si.cbSize = sizeof(si);
                si.fMask = SIF_TRACKPOS;
                GetScrollInfo(m_hwnd, SB_VERT, &si);
                m_scroll_pos = si.nTrackPos;
                break;
            }
        }
        
        m_scroll_pos = max(0, min(m_scroll_pos, max(0, total_height - rc.bottom)));
        
        if (m_scroll_pos != old_pos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);
            load_visible_artwork();
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    void update_scrollbar() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        calculate_layout();
        
        int text_height = m_config.show_text ? (LINE_HEIGHT * m_config.text_lines + 4) : 0;
        int item_total_height = m_item_size + text_height;
        int rows = (m_items.size() + m_config.columns - 1) / m_config.columns;
        int total_height = rows * (item_total_height + m_padding) + m_padding;
        
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = total_height - 1;
        si.nPage = rc.bottom;
        si.nPos = m_scroll_pos;
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
    }
    
    // ui_element_instance methods
    HWND get_wnd() override { return m_hwnd; }
    
    void set_configuration(ui_element_config::ptr config) override {
        m_config.load(config);
        refresh_items();
    }
    
    ui_element_config::ptr get_configuration() override {
        return m_config.save(guid_element);
    }
    
    GUID get_guid() override {
        return guid_element;
    }
    
    GUID get_subclass() override {
        return ui_element_subclass_utility;
    }
    
    ui_element_min_max_info get_min_max_info() override {
        ui_element_min_max_info ret;
        ret.m_min_width = 200;
        ret.m_min_height = 100;
        ret.m_max_width = 0;  // 0 means no limit
        ret.m_max_height = 0;  // 0 means no limit
        return ret;
    }
};

// Factory
class album_grid_factory : public ui_element_factory<album_grid_instance> {
public:
    GUID get_guid() override {
        return guid_element;
    }
    
    void get_name(pfc::string_base& out) override {
        out = "Album Art Grid";
    }
    
    ui_element_config::ptr get_default_configuration() override {
        grid_config cfg;
        return cfg.save(get_guid());
    }
    
    const char* get_description() override {
        return "Album Art Grid v8.4 - Auto-Fill with Column Control\n\n"
               "Features:\n"
               "• Automatically fills container width\n"
               "• Ctrl+Mouse Wheel to adjust columns (2-20)\n"
               "• Multi-select with Ctrl/Shift+Click\n"
               "• Dark mode scrollbar support\n"
               "• Fixed selection alignment\n\n"
               "Keyboard shortcuts:\n"
               "• F5 - Refresh library\n"
               "• Ctrl+A - Select all\n"
               "• Ctrl+Wheel - Change column count";
    }
};

static album_grid_factory g_album_grid_factory;