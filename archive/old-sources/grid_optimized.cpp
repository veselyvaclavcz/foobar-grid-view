// Optimized Album Art Grid UI Element for foobar2000 v2
#define FOOBAR2000_TARGET_VERSION 80
#define _WIN32_WINNT 0x0600

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <memory>
#include <unordered_map>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "6.0.0",
    "Optimized Album Art Grid UI Element for foobar2000 v2\n"
    "Fast thumbnail loading with folder grouping\n"
    "Lazy loading for smooth scrolling\n"
    "\n"
    "Right-click for options, Ctrl+Wheel to resize"
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
    
    int item_size;
    int text_height;
    bool show_text;
    int font_size;
    group_mode grouping;
    
    grid_config() : item_size(150), text_height(35), show_text(true), font_size(9), grouping(GROUP_BY_FOLDER) {}
    
    void load(ui_element_config::ptr cfg) {
        if (cfg.is_valid() && cfg->get_data_size() >= sizeof(grid_config)) {
            memcpy(this, cfg->get_data(), sizeof(grid_config));
            // Validate ranges
            item_size = max(60, min(300, item_size));
            text_height = max(20, min(60, text_height));
            font_size = max(7, min(14, font_size));
        }
    }
    
    ui_element_config::ptr save(const GUID& guid) {
        return ui_element_config::g_create(guid, this, sizeof(grid_config));
    }
};

// Thumbnail cache entry
struct thumbnail_data {
    Gdiplus::Bitmap* bitmap;
    DWORD last_access;
    bool loading;
    
    thumbnail_data() : bitmap(nullptr), last_access(GetTickCount()), loading(false) {}
    ~thumbnail_data() {
        if (bitmap) delete bitmap;
    }
};

// Album/folder data structure
struct grid_item {
    std::string display_name;
    std::string sort_key;
    std::string path;
    metadb_handle_list tracks;
    album_art_data_ptr artwork;
    std::shared_ptr<thumbnail_data> thumbnail;
    
    grid_item() : thumbnail(std::make_shared<thumbnail_data>()) {}
};

// Grid UI element instance
class album_grid_instance : public ui_element_instance {
private:
    HWND m_hwnd;
    HWND m_parent;
    std::vector<std::unique_ptr<grid_item>> m_items;
    int m_scroll_pos;
    int m_selected_index;
    int m_hover_index;
    service_ptr_t<ui_element_instance_callback> m_callback;
    grid_config m_config;
    bool m_tracking;
    
    // Thumbnail cache - limited size for memory efficiency
    static const int MAX_CACHED_THUMBNAILS = 100;
    std::unordered_map<std::string, std::shared_ptr<thumbnail_data>> m_thumbnail_cache;
    
    // Visible range for lazy loading
    int m_first_visible;
    int m_last_visible;
    
