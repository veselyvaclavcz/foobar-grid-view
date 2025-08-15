// Basic Album Art Grid Component for foobar2000 v2
#define FOOBAR2000_TARGET_VERSION 80
#define _WIN32_WINNT 0x0600

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "3.0.0",
    "Album Art Grid Component for foobar2000 v2\n"
    "Displays album covers in a grid layout\n"
    "\n"
    "Access via View -> Album Art Grid"
);

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Album data structure
struct album_info {
    std::string artist;
    std::string album;
    std::string path;
    metadb_handle_list tracks;
    bool has_art;
};

// Grid window
static HWND g_grid_hwnd = NULL;
static std::vector<album_info> g_albums;
static int g_scroll_pos = 0;

// Window procedure
LRESULT CALLBACK GridWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // Create memory DC for double buffering
            HDC memdc = CreateCompatibleDC(hdc);
            HBITMAP membmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HBITMAP oldbmp = (HBITMAP)SelectObject(memdc, membmp);
            
            // Fill background
            HBRUSH bg_brush = CreateSolidBrush(RGB(30, 30, 30));
            FillRect(memdc, &rc, bg_brush);
            DeleteObject(bg_brush);
            
            // Calculate grid layout
            int item_size = 150;
            int padding = 10;
            int cols = max(1, (rc.right - padding) / (item_size + padding));
            int y_offset = -g_scroll_pos;
            
            // Set text properties
            SetBkMode(memdc, TRANSPARENT);
            SetTextColor(memdc, RGB(255, 255, 255));
            
            HFONT font = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
            HFONT oldfont = (HFONT)SelectObject(memdc, font);
            
            // Draw albums
            for (size_t i = 0; i < g_albums.size(); i++) {
                int col = i % cols;
                int row = i / cols;
                
                int x = col * (item_size + padding) + padding;
                int y = row * (item_size + 50) + padding + y_offset;
                
                if (y + item_size > 0 && y < rc.bottom) {
                    // Draw album placeholder
                    RECT album_rc = {x, y, x + item_size, y + item_size};
                    
                    // Draw border
                    HBRUSH item_brush = CreateSolidBrush(RGB(50, 50, 50));
                    FillRect(memdc, &album_rc, item_brush);
                    DeleteObject(item_brush);
                    
                    // Draw album art placeholder text
                    if (g_albums[i].has_art) {
                        SetTextColor(memdc, RGB(100, 200, 100));
                        DrawText(memdc, TEXT("[ART]"), -1, &album_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    } else {
                        SetTextColor(memdc, RGB(100, 100, 100));
                        DrawText(memdc, TEXT("[NO ART]"), -1, &album_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    }
                    
                    // Draw text below
                    SetTextColor(memdc, RGB(255, 255, 255));
                    RECT text_rc = {x, y + item_size + 2, x + item_size, y + item_size + 20};
                    DrawTextA(memdc, g_albums[i].album.c_str(), -1, &text_rc, 
                        DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
                    
                    text_rc.top += 16;
                    text_rc.bottom += 16;
                    SetTextColor(memdc, RGB(180, 180, 180));
                    DrawTextA(memdc, g_albums[i].artist.c_str(), -1, &text_rc,
                        DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
                }
            }
            
            SelectObject(memdc, oldfont);
            DeleteObject(font);
            
            // Copy to screen
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memdc, 0, 0, SRCCOPY);
            
            // Cleanup
            SelectObject(memdc, oldbmp);
            DeleteObject(membmp);
            DeleteDC(memdc);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_SIZE: {
            // Update scrollbar
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            int item_size = 150;
            int padding = 10;
            int cols = max(1, (rc.right - padding) / (item_size + padding));
            int rows = (g_albums.size() + cols - 1) / cols;
            int total_height = rows * (item_size + 50) + 2 * padding;
            
            SCROLLINFO si = {};
            si.cbSize = sizeof(si);
            si.fMask = SIF_RANGE | SIF_PAGE;
            si.nMin = 0;
            si.nMax = total_height - 1;
            si.nPage = rc.bottom;
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
            
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        
        case WM_VSCROLL: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            int old_pos = g_scroll_pos;
            
            switch (LOWORD(wp)) {
                case SB_LINEUP: g_scroll_pos -= 30; break;
                case SB_LINEDOWN: g_scroll_pos += 30; break;
                case SB_PAGEUP: g_scroll_pos -= rc.bottom; break;
                case SB_PAGEDOWN: g_scroll_pos += rc.bottom; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: {
                    SCROLLINFO si = {};
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_TRACKPOS;
                    GetScrollInfo(hwnd, SB_VERT, &si);
                    g_scroll_pos = si.nTrackPos;
                    break;
                }
            }
            
            // Get max scroll
            SCROLLINFO si = {};
            si.cbSize = sizeof(si);
            si.fMask = SIF_RANGE | SIF_PAGE;
            GetScrollInfo(hwnd, SB_VERT, &si);
            int max_scroll = max(0, si.nMax - (int)si.nPage + 1);
            
            g_scroll_pos = max(0, min(g_scroll_pos, max_scroll));
            
            if (g_scroll_pos != old_pos) {
                SetScrollPos(hwnd, SB_VERT, g_scroll_pos, TRUE);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wp);
            SendMessage(hwnd, WM_VSCROLL, delta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            return 0;
        }
        
        case WM_LBUTTONDBLCLK: {
            // Get click position
            int x = GET_X_LPARAM(lp);
            int y = GET_Y_LPARAM(lp) + g_scroll_pos;
            
            // Calculate which album was clicked
            int item_size = 150;
            int padding = 10;
            RECT rc;
            GetClientRect(hwnd, &rc);
            int cols = max(1, (rc.right - padding) / (item_size + padding));
            
            int col = (x - padding) / (item_size + padding);
            int row = (y - padding) / (item_size + 50);
            
            if (col >= 0 && col < cols) {
                size_t index = row * cols + col;
                if (index < g_albums.size()) {
                    // Play the album
                    static_api_ptr_t<playlist_manager> pm;
                    t_size playlist = pm->get_active_playlist();
                    if (playlist != pfc::infinite_size) {
                        pm->playlist_clear(playlist);
                        pm->playlist_add_items(playlist, g_albums[index].tracks, bit_array_false());
                        pm->set_playing_playlist(playlist);
                        
                        static_api_ptr_t<playback_control> pc;
                        pc->play_start();
                        
                        // Show feedback
                        std::string msg = "Playing: " + g_albums[index].artist + " - " + g_albums[index].album;
                        SetWindowTextA(hwnd, msg.c_str());
                    }
                }
            }
            return 0;
        }
        
        case WM_DESTROY:
            g_grid_hwnd = NULL;
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wp, lp);
}

// Load albums from library
void load_albums() {
    g_albums.clear();
    
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
            
            if (artist && album) {
                std::string key = std::string(artist) + " - " + album;
                
                if (album_map.find(key) == album_map.end()) {
                    album_info ai;
                    ai.artist = artist;
                    ai.album = album;
                    ai.path = handle->get_path();
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
        g_albums.push_back(pair.second);
    }
    
    // Sort by artist then album
    std::sort(g_albums.begin(), g_albums.end(), [](const album_info& a, const album_info& b) {
        if (a.artist != b.artist) return a.artist < b.artist;
        return a.album < b.album;
    });
}

// Create and show grid window
void show_grid_window() {
    if (g_grid_hwnd && IsWindow(g_grid_hwnd)) {
        SetForegroundWindow(g_grid_hwnd);
        return;
    }
    
    // Register window class
    static bool registered = false;
    if (!registered) {
        WNDCLASS wc = {};
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = GridWndProc;
        wc.hInstance = core_api::get_my_instance();
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszClassName = TEXT("AlbumArtGridClass");
        RegisterClass(&wc);
        registered = true;
    }
    
    // Load albums
    load_albums();
    
    // Create window
    g_grid_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        TEXT("AlbumArtGridClass"),
        TEXT("Album Art Grid"),
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL,
        core_api::get_my_instance(),
        NULL
    );
    
    ShowWindow(g_grid_hwnd, SW_SHOW);
    UpdateWindow(g_grid_hwnd);
}

// Menu command implementation
class mainmenu_albumart : public mainmenu_commands {
    enum {
        cmd_show_grid = 0,
        cmd_total
    };

public:
    t_uint32 get_command_count() override { 
        return cmd_total; 
    }
    
    GUID get_command(t_uint32 p_index) override {
        static const GUID guid_show_grid = { 0x12345678, 0x1234, 0x5678, { 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef } };
        if (p_index == cmd_show_grid) return guid_show_grid;
        return pfc::guid_null;
    }
    
    void get_name(t_uint32 p_index, pfc::string_base & p_out) override {
        if (p_index == cmd_show_grid) p_out = "Album Art Grid";
    }
    
    bool get_description(t_uint32 p_index, pfc::string_base & p_out) override {
        if (p_index == cmd_show_grid) {
            p_out = "Show Album Art Grid view";
            return true;
        }
        return false;
    }
    
    GUID get_parent() override {
        return mainmenu_groups::view;
    }
    
    void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override {
        if (p_index == cmd_show_grid) {
            show_grid_window();
        }
    }
};

static mainmenu_commands_factory_t<mainmenu_albumart> g_mainmenu_albumart;

// Initialization
class albumart_grid_initquit : public initquit {
public:
    void on_init() override {
        console::print("Album Art Grid v3.0.0 loaded - Select View -> Album Art Grid");
    }
    
    void on_quit() override {
        if (g_grid_hwnd && IsWindow(g_grid_hwnd)) {
            DestroyWindow(g_grid_hwnd);
        }
    }
};

static initquit_factory_t<albumart_grid_initquit> g_albumart_grid_initquit;