// Album Art Grid UI Element for foobar2000 v2
#define FOOBAR2000_TARGET_VERSION 80
#define _WIN32_WINNT 0x0600

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "4.0.0",
    "Album Art Grid UI Element for foobar2000 v2\n"
    "Displays album covers in a grid layout\n"
    "\n"
    "Add via Layout Editor as a UI Element"
);

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Album data structure
struct album_info {
    std::string artist;
    std::string album;
    metadb_handle_list tracks;
    bool has_art;
};

// Grid UI element instance
class album_grid_instance : public ui_element_instance {
private:
    HWND m_hwnd;
    HWND m_parent;
    std::vector<album_info> m_albums;
    int m_scroll_pos;
    int m_selected_index;
    int m_hover_index;
    service_ptr_t<ui_element_instance_callback> m_callback;
    
    static const int ITEM_SIZE = 120;
    static const int ITEM_HEIGHT = 160;
    static const int PADDING = 8;
    
public:
    album_grid_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr callback)
        : m_callback(callback), m_hwnd(NULL), m_parent(NULL), m_scroll_pos(0), 
          m_selected_index(-1), m_hover_index(-1) {
    }
    
    ~album_grid_instance() {
        if (m_hwnd && IsWindow(m_hwnd)) {
            DestroyWindow(m_hwnd);
        }
    }
    
    void initialize_window(HWND parent) {
        m_parent = parent;
        
        // Register window class
        static const TCHAR* class_name = TEXT("AlbumGridUIElement");
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
        
        // Load albums
        refresh_albums();
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
                case WM_MOUSEWHEEL: return instance->on_mousewheel(GET_WHEEL_DELTA_WPARAM(wp));
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
    
    void refresh_albums() {
        m_albums.clear();
        
        // Get media library
        auto lib = library_manager::get();
        metadb_handle_list all_items;
        lib->get_all_items(all_items);
        
        // Group by album
        std::map<std::string, album_info> album_map;
        
        for (t_size i = 0; i < all_items.get_count(); i++) {
            auto handle = all_items[i];
            
            file_info_impl info;
            if (handle->get_info(info)) {
                const char* artist = info.meta_get("ARTIST", 0);
                const char* album = info.meta_get("ALBUM", 0);
                
                if (album && album[0]) {  // At least album name required
                    std::string key;
                    if (artist && artist[0]) {
                        key = std::string(artist) + " - " + album;
                    } else {
                        key = album;
                    }
                    
                    if (album_map.find(key) == album_map.end()) {
                        album_info ai;
                        ai.artist = artist ? artist : "Unknown Artist";
                        ai.album = album;
                        ai.has_art = false;
                        
                        // Check if album art exists
                        try {
                            auto art_api = album_art_manager_v2::get();
                            abort_callback_dummy abort;
                            auto extractor = art_api->open(
                                pfc::list_single_ref_t<metadb_handle_ptr>(handle),
                                pfc::list_single_ref_t<GUID>(album_art_ids::cover_front),
                                abort
                            );
                            
                            try {
                                auto data = extractor->query(album_art_ids::cover_front, abort);
                                if (data.is_valid() && data->get_size() > 0) {
                                    ai.has_art = true;
                                }
                            } catch (...) {}
                        } catch (...) {}
                        
                        album_map[key] = ai;
                    }
                    
                    album_map[key].tracks.add_item(handle);
                }
            }
        }
        
        // Convert map to vector
        for (auto& pair : album_map) {
            m_albums.push_back(pair.second);
        }
        
        // Sort by artist then album
        std::sort(m_albums.begin(), m_albums.end(), [](const album_info& a, const album_info& b) {
            if (a.artist != b.artist) return a.artist < b.artist;
            return a.album < b.album;
        });
        
        // Update scrollbar and repaint
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
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
        HBRUSH bg_brush = CreateSolidBrush(RGB(25, 25, 25));
        FillRect(memdc, &rc, bg_brush);
        DeleteObject(bg_brush);
        
        // Calculate grid layout
        int cols = max(1, (rc.right - PADDING) / (ITEM_SIZE + PADDING));
        int visible_rows = (rc.bottom + ITEM_HEIGHT + PADDING - 1) / (ITEM_HEIGHT + PADDING) + 1;
        int first_row = m_scroll_pos / (ITEM_HEIGHT + PADDING);
        int y_offset = -(m_scroll_pos % (ITEM_HEIGHT + PADDING));
        
        // Set text properties
        SetBkMode(memdc, TRANSPARENT);
        
        HFONT font = CreateFont(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
        HFONT font_bold = CreateFont(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
        
        // Draw albums
        for (int row = 0; row < visible_rows; row++) {
            for (int col = 0; col < cols; col++) {
                size_t index = (first_row + row) * cols + col;
                if (index >= m_albums.size()) break;
                
                int x = col * (ITEM_SIZE + PADDING) + PADDING;
                int y = row * (ITEM_HEIGHT + PADDING) + PADDING + y_offset;
                
                if (y + ITEM_HEIGHT > 0 && y < rc.bottom) {
                    draw_album_item(memdc, font, font_bold, x, y, index);
                }
            }
        }
        
        DeleteObject(font);
        DeleteObject(font_bold);
        
        // Copy to screen
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memdc, 0, 0, SRCCOPY);
        
        // Cleanup
        SelectObject(memdc, oldbmp);
        DeleteObject(membmp);
        DeleteDC(memdc);
        
        EndPaint(m_hwnd, &ps);
        return 0;
    }
    
    void draw_album_item(HDC hdc, HFONT font, HFONT font_bold, int x, int y, size_t index) {
        auto& album = m_albums[index];
        
        // Draw selection/hover background
        if (index == m_selected_index) {
            RECT sel_rc = {x-2, y-2, x + ITEM_SIZE + 2, y + ITEM_HEIGHT + 2};
            HBRUSH sel_brush = CreateSolidBrush(RGB(50, 80, 120));
            FillRect(hdc, &sel_rc, sel_brush);
            DeleteObject(sel_brush);
        } else if (index == m_hover_index) {
            RECT hover_rc = {x-1, y-1, x + ITEM_SIZE + 1, y + ITEM_HEIGHT + 1};
            HBRUSH hover_brush = CreateSolidBrush(RGB(40, 40, 40));
            FillRect(hdc, &hover_rc, hover_brush);
            DeleteObject(hover_brush);
        }
        
        // Draw album art placeholder
        RECT album_rc = {x, y, x + ITEM_SIZE, y + ITEM_SIZE};
        
        if (album.has_art) {
            // Has art - draw dark grey placeholder with icon
            HBRUSH art_brush = CreateSolidBrush(RGB(60, 60, 60));
            FillRect(hdc, &album_rc, art_brush);
            DeleteObject(art_brush);
            
            // Draw music note icon
            SetTextColor(hdc, RGB(150, 150, 150));
            SelectObject(hdc, font_bold);
            DrawText(hdc, TEXT("♪"), -1, &album_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            // No art - draw darker placeholder
            HBRUSH noart_brush = CreateSolidBrush(RGB(35, 35, 35));
            FillRect(hdc, &album_rc, noart_brush);
            DeleteObject(noart_brush);
            
            // Draw empty icon
            SetTextColor(hdc, RGB(80, 80, 80));
            SelectObject(hdc, font);
            DrawText(hdc, TEXT("□"), -1, &album_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        
        // Draw border
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(45, 45, 45));
        HPEN oldpen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, x, y, x + ITEM_SIZE, y + ITEM_SIZE);
        SelectObject(hdc, oldpen);
        SelectObject(hdc, oldbrush);
        DeleteObject(pen);
        
        // Draw text below
        RECT text_rc = {x, y + ITEM_SIZE + 2, x + ITEM_SIZE, y + ITEM_SIZE + 18};
        
        // Album name
        SetTextColor(hdc, RGB(220, 220, 220));
        SelectObject(hdc, font_bold);
        DrawTextA(hdc, album.album.c_str(), -1, &text_rc, 
            DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        
        // Artist name
        text_rc.top += 16;
        text_rc.bottom += 16;
        SetTextColor(hdc, RGB(160, 160, 160));
        SelectObject(hdc, font);
        DrawTextA(hdc, album.artist.c_str(), -1, &text_rc,
            DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
    
    int hit_test(int x, int y) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int cols = max(1, (rc.right - PADDING) / (ITEM_SIZE + PADDING));
        int col = (x - PADDING) / (ITEM_SIZE + PADDING);
        int row = (y + m_scroll_pos - PADDING) / (ITEM_HEIGHT + PADDING);
        
        if (col >= 0 && col < cols && x >= PADDING) {
            int index = row * cols + col;
            if (index >= 0 && index < (int)m_albums.size()) {
                // Check if click is within item bounds
                int item_x = col * (ITEM_SIZE + PADDING) + PADDING;
                int item_y = row * (ITEM_HEIGHT + PADDING) + PADDING - m_scroll_pos;
                
                if (x >= item_x && x < item_x + ITEM_SIZE &&
                    y >= item_y && y < item_y + ITEM_HEIGHT) {
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
        if (index >= 0 && index < (int)m_albums.size()) {
            // Play the album
            static_api_ptr_t<playlist_manager> pm;
            t_size playlist = pm->get_active_playlist();
            if (playlist != pfc::infinite_size) {
                pm->playlist_clear(playlist);
                pm->playlist_add_items(playlist, m_albums[index].tracks, bit_array_false());
                pm->set_playing_playlist(playlist);
                
                static_api_ptr_t<playback_control> pc;
                pc->play_start();
            }
        }
        return 0;
    }
    
    LRESULT on_rbuttondown(int x, int y) {
        int index = hit_test(x, y);
        if (index >= 0 && index < (int)m_albums.size()) {
            m_selected_index = index;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        return 0;
    }
    
    LRESULT on_contextmenu(int x, int y) {
        if (m_selected_index >= 0 && m_selected_index < (int)m_albums.size()) {
            POINT pt = {x, y};
            if (x == -1 && y == -1) {
                GetCursorPos(&pt);
            }
            
            HMENU menu = CreatePopupMenu();
            AppendMenu(menu, MF_STRING, 1, TEXT("Play"));
            AppendMenu(menu, MF_STRING, 2, TEXT("Add to Current Playlist"));
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
            AppendMenu(menu, MF_STRING, 10, TEXT("Refresh"));
            
            int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, 
                pt.x, pt.y, 0, m_hwnd, NULL);
            DestroyMenu(menu);
            
            static_api_ptr_t<playlist_manager> pm;
            t_size playlist = pm->get_active_playlist();
            
            switch (cmd) {
                case 1: // Play
                    if (playlist != pfc::infinite_size) {
                        pm->playlist_clear(playlist);
                        pm->playlist_add_items(playlist, m_albums[m_selected_index].tracks, bit_array_false());
                        pm->set_playing_playlist(playlist);
                        static_api_ptr_t<playback_control> pc;
                        pc->play_start();
                    }
                    break;
                case 2: // Add to playlist
                    if (playlist != pfc::infinite_size) {
                        pm->playlist_add_items(playlist, m_albums[m_selected_index].tracks, bit_array_false());
                    }
                    break;
                case 10: // Refresh
                    refresh_albums();
                    break;
            }
        }
        return 0;
    }
    
    LRESULT on_mousemove(int x, int y) {
        int index = hit_test(x, y);
        if (index != m_hover_index) {
            m_hover_index = index;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        // Track mouse leave
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hwnd;
        TrackMouseEvent(&tme);
        
        return 0;
    }
    
    LRESULT on_mouseleave() {
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
        
        int cols = max(1, (rc.right - PADDING) / (ITEM_SIZE + PADDING));
        int rows = (m_albums.size() + cols - 1) / cols;
        int total_height = rows * (ITEM_HEIGHT + PADDING) + PADDING;
        
        int old_pos = m_scroll_pos;
        
        switch (code) {
            case SB_LINEUP: m_scroll_pos -= 20; break;
            case SB_LINEDOWN: m_scroll_pos += 20; break;
            case SB_PAGEUP: m_scroll_pos -= rc.bottom; break;
            case SB_PAGEDOWN: m_scroll_pos += rc.bottom; break;
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
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    LRESULT on_mousewheel(int delta) {
        on_vscroll(delta > 0 ? SB_LINEUP : SB_LINEDOWN);
        return 0;
    }
    
    void update_scrollbar() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int cols = max(1, (rc.right - PADDING) / (ITEM_SIZE + PADDING));
        int rows = (m_albums.size() + cols - 1) / cols;
        int total_height = rows * (ITEM_HEIGHT + PADDING) + PADDING;
        
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
    
    void set_configuration(ui_element_config::ptr config) override {}
    
    ui_element_config::ptr get_configuration() override { 
        return ui_element_config::g_create_empty(g_get_guid()); 
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
        return ui_element_config::g_create_empty(get_guid()); 
    }
    
    bool get_description(pfc::string_base & p_out) override { 
        p_out = "Displays album artwork in a grid layout. Double-click to play, right-click for options.";
        return true;
    }
    
    ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) override {
        return NULL;
    }
};

static service_factory_single_t<album_grid_ui_element> g_album_grid_ui_element_factory;

// Also keep a simple menu command for testing
class mainmenu_albumart : public mainmenu_commands {
    enum {
        cmd_refresh = 0,
        cmd_total
    };

public:
    t_uint32 get_command_count() override { 
        return cmd_total; 
    }
    
    GUID get_command(t_uint32 p_index) override {
        static const GUID guid_refresh = { 0x12345679, 0x1234, 0x5678, { 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef } };
        if (p_index == cmd_refresh) return guid_refresh;
        return pfc::guid_null;
    }
    
    void get_name(t_uint32 p_index, pfc::string_base & p_out) override {
        if (p_index == cmd_refresh) p_out = "Refresh Album Art Grid";
    }
    
    bool get_description(t_uint32 p_index, pfc::string_base & p_out) override {
        if (p_index == cmd_refresh) {
            p_out = "Refresh all Album Art Grid views";
            return true;
        }
        return false;
    }
    
    GUID get_parent() override {
        return mainmenu_groups::view;
    }
    
    void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override {
        if (p_index == cmd_refresh) {
            console::print("Album Art Grid: Refresh command (views will refresh on next repaint)");
        }
    }
};

static mainmenu_commands_factory_t<mainmenu_albumart> g_mainmenu_albumart;