    static const int PADDING = 8;
    
public:
    album_grid_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr callback)
        : m_callback(callback), m_hwnd(NULL), m_parent(NULL), m_scroll_pos(0), 
          m_selected_index(-1), m_hover_index(-1), m_tracking(false),
          m_first_visible(0), m_last_visible(0) {
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
        static const TCHAR* class_name = TEXT("AlbumGridOptimized");
        static bool registered = false;
        
        if (!registered) {
            WNDCLASS wc = {};
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
            wc.lpfnWndProc = WndProc;
            wc.hInstance = core_api::get_my_instance();
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = NULL;
            wc.lpszClassName = class_name;
            RegisterClass(&wc);
            registered = true;
        }
        
        // Create window
        m_hwnd = CreateWindowEx(
            0, class_name, TEXT(""),
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 100, 100,
            parent, NULL,
            core_api::get_my_instance(),
            this
        );
        
        // Load items
        refresh_items();
    }
    
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        album_grid_instance* instance;
        
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
                case WM_LBUTTONDOWN: return instance->on_lbuttondown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_LBUTTONDBLCLK: return instance->on_lbuttondblclk(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_RBUTTONDOWN: return instance->on_rbuttondown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_MOUSEMOVE: return instance->on_mousemove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_MOUSELEAVE: return instance->on_mouseleave();
                case WM_ERASEBKGND: return 1;
                case WM_CONTEXTMENU: return instance->on_contextmenu(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            }
        }
        
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    
    void refresh_items() {
        m_items.clear();
        m_thumbnail_cache.clear();
        
        // Get media library
        auto lib = library_manager::get();
        metadb_handle_list all_items;
        lib->get_all_items(all_items);
        
        // Group items based on mode
        std::map<std::string, std::unique_ptr<grid_item>> item_map;
        
        for (t_size i = 0; i < all_items.get_count(); i++) {
            auto handle = all_items[i];
            
            std::string key;
            std::string display_name;
            
            if (m_config.grouping == grid_config::GROUP_BY_FOLDER) {
                // Group by folder
                std::string path = handle->get_path();
                size_t last_slash = path.find_last_of("\\/");
                if (last_slash != std::string::npos && last_slash > 0) {
                    size_t second_last = path.find_last_of("\\/", last_slash - 1);
                    if (second_last != std::string::npos) {
                        key = path.substr(second_last + 1, last_slash - second_last - 1);
                        display_name = key;
                    }
                }
                if (key.empty()) {
                    key = "Unknown Folder";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_ALBUM) {
                // Group by album
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* artist = info.meta_get("ARTIST", 0);
                    const char* album = info.meta_get("ALBUM", 0);
                    
                    if (album && album[0]) {
                        key = album;
                        display_name = album;
                        if (artist && artist[0]) {
                            key = std::string(artist) + " - " + album;
                        }
                    }
                }
                if (key.empty()) {
                    key = "Unknown Album";
                    display_name = key;
                }
            } else { // GROUP_BY_ARTIST
                // Group by artist
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* artist = info.meta_get("ARTIST", 0);
                    if (artist && artist[0]) {
                        key = artist;
                        display_name = artist;
                    }
                }
                if (key.empty()) {
                    key = "Unknown Artist";
                    display_name = key;
                }
            }
            
            if (item_map.find(key) == item_map.end()) {
                auto item = std::make_unique<grid_item>();
                item->display_name = display_name;
                item->sort_key = key;
                item->path = handle->get_path();
                item_map[key] = std::move(item);
            }
            
            item_map[key]->tracks.add_item(handle);
        }
        
        // Convert map to vector
        for (auto& pair : item_map) {
            m_items.push_back(std::move(pair.second));
        }
        
        // Sort items
        std::sort(m_items.begin(), m_items.end(), 
            [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                return pfc::stricmp_ascii(a->sort_key.c_str(), b->sort_key.c_str()) < 0;
            });
        
        // Update scrollbar and repaint
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    // Create thumbnail from album art data
    Gdiplus::Bitmap* create_thumbnail(album_art_data_ptr artwork, int size) {
        if (!artwork.is_valid() || artwork->get_size() == 0) return nullptr;
        
        IStream* stream = SHCreateMemStream(
            (const BYTE*)artwork->get_ptr(),
            artwork->get_size()
        );
        
        if (!stream) return nullptr;
        
        Gdiplus::Bitmap* original = Gdiplus::Bitmap::FromStream(stream);
        stream->Release();
        
        if (!original || original->GetLastStatus() != Gdiplus::Ok) {
            if (original) delete original;
            return nullptr;
        }
        
        // Create thumbnail at requested size
        Gdiplus::Bitmap* thumbnail = new Gdiplus::Bitmap(size, size, PixelFormat32bppARGB);
        Gdiplus::Graphics graphics(thumbnail);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.DrawImage(original, 0, 0, size, size);
        
        delete original;
        return thumbnail;
    }
    
    // Load artwork for visible items only
    void load_visible_artwork() {
        for (int i = m_first_visible; i <= m_last_visible && i < (int)m_items.size(); i++) {
            auto& item = m_items[i];
            
            // Skip if already loaded or loading
            if (item->thumbnail->bitmap || item->thumbnail->loading) continue;
            
            // Skip if no tracks
            if (item->tracks.get_count() == 0) continue;
            
            item->thumbnail->loading = true;
            
            // Try to get album art from first track
            try {
                auto art_api = album_art_manager_v2::get();
                abort_callback_dummy abort;
                auto extractor = art_api->open(
                    pfc::list_single_ref_t<metadb_handle_ptr>(item->tracks[0]),
                    pfc::list_single_ref_t<GUID>(album_art_ids::cover_front),
                    abort
                );
                
                try {
                    item->artwork = extractor->query(album_art_ids::cover_front, abort);
                    
                    // Create thumbnail
                    if (item->artwork.is_valid()) {
                        item->thumbnail->bitmap = create_thumbnail(item->artwork, m_config.item_size);
                    }
                } catch (...) {}
            } catch (...) {}
            
            item->thumbnail->loading = false;
            item->thumbnail->last_access = GetTickCount();
        }
        
        // Clean up old thumbnails if cache is too large
        if (m_thumbnail_cache.size() > MAX_CACHED_THUMBNAILS) {
            // Find and remove least recently used thumbnails
            std::vector<std::pair<DWORD, std::string>> access_times;
            for (auto& pair : m_thumbnail_cache) {
                access_times.push_back({pair.second->last_access, pair.first});
            }
            std::sort(access_times.begin(), access_times.end());
            
            // Remove oldest half
            for (size_t i = 0; i < access_times.size() / 2; i++) {
                m_thumbnail_cache.erase(access_times[i].second);
            }
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
        
        // Fill background with theme color
        t_ui_color color_back = m_callback->query_std_color(ui_color_background);
        HBRUSH bg_brush = CreateSolidBrush(color_back);
        FillRect(memdc, &rc, bg_brush);
        DeleteObject(bg_brush);
        
        // Setup GDI+
        Gdiplus::Graphics graphics(memdc);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeDefault); // Faster for thumbnails
        
        // Calculate grid layout
        int item_total_height = m_config.item_size + (m_config.show_text ? m_config.text_height : 0);
        int cols = max(1, (rc.right - PADDING) / (m_config.item_size + PADDING));
        int visible_rows = (rc.bottom + item_total_height + PADDING - 1) / (item_total_height + PADDING) + 1;
        int first_row = m_scroll_pos / (item_total_height + PADDING);
        int y_offset = -(m_scroll_pos % (item_total_height + PADDING));
        
        // Calculate visible range for lazy loading
        m_first_visible = first_row * cols;
        m_last_visible = min((int)m_items.size() - 1, (first_row + visible_rows) * cols);
        
        // Load artwork for visible items (async would be better but this is simplified)
        load_visible_artwork();
        
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
        
        // Draw items
        for (int row = 0; row < visible_rows; row++) {
            for (int col = 0; col < cols; col++) {
                size_t index = (first_row + row) * cols + col;
                if (index >= m_items.size()) break;
                
                int x = col * (m_config.item_size + PADDING) + PADDING;
                int y = row * (item_total_height + PADDING) + PADDING + y_offset;
                
                if (y + item_total_height > 0 && y < rc.bottom) {
                    draw_item(memdc, graphics, hFont, x, y, index, color_text, color_selected);
                }
            }
        }
        
        DeleteObject(hFont);
        
        // Copy to screen
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memdc, 0, 0, SRCCOPY);
        
        // Cleanup
        SelectObject(memdc, oldbmp);
        DeleteObject(membmp);
        DeleteDC(memdc);
        
        EndPaint(m_hwnd, &ps);
        return 0;
    }
    
    void draw_item(HDC hdc, Gdiplus::Graphics& graphics, HFONT font,
                   int x, int y, size_t index, 
                   t_ui_color color_text, t_ui_color color_selected) {
        auto& item = m_items[index];
        
        // Draw selection/hover background
        if (index == m_selected_index) {
            RECT sel_rc = {x-2, y-2, x + m_config.item_size + 2, 
                          y + m_config.item_size + (m_config.show_text ? m_config.text_height : 0) + 2};
            HBRUSH sel_brush = CreateSolidBrush(color_selected);
            FillRect(hdc, &sel_rc, sel_brush);
            DeleteObject(sel_brush);
        } else if (index == m_hover_index) {
            RECT hover_rc = {x-1, y-1, x + m_config.item_size + 1, 
                            y + m_config.item_size + (m_config.show_text ? m_config.text_height : 0) + 1};
            HBRUSH hover_brush = CreateSolidBrush(RGB(60, 60, 60));
            FillRect(hdc, &hover_rc, hover_brush);
            DeleteObject(hover_brush);
        }
        
        // Draw thumbnail or placeholder
        if (item->thumbnail->bitmap) {
            graphics.DrawImage(item->thumbnail->bitmap, x, y, m_config.item_size, m_config.item_size);
            item->thumbnail->last_access = GetTickCount();
        } else {
            // Draw folder/album placeholder
            RECT placeholder_rc = {x, y, x + m_config.item_size, y + m_config.item_size};
            HBRUSH placeholder_brush = CreateSolidBrush(RGB(45, 45, 45));
            FillRect(hdc, &placeholder_rc, placeholder_brush);
            DeleteObject(placeholder_brush);
            
            // Draw folder icon for folder mode, music note for others
            SetTextColor(hdc, RGB(90, 90, 90));
            HFONT icon_font = CreateFont(m_config.item_size / 3, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI Symbol"));
            HFONT old_font = (HFONT)SelectObject(hdc, icon_font);
            
            if (m_config.grouping == grid_config::GROUP_BY_FOLDER) {
                DrawText(hdc, TEXT("📁"), -1, &placeholder_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            } else {
                DrawText(hdc, TEXT("🎵"), -1, &placeholder_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            
            SelectObject(hdc, old_font);
            DeleteObject(icon_font);
        }
        
        // Draw border
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(50, 50, 50));
        HPEN oldpen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, x, y, x + m_config.item_size, y + m_config.item_size);
        SelectObject(hdc, oldpen);
        SelectObject(hdc, oldbrush);
        DeleteObject(pen);
        
        // Draw text
        if (m_config.show_text) {
            RECT text_rc = {x, y + m_config.item_size + 2, x + m_config.item_size, 
                           y + m_config.item_size + m_config.text_height};
            SetTextColor(hdc, color_text);
            SelectObject(hdc, font);
            DrawTextA(hdc, item->display_name.c_str(), -1, &text_rc, 
                DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        }
    }
    
    int hit_test(int x, int y) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int item_total_height = m_config.item_size + (m_config.show_text ? m_config.text_height : 0);
        int cols = max(1, (rc.right - PADDING) / (m_config.item_size + PADDING));
        int col = (x - PADDING) / (m_config.item_size + PADDING);
        int row = (y + m_scroll_pos - PADDING) / (item_total_height + PADDING);
        
        if (col >= 0 && col < cols && x >= PADDING) {
            int index = row * cols + col;
            if (index >= 0 && index < (int)m_items.size()) {
                int item_x = col * (m_config.item_size + PADDING) + PADDING;
                int item_y = row * (item_total_height + PADDING) + PADDING - m_scroll_pos;
                
                if (x >= item_x && x < item_x + m_config.item_size &&
                    y >= item_y && y < item_y + item_total_height) {
                    return index;
                }
            }
        }
        
        return -1;
    }
    
    LRESULT on_lbuttondown(int x, int y) {
        int index = hit_test(x, y);
        if (index != m_selected_index) {
            m_selected_index = index;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
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
                pm->playlist_add_items(playlist, m_items[index]->tracks, bit_array_false());
                pm->set_playing_playlist(playlist);
                
                static_api_ptr_t<playback_control> pc;
                pc->play_start();
            }
        }
        return 0;
    }
    
    LRESULT on_rbuttondown(int x, int y) {
        int index = hit_test(x, y);
        if (index >= 0 && index < (int)m_items.size()) {
            m_selected_index = index;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        return 0;
    }
    
    LRESULT on_contextmenu(int x, int y) {
        POINT pt = {x, y};
        if (x == -1 && y == -1) {
            GetCursorPos(&pt);
        }
        
        HMENU menu = CreatePopupMenu();
        
        if (m_selected_index >= 0 && m_selected_index < (int)m_items.size()) {
            AppendMenu(menu, MF_STRING, 1, TEXT("Play"));
            AppendMenu(menu, MF_STRING, 2, TEXT("Add to Current Playlist"));
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        }
        
        // Grouping options
        HMENU group_menu = CreatePopupMenu();
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_FOLDER ? MF_CHECKED : 0), 
                   10, TEXT("By Folder"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_ALBUM ? MF_CHECKED : 0), 
                   11, TEXT("By Album"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_ARTIST ? MF_CHECKED : 0), 
                   12, TEXT("By Artist"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)group_menu, TEXT("Group"));
        
        // Size options
        HMENU size_menu = CreatePopupMenu();
        AppendMenu(size_menu, MF_STRING | (m_config.item_size == 80 ? MF_CHECKED : 0), 20, TEXT("Small (80px)"));
        AppendMenu(size_menu, MF_STRING | (m_config.item_size == 120 ? MF_CHECKED : 0), 21, TEXT("Medium (120px)"));
        AppendMenu(size_menu, MF_STRING | (m_config.item_size == 150 ? MF_CHECKED : 0), 22, TEXT("Large (150px)"));
        AppendMenu(size_menu, MF_STRING | (m_config.item_size == 200 ? MF_CHECKED : 0), 23, TEXT("Extra Large (200px)"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)size_menu, TEXT("Size"));
        
        AppendMenu(menu, MF_STRING | (m_config.show_text ? MF_CHECKED : 0), 30, TEXT("Show Labels"));
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu, MF_STRING, 40, TEXT("Refresh"));
        
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, 
            pt.x, pt.y, 0, m_hwnd, NULL);
        DestroyMenu(group_menu);
        DestroyMenu(size_menu);
        DestroyMenu(menu);
        
        static_api_ptr_t<playlist_manager> pm;
        t_size playlist = pm->get_active_playlist();
        
        bool config_changed = false;
        bool needs_refresh = false;
        
        switch (cmd) {
            case 1: // Play
                if (playlist != pfc::infinite_size && m_selected_index >= 0) {
                    pm->playlist_clear(playlist);
                    pm->playlist_add_items(playlist, m_items[m_selected_index]->tracks, bit_array_false());
                    pm->set_playing_playlist(playlist);
                    static_api_ptr_t<playback_control> pc;
                    pc->play_start();
                }
                break;
            case 2: // Add to playlist
                if (playlist != pfc::infinite_size && m_selected_index >= 0) {
                    pm->playlist_add_items(playlist, m_items[m_selected_index]->tracks, bit_array_false());
                }
                break;
            case 10: m_config.grouping = grid_config::GROUP_BY_FOLDER; needs_refresh = true; config_changed = true; break;
            case 11: m_config.grouping = grid_config::GROUP_BY_ALBUM; needs_refresh = true; config_changed = true; break;
            case 12: m_config.grouping = grid_config::GROUP_BY_ARTIST; needs_refresh = true; config_changed = true; break;
            case 20: m_config.item_size = 80; config_changed = true; break;
            case 21: m_config.item_size = 120; config_changed = true; break;
            case 22: m_config.item_size = 150; config_changed = true; break;
            case 23: m_config.item_size = 200; config_changed = true; break;
            case 30: m_config.show_text = !m_config.show_text; config_changed = true; break;
            case 40: needs_refresh = true; break;
        }
        
        if (needs_refresh) {
            refresh_items();
        }
        
        if (config_changed) {
            m_callback->on_min_max_info_change();
            update_scrollbar();
            InvalidateRect(m_hwnd, NULL, FALSE);
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
    
    LRESULT on_size() {
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
        return 0;
    }
    
    LRESULT on_vscroll(int code) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int item_total_height = m_config.item_size + (m_config.show_text ? m_config.text_height : 0);
        int cols = max(1, (rc.right - PADDING) / (m_config.item_size + PADDING));
        int rows = (m_items.size() + cols - 1) / cols;
        int total_height = rows * (item_total_height + PADDING) + PADDING;
        
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
            case SB_TOP: m_scroll_pos = 0; break;
            case SB_BOTTOM: m_scroll_pos = total_height; break;
        }
        
        m_scroll_pos = max(0, min(m_scroll_pos, max(0, total_height - rc.bottom)));
        
        if (m_scroll_pos != old_pos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    LRESULT on_mousewheel(int delta, int keys) {
        // Ctrl+Wheel to resize
        if (keys & MK_CONTROL) {
            int new_size = m_config.item_size;
            if (delta > 0) {
                new_size = min(250, new_size + 10);
            } else {
                new_size = max(60, new_size - 10);
            }
            
            if (new_size != m_config.item_size) {
                m_config.item_size = new_size;
                m_callback->on_min_max_info_change();
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        } else {
            // Smoother scrolling
            int lines = 3;
            if (delta > 0) {
                on_vscroll(SB_LINEUP);
                if (lines > 1) on_vscroll(SB_LINEUP);
                if (lines > 2) on_vscroll(SB_LINEUP);
            } else {
                on_vscroll(SB_LINEDOWN);
                if (lines > 1) on_vscroll(SB_LINEDOWN);
                if (lines > 2) on_vscroll(SB_LINEDOWN);
            }
        }
        return 0;
    }
    
    void update_scrollbar() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int item_total_height = m_config.item_size + (m_config.show_text ? m_config.text_height : 0);
        int cols = max(1, (rc.right - PADDING) / (m_config.item_size + PADDING));
        int rows = (m_items.size() + cols - 1) / cols;
        int total_height = rows * (item_total_height + PADDING) + PADDING;
        
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
        return m_config.save(g_get_guid());
    }
    
    GUID get_guid() override {
        return g_get_guid();
    }
    
    GUID get_subclass() override {
        return ui_element_subclass_media_library_viewers;
    }
    
    static GUID g_get_guid() {
        // {5A6B4F80-9B3D-4c85-A8F2-7C3D9B5E2A10}
        static const GUID guid = { 0x5a6b4f80, 0x9b3d, 0x4c85, { 0xa8, 0xf2, 0x7c, 0x3d, 0x9b, 0x5e, 0x2a, 0x10 } };
        return guid;
    }
};

// UI element factory
class album_grid_ui_element : public ui_element {
public:
    GUID get_guid() override { 
        return album_grid_instance::g_get_guid(); 
    }
    
    GUID get_subclass() override { 
        return ui_element_subclass_media_library_viewers; 
    }
    
    void get_name(pfc::string_base& out) override { 
        out = "Album Art Grid"; 
    }
    
    ui_element_instance_ptr instantiate(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback_ptr callback) override {
        service_ptr_t<album_grid_instance> instance = new service_impl_t<album_grid_instance>(cfg, callback);
        instance->initialize_window(parent);
        return instance;
    }
    
    ui_element_config::ptr get_default_configuration() override { 
        grid_config default_cfg;
        return default_cfg.save(get_guid());
    }
    
    bool get_description(pfc::string_base & p_out) override { 
        p_out = "Fast album art grid with folder grouping. Right-click for options, Ctrl+Wheel to resize.";
        return true;
    }
    
    ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) override {
        return NULL;
    }
};

static service_factory_single_t<album_grid_ui_element> g_album_grid_ui_element_factory;