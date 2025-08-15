// Album Art Grid UI Element for foobar2000 v2
#define FOOBAR2000_TARGET_VERSION 80

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "2.0.0",
    "Album Art Grid Component for foobar2000 v2\n"
    "Displays album covers in a customizable grid layout.\n"
    "\n"
    "Version 2.0: Added UI element with grid view"
);

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// GDI+ initialization
class gdiplus_initializer {
    ULONG_PTR m_token;
public:
    gdiplus_initializer() {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup(&m_token, &input, NULL);
    }
    ~gdiplus_initializer() {
        Gdiplus::GdiplusShutdown(m_token);
    }
};

static gdiplus_initializer g_gdiplus;

// Album info structure
struct album_info {
    pfc::string8 artist;
    pfc::string8 album;
    pfc::string8 path;
    album_art_data_ptr artwork;
};

// Grid window class
class album_grid_window : public ui_element_instance, public playlist_callback_static {
private:
    HWND m_hwnd;
    std::vector<album_info> m_albums;
    int m_item_size;
    int m_scroll_pos;
    service_ptr_t<ui_element_instance_callback> m_callback;
    
    static const UINT_PTR TIMER_REFRESH = 1;
    
public:
    album_grid_window(ui_element_config::ptr cfg, ui_element_instance_callback_ptr callback) 
        : m_callback(callback), m_hwnd(NULL), m_item_size(150), m_scroll_pos(0) {
    }
    
    void initialize_window(HWND parent) {
        static const TCHAR class_name[] = _T("AlbumGridClass");
        
        WNDCLASS wc = {};
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = core_api::get_my_instance();
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = class_name;
        
        RegisterClass(&wc);
        
        m_hwnd = CreateWindowEx(
            0, class_name, _T("Album Grid"),
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 100, 100,
            parent, NULL,
            core_api::get_my_instance(),
            this
        );
        
        SetTimer(m_hwnd, TIMER_REFRESH, 5000, NULL); // Refresh every 5 seconds
        load_albums();
    }
    
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        album_grid_window* instance;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lp;
            instance = (album_grid_window*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)instance);
        } else {
            instance = (album_grid_window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        
        if (instance) {
            switch (msg) {
                case WM_PAINT:
                    instance->on_paint();
                    return 0;
                case WM_SIZE:
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                case WM_VSCROLL:
                    instance->on_vscroll(LOWORD(wp));
                    return 0;
                case WM_MOUSEWHEEL:
                    instance->on_mousewheel(GET_WHEEL_DELTA_WPARAM(wp));
                    return 0;
                case WM_TIMER:
                    if (wp == TIMER_REFRESH) {
                        instance->load_albums();
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                    return 0;
                case WM_DESTROY:
                    KillTimer(hwnd, TIMER_REFRESH);
                    return 0;
            }
        }
        
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    
    void load_albums() {
        m_albums.clear();
        
        // Get media library
        auto api = library_manager::get();
        metadb_handle_list items;
        api->get_all_items(items);
        
        // Group by album
        pfc::map_t<pfc::string8, metadb_handle_ptr> album_map;
        
        for (size_t i = 0; i < items.get_count() && m_albums.size() < 100; i++) {
            auto handle = items[i];
            
            pfc::string8 artist, album;
            const file_info* info = NULL;
            
            if (handle->get_info(info)) {
                const char* artist_str = info->meta_get("ARTIST", 0);
                const char* album_str = info->meta_get("ALBUM", 0);
                
                if (artist_str && album_str) {
                    artist = artist_str;
                    album = album_str;
                    
                    pfc::string8 key;
                    key << artist << " - " << album;
                    
                    if (!album_map.have_item(key)) {
                        album_map[key] = handle;
                        
                        album_info ai;
                        ai.artist = artist;
                        ai.album = album;
                        ai.path = handle->get_path();
                        
                        // Try to get album art
                        try {
                            auto art_api = album_art_manager_v2::get();
                            auto extractor = art_api->open(items, pfc::list_single_ref_t<GUID>(album_art_ids::cover_front), NULL);
                            
                            try {
                                ai.artwork = extractor->query(album_art_ids::cover_front, NULL);
                            } catch (...) {}
                        } catch (...) {}
                        
                        m_albums.push_back(ai);
                    }
                }
            }
        }
    }
    
    void on_paint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Create memory DC for double buffering
        HDC memdc = CreateCompatibleDC(hdc);
        HBITMAP membmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP oldbmp = (HBITMAP)SelectObject(memdc, membmp);
        
        // Fill background
        FillRect(memdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
        
        // Calculate grid layout
        int cols = max(1, rc.right / (m_item_size + 10));
        int y_offset = -m_scroll_pos;
        
        // Draw albums
        for (size_t i = 0; i < m_albums.size(); i++) {
            int col = i % cols;
            int row = i / cols;
            
            int x = col * (m_item_size + 10) + 10;
            int y = row * (m_item_size + 40) + 10 + y_offset;
            
            if (y + m_item_size > 0 && y < rc.bottom) {
                draw_album(memdc, x, y, m_albums[i]);
            }
        }
        
        // Copy to screen
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memdc, 0, 0, SRCCOPY);
        
        // Cleanup
        SelectObject(memdc, oldbmp);
        DeleteObject(membmp);
        DeleteDC(memdc);
        
        EndPaint(m_hwnd, &ps);
    }
    
    void draw_album(HDC hdc, int x, int y, const album_info& album) {
        // Draw placeholder or artwork
        RECT rc = {x, y, x + m_item_size, y + m_item_size};
        
        if (album.artwork.is_valid() && album.artwork->get_size() > 0) {
            // Draw actual artwork using GDI+
            Gdiplus::Graphics graphics(hdc);
            
            IStream* stream = SHCreateMemStream(
                (const BYTE*)album.artwork->get_ptr(),
                album.artwork->get_size()
            );
            
            if (stream) {
                Gdiplus::Image* image = Gdiplus::Image::FromStream(stream);
                if (image && image->GetLastStatus() == Gdiplus::Ok) {
                    graphics.DrawImage(image, x, y, m_item_size, m_item_size);
                    delete image;
                }
                stream->Release();
            }
        } else {
            // Draw placeholder
            HBRUSH brush = CreateSolidBrush(RGB(200, 200, 200));
            FillRect(hdc, &rc, brush);
            DeleteObject(brush);
            
            // Draw border
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(150, 150, 150));
            HPEN oldpen = (HPEN)SelectObject(hdc, pen);
            HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, x, y, x + m_item_size, y + m_item_size);
            SelectObject(hdc, oldpen);
            SelectObject(hdc, oldbrush);
            DeleteObject(pen);
        }
        
        // Draw text
        RECT text_rc = {x, y + m_item_size + 2, x + m_item_size, y + m_item_size + 38};
        
        SetBkMode(hdc, TRANSPARENT);
        
        // Album name
        DrawTextA(hdc, album.album, -1, &text_rc, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        
        // Artist name
        text_rc.top += 18;
        COLORREF oldcolor = SetTextColor(hdc, RGB(100, 100, 100));
        DrawTextA(hdc, album.artist, -1, &text_rc, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        SetTextColor(hdc, oldcolor);
    }
    
    void on_vscroll(int code) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int cols = max(1, rc.right / (m_item_size + 10));
        int rows = (m_albums.size() + cols - 1) / cols;
        int total_height = rows * (m_item_size + 40) + 20;
        
        int old_pos = m_scroll_pos;
        
        switch (code) {
            case SB_LINEUP: m_scroll_pos -= 20; break;
            case SB_LINEDOWN: m_scroll_pos += 20; break;
            case SB_PAGEUP: m_scroll_pos -= rc.bottom; break;
            case SB_PAGEDOWN: m_scroll_pos += rc.bottom; break;
        }
        
        m_scroll_pos = max(0, min(m_scroll_pos, total_height - rc.bottom));
        
        if (m_scroll_pos != old_pos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    void on_mousewheel(int delta) {
        on_vscroll(delta > 0 ? SB_LINEUP : SB_LINEDOWN);
    }
    
    // ui_element_instance methods
    HWND get_wnd() override { return m_hwnd; }
    void set_configuration(ui_element_config::ptr config) override {}
    ui_element_config::ptr get_configuration() override { return ui_element_config::g_create_empty(g_get_guid()); }
    static GUID g_get_guid() {
        static const GUID guid = { 0xabcd1234, 0x5678, 0x90ab, { 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab } };
        return guid;
    }
    
    // playlist_callback_static methods (for updates)
    unsigned get_flags() override { return flag_on_items_added | flag_on_items_removed; }
    void on_items_added(t_size playlist, t_size start, const pfc::list_base_const_t<metadb_handle_ptr>& items, const bit_array& selection) override {
        load_albums();
        if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
    }
    void on_items_removed(t_size playlist, const bit_array& mask, t_size old_count, t_size new_count) override {
        load_albums();
        if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
    }
};

// UI element factory
class album_grid_ui_element : public ui_element_impl_withpopup<album_grid_window> {
public:
    GUID get_guid() override { return album_grid_window::g_get_guid(); }
    GUID get_subclass() override { return ui_element_subclass_media_library_viewers; }
    void get_name(pfc::string_base& out) override { out = "Album Art Grid"; }
    ui_element_instance_ptr instantiate(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback_ptr callback) override {
        auto instance = new service_impl_t<album_grid_window>(cfg, callback);
        instance->initialize_window(parent);
        return instance;
    }
    ui_element_config::ptr get_default_configuration() override { return ui_element_config::g_create_empty(get_guid()); }
    const char* get_description() override { return "Displays album artwork in a grid layout"; }
};

static service_factory_single_t<album_grid_ui_element> g_album_grid_ui_element_factory;