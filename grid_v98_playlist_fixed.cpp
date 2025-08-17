// Album Art Grid v9.7 - Enhanced Badge & Tooltip Edition
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
#include <unordered_map>
#include <random>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uxtheme.lib")

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "9.8.10",
    "Album Art Grid v9.8.10 - Critical Crash Fixes\n"
    "FIXES in v9.8.10:\n"
    "- Fixed: Crash when using 'Add to Current Playlist' multiple times\n"
    "- Fixed: Race condition during playlist refresh operations\n"
    "- Fixed: Menu Play now preserves playlist in playlist view mode\n"
    "- Fixed: Menu Play searches for tracks in playlist (like double-click)\n"
    "From v9.8.9:\n"
    "- Fixed: Double-click in Playlist view no longer clears the playlist\n"
    "- Fixed: Double-click on filtered items now plays the correct album\n"
    "- Fixed: Hover tooltips now show correct items when search filter is active\n"
    "NEW in v9.8.8:\n"
    "- Search box with real-time filtering (Ctrl+Shift+S)\n"
    "From v9.8.2:\n"
    "- Configurable double-click behavior via submenu\n"
    "- Choose: Play, Add to Current Playlist, or Add to New Playlist\n"
    "From v9.8:\n"
    "- Toggle between Media Library and Current Playlist views\n"
    "- Auto-refresh when playlist changes\n"
    "- Right-click menu to switch view modes\n"
    "Features:\n"
    "- Artist images for artist-based groupings\n"
    "- Track count badge in top-right corner\n"
    "- 13 grouping modes, 11 sorting options\n"
    "  Name, Artist, Album, Year, Genre,\n"
    "  Date Modified, Total Size, Track Count,\n"
    "  Rating, Path, Random/Shuffle\n"
    "- Auto-fill mode with Ctrl+Mouse Wheel (3-10 columns)\n"
    "- High-quality image rendering\n"
    "- Dark mode support\n"
    "\n"
    "Created with assistance from Anthropic's Claude"
);

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid_v98.dll");

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
        GROUP_BY_ARTIST,
        GROUP_BY_ARTIST_ALBUM,
        GROUP_BY_GENRE,
        GROUP_BY_YEAR,
        GROUP_BY_LABEL,
        GROUP_BY_COMPOSER,
        GROUP_BY_PERFORMER,
        GROUP_BY_ALBUM_ARTIST,
        GROUP_BY_DIRECTORY,
        GROUP_BY_COMMENT,
        GROUP_BY_RATING
    };
    
    enum sort_mode {
        SORT_BY_NAME = 0,
        SORT_BY_DATE,
        SORT_BY_TRACK_COUNT,
        SORT_BY_ARTIST,
        SORT_BY_ALBUM,
        SORT_BY_YEAR,
        SORT_BY_GENRE,
        SORT_BY_PATH,
        SORT_BY_RANDOM,
        SORT_BY_SIZE,
        SORT_BY_RATING
    };
    
    enum view_mode {
        VIEW_LIBRARY = 0,
        VIEW_PLAYLIST
    };
    
    enum doubleclick_action {
        DOUBLECLICK_PLAY = 0,
        DOUBLECLICK_ADD_TO_CURRENT,
        DOUBLECLICK_ADD_TO_NEW
    };
    
    int columns;  // Number of columns (user-controlled via Ctrl+Wheel)
    int text_lines;  // Number of text lines to show
    bool show_text;
    bool show_track_count;
    int font_size;
    group_mode grouping;
    sort_mode sorting;
    view_mode view;  // NEW: Library or Playlist view
    doubleclick_action doubleclick;  // NEW: Configurable double-click behavior
    
    grid_config() : columns(5), text_lines(2), show_text(true), show_track_count(true),
                    font_size(11), grouping(GROUP_BY_FOLDER), sorting(SORT_BY_NAME), view(VIEW_LIBRARY), doubleclick(DOUBLECLICK_PLAY) {}
    
    void load(ui_element_config::ptr cfg) {
        if (cfg.is_valid()) {
            size_t expected_size = sizeof(grid_config);
            size_t actual_size = cfg->get_data_size();
            
            if (actual_size >= expected_size - sizeof(view_mode) - sizeof(doubleclick_action)) {
                memcpy(this, cfg->get_data(), min(actual_size, expected_size));
                
                // Handle backward compatibility for view field
                if (actual_size < expected_size - sizeof(doubleclick_action)) {
                    view = VIEW_LIBRARY;
                }
                // Handle backward compatibility for doubleclick field
                if (actual_size < expected_size) {
                    doubleclick = DOUBLECLICK_PLAY;
                }
                
                columns = max(3, min(10, columns));
                text_lines = max(1, min(3, text_lines));
                font_size = max(7, min(14, font_size));
                if ((int)view < 0 || (int)view > VIEW_PLAYLIST) view = VIEW_LIBRARY;
                if ((int)doubleclick < 0 || (int)doubleclick > DOUBLECLICK_ADD_TO_NEW) doubleclick = DOUBLECLICK_PLAY;
            }
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
    int cached_size;  // Size this thumbnail was created at
    
    thumbnail_data() : bitmap(nullptr), last_access(GetTickCount()), loading(false), cached_size(0) {}
    ~thumbnail_data() {
        if (bitmap) delete bitmap;
    }
};

// Album/folder data structure
struct grid_item {
    pfc::string8 display_name;  // Using pfc::string8 for proper UTF-8 handling
    pfc::string8 sort_key;
    pfc::string8 path;
    metadb_handle_list tracks;
    album_art_data_ptr artwork;
    std::shared_ptr<thumbnail_data> thumbnail;
    t_filetimestamp newest_date;
    // Additional fields for sorting
    pfc::string8 artist;
    pfc::string8 album;
    pfc::string8 genre;
    pfc::string8 year;
    int rating;
    t_filesize total_size;
    
    grid_item() : thumbnail(std::make_shared<thumbnail_data>()), newest_date(0), rating(0), total_size(0) {}
};

// Grid UI element instance
class album_grid_instance : public ui_element_instance, public playlist_callback_single {
private:
    HWND m_hwnd;
    HWND m_parent;
    HWND m_tooltip;  // Tooltip window
    HWND m_search_box;  // Search/filter text box
    std::vector<std::unique_ptr<grid_item>> m_items;
    std::vector<int> m_filtered_indices;  // Indices of filtered items
    pfc::string8 m_search_text;  // Current search filter
    bool m_search_visible;  // Is search box visible
    int m_scroll_pos;
    std::set<int> m_selected_indices;  // Multi-select support
    int m_last_selected;  // For shift-select
    int m_hover_index;
    int m_tooltip_index;  // Index of item tooltip is showing for
    service_ptr_t<ui_element_instance_callback> m_callback;
    grid_config m_config;
    fb2k::CCoreDarkModeHooks m_dark;
    bool m_tracking;
    
    // Visible range for lazy loading
    int m_first_visible;
    int m_last_visible;
    
    // Timer for deferred loading
    static const UINT_PTR TIMER_LOAD = 1;
    static const UINT_PTR TIMER_PROGRESSIVE = 2;
    
    static const int PADDING = 8;
    
    // Calculated layout values
    int m_item_size;  // Calculated item size based on columns and width
    int m_font_height;  // Actual font height from foobar2000
    
    // Playlist refresh timer
    static const UINT_PTR TIMER_REFRESH = 3;
    
public:
    album_grid_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr callback)
        : m_callback(callback), m_hwnd(NULL), m_parent(NULL), m_tooltip(NULL), m_search_box(NULL),
          m_search_visible(false), m_scroll_pos(0), 
          m_last_selected(-1), m_hover_index(-1), m_tooltip_index(-1), m_tracking(false),
          m_first_visible(0), m_last_visible(0), m_item_size(120), m_font_height(14) {
        m_config.load(config);
        
        // Register for playlist callbacks
        static_api_ptr_t<playlist_manager> pm;
        pm->register_callback(this, playlist_callback::flag_on_items_added | 
                                   playlist_callback::flag_on_items_removed |
                                   playlist_callback::flag_on_items_reordered);
    }
    
    ~album_grid_instance() {
        // Unregister playlist callbacks
        static_api_ptr_t<playlist_manager> pm;
        pm->unregister_callback(this);
        
        if (m_hwnd && IsWindow(m_hwnd)) {
            DestroyWindow(m_hwnd);
        }
    }
    
    void initialize_window(HWND parent) {
        m_parent = parent;
        
        // Register window class
        static const TCHAR* class_name = TEXT("AlbumGridV8");
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
        
        // Apply dark mode to window and controls
        if (m_hwnd) {
            // Create tooltip window
            m_tooltip = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TRANSPARENT,
                TOOLTIPS_CLASS, NULL,
                WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                m_hwnd, NULL, core_api::get_my_instance(), NULL);
            
            if (m_tooltip) {
                SetWindowPos(m_tooltip, HWND_TOPMOST, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                
                TOOLINFO ti = {};
                ti.cbSize = sizeof(TOOLINFO);
                ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
                ti.hwnd = m_hwnd;
                ti.uId = 0;
                ti.lpszText = const_cast<LPTSTR>(TEXT(""));
                SendMessage(m_tooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
                SendMessage(m_tooltip, TTM_SETMAXTIPWIDTH, 0, 300);
            }
            
            // Use foobar2000's dark mode hooks
            m_dark.AddDialogWithControls(m_hwnd);
            
            // Additionally, if we're in dark mode, try to apply theme to scrollbar
            if (m_dark) {  // Check if dark mode is enabled using our hooks
                // Force Windows to use dark scrollbars by setting appropriate theme
                SetWindowTheme(m_hwnd, L"DarkMode_Explorer", nullptr);
            }
        }
        
        // Start loading after a short delay
        SetTimer(m_hwnd, TIMER_LOAD, 100, NULL);
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
                case WM_LBUTTONDOWN: return instance->on_lbuttondown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), wp);
                case WM_LBUTTONDBLCLK: return instance->on_lbuttondblclk(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_RBUTTONDOWN: return instance->on_rbuttondown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_MOUSEMOVE: return instance->on_mousemove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_MOUSELEAVE: return instance->on_mouseleave();
                case WM_ERASEBKGND: return 1;
                case WM_CONTEXTMENU: return instance->on_contextmenu(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                case WM_TIMER: return instance->on_timer(wp);
                case WM_KEYDOWN: return instance->on_keydown(wp);
                case WM_COMMAND: return instance->on_command(LOWORD(wp), HIWORD(wp));
                case WM_DESTROY: {
                    KillTimer(hwnd, TIMER_LOAD);
                    KillTimer(hwnd, TIMER_PROGRESSIVE);
                    return 0;
                }
            }
        }
        
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    
    LRESULT on_timer(WPARAM timer_id) {
        if (timer_id == TIMER_LOAD) {
            KillTimer(m_hwnd, TIMER_LOAD);
            refresh_items();
            SetTimer(m_hwnd, TIMER_PROGRESSIVE, 50, NULL);
        } else if (timer_id == TIMER_PROGRESSIVE) {
            load_visible_artwork();
            bool all_loaded = true;
            for (int i = m_first_visible; i <= m_last_visible && i < (int)m_items.size(); i++) {
                if (!m_items[i]->thumbnail->bitmap && !m_items[i]->thumbnail->loading) {
                    all_loaded = false;
                    break;
                }
            }
            if (all_loaded) {
                KillTimer(m_hwnd, TIMER_PROGRESSIVE);
            }
        } else if (timer_id == TIMER_REFRESH) {
            // Refresh playlist view periodically
            if (m_config.view == grid_config::VIEW_PLAYLIST) {
                refresh_items();
            }
        }
        return 0;
    }
    
    LRESULT on_command(WORD id, WORD notify_code) {
        if (id == 1001 && notify_code == EN_CHANGE) {
            // Search box text changed
            on_search_text_changed();
        }
        return 0;
    }
    
    LRESULT on_keydown(WPARAM key) {
        if (key == 'P') {
            // Toggle between library and playlist view
            m_config.view = (m_config.view == grid_config::VIEW_LIBRARY) 
                            ? grid_config::VIEW_PLAYLIST 
                            : grid_config::VIEW_LIBRARY;
            
            // Update refresh timer
            if (m_config.view == grid_config::VIEW_PLAYLIST) {
                SetTimer(m_hwnd, TIMER_REFRESH, 2000, NULL);
            } else {
                KillTimer(m_hwnd, TIMER_REFRESH);
            }
            
            refresh_items();
            return 0;
        } else if (key == VK_F5) {
            refresh_items();
            return 0;
        } else if (GetKeyState(VK_CONTROL) & 0x8000) {
            if (key == 'A') {
                // Select all
                m_selected_indices.clear();
                size_t count = get_item_count();
                for (size_t i = 0; i < count; i++) {
                    m_selected_indices.insert(i);
                }
                InvalidateRect(m_hwnd, NULL, FALSE);
                return 0;
            } else if (key == 'S' && (GetKeyState(VK_SHIFT) & 0x8000)) {
                // Toggle search box on Ctrl+Shift+S
                toggle_search(!m_search_visible);
                return 0;
            }
        }
        return 0;
    }
    
    void calculate_layout() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int width = rc.right;
        
        if (width <= 0 || m_config.columns <= 0) {
            m_item_size = 120;
            return;
        }
        
        // Calculate item size to fill width with desired columns
        // Total width = columns * item_size + (columns + 1) * padding
        int total_padding = PADDING * (m_config.columns + 1);
        int available_for_items = width - total_padding;
        m_item_size = max(50, min(250, available_for_items / m_config.columns));
    }
    
    int calculate_text_height() {
        if (!m_config.show_text || !m_callback.is_valid()) {
            return 0;
        }
        
        // Get font from foobar2000
        HFONT hFont = m_callback->query_font_ex(ui_font_default);
        if (!hFont) {
            hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
        
        // Get font metrics
        HDC hdc = GetDC(m_hwnd);
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        SelectObject(hdc, oldFont);
        ReleaseDC(m_hwnd, hdc);
        
        // Calculate height based on actual font metrics
        // Use actual line height from font
        int line_height = tm.tmHeight + tm.tmExternalLeading;
        
        // For multi-line text, add appropriate spacing
        int total_height;
        if (m_config.text_lines == 1) {
            total_height = line_height + 6;  // Single line with some padding
        } else if (m_config.text_lines == 2) {
            total_height = (line_height * 2) + 8;  // Two lines with spacing
        } else {
            total_height = (line_height * 3) + 10;  // Three lines with spacing
        }
        
        return total_height;
    }
    
    void refresh_items() {
        m_items.clear();
        m_selected_indices.clear();
        
        metadb_handle_list all_items;
        
        // Get items based on view mode
        if (m_config.view == grid_config::VIEW_PLAYLIST) {
            // Get items from current playlist
            auto pm = playlist_manager::get();
            t_size active = pm->get_active_playlist();
            if (active != pfc::infinite_size) {
                pm->playlist_get_all_items(active, all_items);
            }
        } else {
            // Get items from media library
            auto lib = library_manager::get();
            lib->get_all_items(all_items);
        }
        
        // Group items based on mode
        std::map<pfc::string8, std::unique_ptr<grid_item>> item_map;
        
        for (t_size i = 0; i < all_items.get_count(); i++) {
            auto handle = all_items[i];
            
            pfc::string8 key;
            pfc::string8 display_name;
            
            if (m_config.grouping == grid_config::GROUP_BY_FOLDER) {
                // Group by folder
                pfc::string8 path = handle->get_path();
                const char* last_slash = strrchr(path.c_str(), '\\');
                if (!last_slash) last_slash = strrchr(path.c_str(), '/');
                
                if (last_slash && last_slash > path.c_str()) {
                    const char* start = path.c_str();
                    const char* second_last = last_slash - 1;
                    while (second_last > start && *second_last != '\\' && *second_last != '/') {
                        second_last--;
                    }
                    if (*second_last == '\\' || *second_last == '/') {
                        second_last++;
                    }
                    key.set_string(second_last, last_slash - second_last);
                    display_name = key;
                }
                
                if (key.is_empty()) {
                    key = "Unknown Folder";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_ALBUM) {
                // Group by album - use titleformat for better results
                static_api_ptr_t<titleformat_compiler> compiler;
                titleformat_object::ptr script;
                compiler->compile_safe(script, "[%album artist%]|[%artist%]|[%album%]");
                
                pfc::string8 formatted;
                handle->format_title(NULL, formatted, script, NULL);
                
                // Parse the formatted result
                pfc::list_t<pfc::string8> parts;
                pfc::splitStringSimple_toList(parts, "|", formatted);
                
                pfc::string8 album_artist, artist, album;
                if (parts.get_count() > 0 && !parts[0].is_empty()) album_artist = parts[0];
                if (parts.get_count() > 1 && !parts[1].is_empty()) artist = parts[1];
                if (parts.get_count() > 2 && !parts[2].is_empty()) album = parts[2];
                
                if (!album.is_empty()) {
                    // Use album artist if available, otherwise use artist
                    pfc::string8 primary_artist = !album_artist.is_empty() ? album_artist : artist;
                    
                    if (!primary_artist.is_empty()) {
                        key << primary_artist << " - " << album;
                        display_name = album;
                    } else {
                        key = album;
                        display_name = album;
                    }
                } else {
                    key = "Unknown Album";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_ARTIST) {
                // Group by artist
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* artist = info.meta_get("ARTIST", 0);
                    if (!artist || !artist[0]) artist = info.meta_get("ALBUM ARTIST", 0);
                    if (artist && artist[0]) {
                        key = artist;
                        display_name = artist;
                    }
                }
                if (key.is_empty()) {
                    key = "Unknown Artist";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_ARTIST_ALBUM) {
                // Group by artist/album combination
                static_api_ptr_t<titleformat_compiler> compiler;
                titleformat_object::ptr script;
                compiler->compile_safe(script, "[%album artist%]|[%artist%]|[%album%]");
                
                pfc::string8 formatted;
                handle->format_title(NULL, formatted, script, NULL);
                
                pfc::list_t<pfc::string8> parts;
                pfc::splitStringSimple_toList(parts, "|", formatted);
                
                pfc::string8 album_artist, artist, album;
                if (parts.get_count() > 0 && !parts[0].is_empty()) album_artist = parts[0];
                if (parts.get_count() > 1 && !parts[1].is_empty()) artist = parts[1];
                if (parts.get_count() > 2 && !parts[2].is_empty()) album = parts[2];
                
                pfc::string8 primary_artist = !album_artist.is_empty() ? album_artist : artist;
                
                if (!primary_artist.is_empty() && !album.is_empty()) {
                    key << primary_artist << " - " << album;
                    display_name << primary_artist << " - " << album;
                } else if (!primary_artist.is_empty()) {
                    key = primary_artist;
                    display_name = primary_artist;
                } else if (!album.is_empty()) {
                    key = album;
                    display_name = album;
                } else {
                    key = "Unknown";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_GENRE) {
                // Group by genre
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* genre = info.meta_get("GENRE", 0);
                    if (genre && genre[0]) {
                        key = genre;
                        display_name = genre;
                    }
                }
                if (key.is_empty()) {
                    key = "Unknown Genre";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_YEAR) {
                // Group by year
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* date = info.meta_get("DATE", 0);
                    if (!date || !date[0]) date = info.meta_get("YEAR", 0);
                    if (date && date[0]) {
                        // Extract year from date (might be YYYY-MM-DD or just YYYY)
                        pfc::string8 year;
                        if (strlen(date) >= 4) {
                            year.set_string(date, 4);
                            key = year;
                            display_name = year;
                        }
                    }
                }
                if (key.is_empty()) {
                    key = "Unknown Year";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_LABEL) {
                // Group by record label
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* label = info.meta_get("LABEL", 0);
                    if (!label || !label[0]) label = info.meta_get("PUBLISHER", 0);
                    if (label && label[0]) {
                        key = label;
                        display_name = label;
                    }
                }
                if (key.is_empty()) {
                    key = "Unknown Label";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_COMPOSER) {
                // Group by composer
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* composer = info.meta_get("COMPOSER", 0);
                    if (composer && composer[0]) {
                        key = composer;
                        display_name = composer;
                    }
                }
                if (key.is_empty()) {
                    key = "Unknown Composer";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_PERFORMER) {
                // Group by performer
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* performer = info.meta_get("PERFORMER", 0);
                    if (!performer || !performer[0]) performer = info.meta_get("ARTIST", 0);
                    if (performer && performer[0]) {
                        key = performer;
                        display_name = performer;
                    }
                }
                if (key.is_empty()) {
                    key = "Unknown Performer";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_ALBUM_ARTIST) {
                // Group specifically by album artist
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* album_artist = info.meta_get("ALBUM ARTIST", 0);
                    if (!album_artist || !album_artist[0]) album_artist = info.meta_get("ALBUMARTIST", 0);
                    if (!album_artist || !album_artist[0]) album_artist = info.meta_get("ARTIST", 0);
                    if (album_artist && album_artist[0]) {
                        key = album_artist;
                        display_name = album_artist;
                    }
                }
                if (key.is_empty()) {
                    key = "Unknown Album Artist";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_DIRECTORY) {
                // Group by parent directory name (not full path)
                pfc::string8 path = handle->get_path();
                const char* last_slash = strrchr(path.c_str(), '\\');
                if (!last_slash) last_slash = strrchr(path.c_str(), '/');
                if (last_slash && last_slash != path.c_str()) {
                    const char* start = path.c_str();
                    const char* end = last_slash;
                    // Find the second-to-last slash
                    const char* prev_slash = nullptr;
                    for (const char* p = start; p < end; p++) {
                        if (*p == '\\' || *p == '/') {
                            prev_slash = p;
                        }
                    }
                    if (prev_slash) {
                        key.set_string(prev_slash + 1, end - prev_slash - 1);
                        display_name = key;
                    }
                }
                if (key.is_empty()) {
                    key = "Root Directory";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_COMMENT) {
                // Group by comment field
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* comment = info.meta_get("COMMENT", 0);
                    if (comment && comment[0]) {
                        // Truncate long comments for grouping
                        if (strlen(comment) > 50) {
                            key.set_string(comment, 50);
                            key << "...";
                            display_name = key;
                        } else {
                            key = comment;
                            display_name = comment;
                        }
                    }
                }
                if (key.is_empty()) {
                    key = "No Comment";
                    display_name = key;
                }
            } else if (m_config.grouping == grid_config::GROUP_BY_RATING) {
                // Group by rating
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* rating = info.meta_get("RATING", 0);
                    if (rating && rating[0]) {
                        int rating_val = atoi(rating);
                        if (rating_val > 0) {
                            key << rating_val << " Stars";
                            display_name = key;
                        }
                    }
                }
                if (key.is_empty()) {
                    key = "Unrated";
                    display_name = key;
                }
            }
            
            if (item_map.find(key) == item_map.end()) {
                auto item = std::make_unique<grid_item>();
                item->display_name = display_name;
                item->sort_key = key;
                item->path = handle->get_path();
                
                // Get file date and size
                t_filestats stats = handle->get_filestats();
                item->newest_date = stats.m_timestamp;
                item->total_size = stats.m_size;
                
                // Get metadata for sorting
                file_info_impl info;
                if (handle->get_info(info)) {
                    const char* artist = info.meta_get("ARTIST", 0);
                    if (!artist || !artist[0]) artist = info.meta_get("ALBUM ARTIST", 0);
                    if (artist) item->artist = artist;
                    
                    const char* album = info.meta_get("ALBUM", 0);
                    if (album) item->album = album;
                    
                    const char* genre = info.meta_get("GENRE", 0);
                    if (genre) item->genre = genre;
                    
                    const char* date = info.meta_get("DATE", 0);
                    if (!date || !date[0]) date = info.meta_get("YEAR", 0);
                    if (date && strlen(date) >= 4) {
                        item->year.set_string(date, 4);
                    }
                    
                    const char* rating = info.meta_get("RATING", 0);
                    if (rating) item->rating = atoi(rating);
                }
                
                item_map[key] = std::move(item);
            }
            
            item_map[key]->tracks.add_item(handle);
            
            // Track newest date and total size
            t_filestats stats = handle->get_filestats();
            if (stats.m_timestamp > item_map[key]->newest_date) {
                item_map[key]->newest_date = stats.m_timestamp;
            }
            item_map[key]->total_size += stats.m_size;
        }
        
        // Convert map to vector
        for (auto& pair : item_map) {
            m_items.push_back(std::move(pair.second));
        }
        
        // Sort items
        sort_items();
        
        // Update scrollbar and repaint
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void sort_items() {
        switch (m_config.sorting) {
            case grid_config::SORT_BY_NAME:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        return pfc::stricmp_ascii(a->sort_key.c_str(), b->sort_key.c_str()) < 0;
                    });
                break;
            case grid_config::SORT_BY_DATE:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        return a->newest_date > b->newest_date;
                    });
                break;
            case grid_config::SORT_BY_TRACK_COUNT:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        return a->tracks.get_count() > b->tracks.get_count();
                    });
                break;
            case grid_config::SORT_BY_ARTIST:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        return pfc::stricmp_ascii(a->artist.c_str(), b->artist.c_str()) < 0;
                    });
                break;
            case grid_config::SORT_BY_ALBUM:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        return pfc::stricmp_ascii(a->album.c_str(), b->album.c_str()) < 0;
                    });
                break;
            case grid_config::SORT_BY_YEAR:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        // Sort by year descending (newest first)
                        return pfc::stricmp_ascii(a->year.c_str(), b->year.c_str()) > 0;
                    });
                break;
            case grid_config::SORT_BY_GENRE:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        return pfc::stricmp_ascii(a->genre.c_str(), b->genre.c_str()) < 0;
                    });
                break;
            case grid_config::SORT_BY_PATH:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        return pfc::stricmp_ascii(a->path.c_str(), b->path.c_str()) < 0;
                    });
                break;
            case grid_config::SORT_BY_RANDOM:
                {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::shuffle(m_items.begin(), m_items.end(), gen);
                }
                break;
            case grid_config::SORT_BY_SIZE:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        return a->total_size > b->total_size;
                    });
                break;
            case grid_config::SORT_BY_RATING:
                std::sort(m_items.begin(), m_items.end(), 
                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {
                        return a->rating > b->rating;
                    });
                break;
        }
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
        
        // Create high-quality thumbnail with proper aspect ratio
        int orig_width = original->GetWidth();
        int orig_height = original->GetHeight();
        
        // Calculate proper scaling to maintain aspect ratio
        float scale = min((float)size / orig_width, (float)size / orig_height);
        int new_width = (int)(orig_width * scale);
        int new_height = (int)(orig_height * scale);
        int x_offset = (size - new_width) / 2;
        int y_offset = (size - new_height) / 2;
        
        Gdiplus::Bitmap* thumbnail = new Gdiplus::Bitmap(size, size, PixelFormat32bppARGB);
        Gdiplus::Graphics graphics(thumbnail);
        
        // Set highest quality rendering
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
        
        // Clear background (transparent)
        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
        
        // Draw image centered with aspect ratio preserved
        graphics.DrawImage(original, x_offset, y_offset, new_width, new_height);
        
        delete original;
        return thumbnail;
    }
    
    void toggle_search(bool show) {
        m_search_visible = show;
        
        if (show) {
            // Create search box if it doesn't exist
            if (!m_search_box) {
                m_search_box = CreateWindowEx(
                    WS_EX_CLIENTEDGE,
                    TEXT("EDIT"),
                    TEXT(""),
                    WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                    5, 5, 200, 25,
                    m_hwnd,
                    (HMENU)1001,  // Control ID for search box
                    core_api::get_my_instance(),
                    NULL
                );
                
                if (m_search_box) {
                    // Apply dark mode to search box
                    m_dark.AddDialogWithControls(m_search_box);
                    
                    // Set placeholder text
                    SendMessage(m_search_box, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search albums... (Ctrl+Shift+S to close)");
                    
                    // Subclass the edit control to handle Enter key
                    SetWindowSubclass(m_search_box, SearchBoxProc, 0, (DWORD_PTR)this);
                }
            }
            
            if (m_search_box) {
                ShowWindow(m_search_box, SW_SHOW);
                
                // Position search box at top right
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                SetWindowPos(m_search_box, NULL, rc.right - 305, 5, 300, 25, SWP_NOZORDER);
                
                // Set focus to search box after positioning
                SetFocus(m_search_box);
            }
        } else {
            // Hide search box
            if (m_search_box) {
                ShowWindow(m_search_box, SW_HIDE);
                
                // Clear the search box text
                SetWindowText(m_search_box, TEXT(""));
                
                // Clear search and refresh
                m_search_text.reset();
                apply_filter();
                InvalidateRect(m_hwnd, NULL, FALSE);
                
                // Ensure grid window has focus
                SetFocus(m_hwnd);
                // Force the window to be active
                SetActiveWindow(m_hwnd);
            }
        }
    }
    
    static LRESULT CALLBACK SearchBoxProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR dwRefData) {
        album_grid_instance* instance = (album_grid_instance*)dwRefData;
        
        switch (msg) {
            case WM_KEYDOWN:
                // Check for Ctrl+Shift+S in the search box
                if (wp == 'S' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
                    instance->toggle_search(false);
                    return 0;
                }
                break;
        }
        
        // Call default procedure
        return DefSubclassProc(hwnd, msg, wp, lp);
    }
    
    void on_search_text_changed() {
        if (!m_search_box) return;
        
        // Get text from search box
        int len = GetWindowTextLength(m_search_box);
        if (len > 0) {
            wchar_t* buffer = new wchar_t[len + 1];
            GetWindowText(m_search_box, buffer, len + 1);
            pfc::stringcvt::string_utf8_from_os text_utf8(buffer);
            m_search_text = text_utf8.get_ptr();
            delete[] buffer;
        } else {
            m_search_text.reset();
        }
        
        // Apply filter and refresh
        apply_filter();
        m_scroll_pos = 0;  // Reset scroll to top
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    // Helper functions to work with filtered/unfiltered items
    size_t get_item_count() const {
        if (!m_search_text.is_empty() && !m_filtered_indices.empty()) {
            return m_filtered_indices.size();
        }
        return m_items.size();
    }
    
    grid_item* get_item_at(size_t index) {
        if (!m_search_text.is_empty() && !m_filtered_indices.empty()) {
            if (index < m_filtered_indices.size()) {
                return m_items[m_filtered_indices[index]].get();
            }
            return nullptr;
        }
        if (index < m_items.size()) {
            return m_items[index].get();
        }
        return nullptr;
    }
    
    size_t get_real_index(size_t display_index) {
        if (!m_search_text.is_empty() && !m_filtered_indices.empty()) {
            if (display_index < m_filtered_indices.size()) {
                return m_filtered_indices[display_index];
            }
            return SIZE_MAX;
        }
        return display_index;
    }
    
    void apply_filter() {
        m_filtered_indices.clear();
        
        if (m_search_text.is_empty()) {
            // No filter, show all items
            return;
        }
        
        // Convert search text to lowercase for case-insensitive search
        pfc::string8 search_lower = m_search_text;
        for (size_t i = 0; i < search_lower.length(); i++) {
            char c = search_lower[i];
            if (c >= 'A' && c <= 'Z') {
                search_lower.set_char(i, c + 32);  // Convert to lowercase
            }
        }
        
        // Filter items that match the search text
        for (size_t idx = 0; idx < m_items.size(); idx++) {
            auto& item = m_items[idx];
            pfc::string8 item_text = item->display_name;
            
            // Also search in artist, album, genre fields
            item_text << " " << item->artist << " " << item->album << " " << item->genre;
            
            // Convert to lowercase for comparison
            pfc::string8 item_lower = item_text;
            for (size_t i = 0; i < item_lower.length(); i++) {
                char c = item_lower[i];
                if (c >= 'A' && c <= 'Z') {
                    item_lower.set_char(i, c + 32);  // Convert to lowercase
                }
            }
            
            // Check if search text is contained in item text
            if (strstr(item_lower.c_str(), search_lower.c_str()) != nullptr) {
                m_filtered_indices.push_back(idx);
            }
        }
    }
    
    void load_visible_artwork() {
        int load_count = 0;
        size_t item_count = get_item_count();
        
        for (int i = m_first_visible; i <= m_last_visible && i < (int)item_count && load_count < 5; i++) {
            auto* item = get_item_at(i);
            if (!item) continue;
            
            if (item->thumbnail->bitmap || item->thumbnail->loading) continue;
            if (item->tracks.get_count() == 0) continue;
            
            item->thumbnail->loading = true;
            load_count++;
            
            try {
                auto art_api = album_art_manager_v2::get();
                abort_callback_dummy abort;
                auto extractor = art_api->open(
                    pfc::list_single_ref_t<metadb_handle_ptr>(item->tracks[0]),
                    pfc::list_single_ref_t<GUID>(album_art_ids::cover_front),
                    abort
                );
                
                try {
                    // Try artist image for artist-based groupings
                    if (m_config.grouping == grid_config::GROUP_BY_ARTIST || 
                        m_config.grouping == grid_config::GROUP_BY_ALBUM_ARTIST || 
                        m_config.grouping == grid_config::GROUP_BY_ARTIST_ALBUM ||
                        m_config.grouping == grid_config::GROUP_BY_PERFORMER ||
                        m_config.grouping == grid_config::GROUP_BY_COMPOSER) {
                        try {
                            auto artist_ext = art_api->open(
                                pfc::list_single_ref_t<metadb_handle_ptr>(item->tracks[0]),
                                pfc::list_single_ref_t<GUID>(album_art_ids::artist),
                                abort
                            );
                            item->artwork = artist_ext->query(album_art_ids::artist, abort);
                        } catch (...) {
                            // No artist image, try cover below
                        }
                    }
                    
                    // Fallback to front cover
                    if (!item->artwork.is_valid()) {
                        item->artwork = extractor->query(album_art_ids::cover_front, abort);
                    }
                    if (item->artwork.is_valid()) {
                        calculate_layout();  // Update m_item_size
                        if (item->thumbnail->bitmap) {
                            delete item->thumbnail->bitmap;
                        }
                        item->thumbnail->bitmap = create_thumbnail(item->artwork, m_item_size);
                        item->thumbnail->cached_size = m_item_size;
                        InvalidateRect(m_hwnd, NULL, FALSE);
                    }
                } catch (...) {}
            } catch (...) {}
            
            item->thumbnail->loading = false;
            item->thumbnail->last_access = GetTickCount();
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
        
        // View mode indicator removed - now using context menu instead
        
        // Setup GDI+ with high quality settings
        Gdiplus::Graphics graphics(memdc);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
        
        // Calculate grid layout
        calculate_layout();  // Update m_item_size based on current width
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        int visible_rows = (rc.bottom + item_total_height + PADDING - 1) / (item_total_height + PADDING) + 1;
        int first_row = m_scroll_pos / (item_total_height + PADDING);
        int y_offset = -(m_scroll_pos % (item_total_height + PADDING));
        
        // Calculate visible range
        size_t item_count = get_item_count();
        m_first_visible = first_row * cols;
        m_last_visible = min((int)item_count - 1, (first_row + visible_rows) * cols);
        
        // Load artwork for visible items
        load_visible_artwork();
        
        // Restart progressive loading if needed
        bool needs_loading = false;
        for (int i = m_first_visible; i <= m_last_visible && i < (int)item_count; i++) {
            auto* item = get_item_at(i);
            if (item && !item->thumbnail->bitmap && !item->thumbnail->loading) {
                needs_loading = true;
                break;
            }
        }
        if (needs_loading) {
            SetTimer(m_hwnd, TIMER_PROGRESSIVE, 50, NULL);
        }
        
        // Get theme colors
        t_ui_color color_text = m_callback->query_std_color(ui_color_text);
        t_ui_color color_selected = m_callback->query_std_color(ui_color_selection);
        
        // Use foobar2000's standard font
        HFONT hFont = m_callback->query_font_ex(ui_font_default);
        if (!hFont) {
            // Fallback to system font if query fails
            hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
        
        SetBkMode(memdc, TRANSPARENT);
        
        // Draw items - centered in window
        int total_width = cols * m_item_size + (cols + 1) * PADDING;
        int start_x = (rc.right - total_width) / 2;
        if (start_x < 0) start_x = 0;
        
        for (int row = 0; row < visible_rows; row++) {
            for (int col = 0; col < cols; col++) {
                size_t index = (first_row + row) * cols + col;
                if (index >= item_count) break;
                
                int x = start_x + col * (m_item_size + PADDING) + PADDING;
                int y = row * (item_total_height + PADDING) + PADDING + y_offset;
                
                if (y + item_total_height > 0 && y < rc.bottom) {
                    draw_item(memdc, graphics, hFont, x, y, index, color_text, color_selected, text_height);
                }
            }
        }
        
        // Don't delete the font - it's managed by foobar2000
        
        // Draw status text if loading or filtered
        if (item_count == 0) {
            SetTextColor(memdc, color_text);
            HFONT status_font = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
            SelectObject(memdc, status_font);
            
            // Show appropriate message based on view mode
            const TCHAR* message = TEXT("Loading library...");
            if (m_config.view == grid_config::VIEW_PLAYLIST) {
                message = TEXT("Playlist is empty");
            } else if (!m_search_text.is_empty()) {
                message = TEXT("No items match your search");
            }
            
            DrawText(memdc, message, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DeleteObject(status_font);
        }
        
        // Draw edit mode highlight border if in layout edit mode
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            // Draw a colored border to indicate edit mode
            HPEN edit_pen = CreatePen(PS_SOLID, 2, RGB(255, 128, 0));  // Orange border
            HPEN old_pen = (HPEN)SelectObject(memdc, edit_pen);
            HBRUSH old_brush = (HBRUSH)SelectObject(memdc, GetStockObject(NULL_BRUSH));
            
            Rectangle(memdc, 0, 0, rc.right, rc.bottom);
            
            SelectObject(memdc, old_pen);
            SelectObject(memdc, old_brush);
            DeleteObject(edit_pen);
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
    
    void draw_track_count_badge(HDC hdc, int count, int x, int y, bool dark_mode, HFONT app_font) {
        // Create text for the badge
        char count_str[32];
        sprintf_s(count_str, "%d", count);
        
        // Use the exact same font as the app text labels
        HFONT old_font = (HFONT)SelectObject(hdc, app_font);
        
        // Calculate badge size based on text with the actual font
        SIZE text_size;
        GetTextExtentPoint32A(hdc, count_str, strlen(count_str), &text_size);
        
        // Dynamic padding based on actual text size
        int padding_x = max(8, text_size.cy / 2);
        int padding_y = max(4, text_size.cy / 4);
        int badge_width = text_size.cx + padding_x;
        int badge_height = text_size.cy + padding_y;
        int badge_x = x - badge_width - 5;  // Top-right corner
        int badge_y = y + 5;
        
        // Create badge background
        COLORREF badge_bg = dark_mode ? RGB(60, 60, 60) : RGB(220, 50, 50);
        COLORREF badge_border = dark_mode ? RGB(80, 80, 80) : RGB(180, 40, 40);
        COLORREF text_color = dark_mode ? RGB(200, 200, 200) : RGB(255, 255, 255);
        
        // Draw badge background with rounded corners
        HBRUSH hbrBadge = CreateSolidBrush(badge_bg);
        HPEN hpenBadge = CreatePen(PS_SOLID, 1, badge_border);
        HPEN oldPen = (HPEN)SelectObject(hdc, hpenBadge);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hbrBadge);
        
        // Draw rounded rectangle
        int corner_radius = min(10, badge_height / 2);
        RoundRect(hdc, badge_x, badge_y, badge_x + badge_width, badge_y + badge_height, corner_radius, corner_radius);
        
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(hbrBadge);
        DeleteObject(hpenBadge);
        
        // Draw text
        RECT text_rc = {badge_x, badge_y, badge_x + badge_width, badge_y + badge_height};
        SetTextColor(hdc, text_color);
        SetBkMode(hdc, TRANSPARENT);
        
        DrawTextA(hdc, count_str, -1, &text_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        SelectObject(hdc, old_font);
        // Don't delete the font - it's managed by foobar2000
    }
    
    void draw_item(HDC hdc, Gdiplus::Graphics& graphics, HFONT font,
                   int x, int y, size_t index, 
                   t_ui_color color_text, t_ui_color color_selected, int text_height) {
        auto* item = get_item_at(index);
        if (!item) return;
        
        // Draw selection/hover background
        bool selected = m_selected_indices.find(index) != m_selected_indices.end();
        if (selected) {
            RECT sel_rc = {x-2, y-2, x + m_item_size + 2, y + m_item_size + text_height + 2};
            HBRUSH sel_brush = CreateSolidBrush(color_selected);
            FillRect(hdc, &sel_rc, sel_brush);
            DeleteObject(sel_brush);
        } else if ((int)index == m_hover_index) {
            RECT hover_rc = {x-1, y-1, x + m_item_size + 1, y + m_item_size + text_height + 1};
            HBRUSH hover_brush = CreateSolidBrush(RGB(60, 60, 60));
            FillRect(hdc, &hover_rc, hover_brush);
            DeleteObject(hover_brush);
        }
        
        // Draw thumbnail or placeholder
        if (item->thumbnail->bitmap) {
            graphics.DrawImage(item->thumbnail->bitmap, x, y, m_item_size, m_item_size);
            item->thumbnail->last_access = GetTickCount();
        } else {
            // Draw placeholder
            RECT placeholder_rc = {x, y, x + m_item_size, y + m_item_size};
            
            for (int i = 0; i < 3; i++) {
                RECT gradient_rc = {x + i, y + i, x + m_item_size - i, y + m_item_size - i};
                int color_value = 35 + i * 5;
                HBRUSH gradient_brush = CreateSolidBrush(RGB(color_value, color_value, color_value));
                FillRect(hdc, &gradient_rc, gradient_brush);
                DeleteObject(gradient_brush);
            }
            
            // Draw icon
            SetTextColor(hdc, RGB(80, 80, 80));
            HFONT icon_font = CreateFont(m_item_size / 4, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI Symbol"));
            HFONT old_font = (HFONT)SelectObject(hdc, icon_font);
            
            if (item->thumbnail->loading) {
                DrawText(hdc, TEXT("⏳"), -1, &placeholder_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            } else if (m_config.grouping == grid_config::GROUP_BY_FOLDER) {
                DrawText(hdc, TEXT("📁"), -1, &placeholder_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            } else {
                DrawText(hdc, TEXT("🎵"), -1, &placeholder_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            
            SelectObject(hdc, old_font);
            DeleteObject(icon_font);
        }
        
        // Draw subtle border
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(50, 50, 50));
        HPEN oldpen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, x, y, x + m_item_size, y + m_item_size);
        SelectObject(hdc, oldpen);
        SelectObject(hdc, oldbrush);
        DeleteObject(pen);
        
        // Draw track count badge when track count is enabled (always show as badge)
        if (m_config.show_track_count && item->tracks.get_count() > 0) {
            // Check if dark mode is enabled using our hooks
            bool dark_mode = m_dark ? true : false;
            draw_track_count_badge(hdc, item->tracks.get_count(), x + m_item_size, y, dark_mode, font);
        }
        
        // Draw text (multi-line support)
        if (m_config.show_text) {
            RECT text_rc = {x - 2, y + m_item_size + 4, x + m_item_size + 2, 
                           y + m_item_size + text_height - 2};
            SetTextColor(hdc, color_text);
            SelectObject(hdc, font);
            
            // Prepare text with proper UTF-8 to UTF-16 conversion
            pfc::stringcvt::string_os_from_utf8 display_text(item->display_name.c_str());
            
            // Track count is now always shown as badge, not in text
            
            // Draw with word wrap for multi-line support
            UINT format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL;
            if (m_config.text_lines == 1) {
                format = DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_VCENTER;
            } else if (m_config.text_lines == 2) {
                format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL;
            } else {
                // 3 lines - use top alignment to show as much as possible
                format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_TOP;
            }
            
            DrawText(hdc, display_text, -1, &text_rc, format);
        }
    }
    
    int hit_test(int mx, int my) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Use same layout calculation as paint
        calculate_layout();
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        
        // Use same centering as paint
        int total_width = cols * m_item_size + (cols + 1) * PADDING;
        int start_x = (rc.right - total_width) / 2;
        if (start_x < 0) start_x = 0;
        
        // Check if click is within grid bounds
        if (mx < start_x + PADDING || mx >= start_x + total_width - PADDING) {
            return -1;
        }
        
        // Calculate column
        int rel_x = mx - start_x - PADDING;
        int col = rel_x / (m_item_size + PADDING);
        
        // Check if click is in padding between columns
        int col_start = col * (m_item_size + PADDING);
        if (rel_x < col_start || rel_x >= col_start + m_item_size) {
            return -1;
        }
        
        if (col >= cols) {
            return -1;
        }
        
        // Calculate row
        int adjusted_y = my + m_scroll_pos - PADDING;
        if (adjusted_y < 0) {
            return -1;
        }
        
        int row = adjusted_y / (item_total_height + PADDING);
        
        // Check if click is in padding between rows
        int row_start = row * (item_total_height + PADDING);
        if (adjusted_y < row_start || adjusted_y >= row_start + item_total_height) {
            return -1;
        }
        
        int index = row * cols + col;
        
        size_t item_count = get_item_count();
        if (index >= 0 && index < (int)item_count) {
            return index;
        }
        
        return -1;
    }
    
    LRESULT on_lbuttondown(int x, int y, WPARAM keys) {
        // In edit mode, don't handle item selection - let the host handle element selection
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            SetFocus(m_hwnd);  // Make sure we have focus for element selection
            return 0;
        }
        
        int index = hit_test(x, y);
        if (index >= 0) {
            if (keys & MK_CONTROL) {
                // Ctrl+Click: Toggle selection
                if (m_selected_indices.find(index) != m_selected_indices.end()) {
                    m_selected_indices.erase(index);
                } else {
                    m_selected_indices.insert(index);
                }
                m_last_selected = index;
            } else if (keys & MK_SHIFT && m_last_selected >= 0) {
                // Shift+Click: Range selection
                m_selected_indices.clear();
                int start = min(m_last_selected, index);
                int end = max(m_last_selected, index);
                for (int i = start; i <= end; i++) {
                    m_selected_indices.insert(i);
                }
            } else {
                // Normal click: Single selection
                m_selected_indices.clear();
                m_selected_indices.insert(index);
                m_last_selected = index;
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else {
            // Click on empty space: Clear selection
            m_selected_indices.clear();
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        SetFocus(m_hwnd);
        return 0;
    }
    
    LRESULT on_lbuttondblclk(int x, int y) {
        // In edit mode, don't handle double-clicks
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            return 0;
        }
        
        int index = hit_test(x, y);
        size_t item_count = get_item_count();
        if (index >= 0 && index < (int)item_count) {
            static_api_ptr_t<playlist_manager> pm;
            t_size playlist = pm->get_active_playlist();
            
            // Determine which items to process (selected items or just the double-clicked one)
            std::vector<size_t> real_indices_to_process;
            if (!m_selected_indices.empty() && 
                m_selected_indices.find(index) != m_selected_indices.end()) {
                // Process all selected items - convert display indices to real indices
                for (int sel_index : m_selected_indices) {
                    if (sel_index < (int)item_count) {
                        size_t real_idx = get_real_index(sel_index);
                        if (real_idx < m_items.size()) {
                            real_indices_to_process.push_back(real_idx);
                        }
                    }
                }
            } else {
                // Process only the double-clicked item - convert display index to real index
                size_t real_idx = get_real_index(index);
                if (real_idx < m_items.size()) {
                    real_indices_to_process.push_back(real_idx);
                }
            }
            
            // Execute the configured double-click action
            switch (m_config.doubleclick) {
                case grid_config::DOUBLECLICK_PLAY: {
                    // In playlist view, don't clear the playlist - just play the selected items
                    if (playlist != pfc::infinite_size) {
                        if (m_config.view == grid_config::VIEW_PLAYLIST) {
                            // In playlist view: find all tracks from selected albums and play from first match
                            // Collect all tracks from selected items
                            metadb_handle_list all_selected_tracks;
                            for (size_t real_idx : real_indices_to_process) {
                                all_selected_tracks.add_items(m_items[real_idx]->tracks);
                            }
                            
                            if (all_selected_tracks.get_count() > 0) {
                                // Find the first position in playlist of any of the selected tracks
                                t_size playlist_count = pm->playlist_get_item_count(playlist);
                                t_size first_match_position = pfc::infinite_size;
                                
                                for (t_size i = 0; i < playlist_count; i++) {
                                    metadb_handle_ptr playlist_track;
                                    if (pm->playlist_get_item_handle(playlist_track, playlist, i)) {
                                        // Check if this playlist track is in our selected tracks
                                        for (t_size j = 0; j < all_selected_tracks.get_count(); j++) {
                                            if (playlist_track == all_selected_tracks[j]) {
                                                first_match_position = i;
                                                break;
                                            }
                                        }
                                        if (first_match_position != pfc::infinite_size) {
                                            break;
                                        }
                                    }
                                }
                                
                                // If we found a matching track, play from that position
                                if (first_match_position != pfc::infinite_size) {
                                    pm->set_playing_playlist(playlist);
                                    // Use playlist_execute_default_action to play from specific position
                                    pm->playlist_execute_default_action(playlist, first_match_position);
                                }
                            }
                        } else {
                            // In library view: clear playlist and add items as before
                            pm->playlist_clear(playlist);
                            for (size_t real_idx : real_indices_to_process) {
                                pm->playlist_add_items(playlist, m_items[real_idx]->tracks, bit_array_false());
                            }
                            pm->set_playing_playlist(playlist);
                            static_api_ptr_t<playback_control> pc;
                            pc->play_start();
                        }
                    }
                    break;
                }
                
                case grid_config::DOUBLECLICK_ADD_TO_CURRENT: {
                    // Add to current playlist
                    if (playlist != pfc::infinite_size) {
                        metadb_handle_list tracks_to_add;
                        for (size_t real_idx : real_indices_to_process) {
                            if (real_idx < m_items.size() && m_items[real_idx]->tracks.get_count() > 0) {
                                tracks_to_add.add_items(m_items[real_idx]->tracks);
                            }
                        }
                        if (tracks_to_add.get_count() > 0) {
                            pm->playlist_add_items(playlist, tracks_to_add, bit_array_false());
                        }
                    }
                    break;
                }
                
                case grid_config::DOUBLECLICK_ADD_TO_NEW: {
                    // Create new playlist and add items
                    metadb_handle_list tracks_to_add;
                    for (size_t real_idx : real_indices_to_process) {
                        if (real_idx < m_items.size() && m_items[real_idx]->tracks.get_count() > 0) {
                            tracks_to_add.add_items(m_items[real_idx]->tracks);
                        }
                    }
                    
                    if (tracks_to_add.get_count() > 0) {
                        t_size new_playlist = pm->create_playlist("New Playlist", pfc::infinite_size, pfc::infinite_size);
                        if (new_playlist != pfc::infinite_size) {
                            pm->playlist_add_items(new_playlist, tracks_to_add, bit_array_false());
                            pm->set_active_playlist(new_playlist);
                        }
                    }
                    break;
                }
            }
        }
        return 0;
    }
    
    LRESULT on_rbuttondown(int x, int y) {
        int index = hit_test(x, y);
        size_t item_count = get_item_count();
        if (index >= 0 && index < (int)item_count) {
            // If right-clicking on unselected item, select it
            if (m_selected_indices.find(index) == m_selected_indices.end()) {
                m_selected_indices.clear();
                m_selected_indices.insert(index);
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
    
    LRESULT on_contextmenu(int x, int y) {
        POINT pt = {x, y};
        if (x == -1 && y == -1) {
            GetCursorPos(&pt);
        }
        
        // In edit mode, let the host handle the context menu for UI element operations
        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {
            // Pass to default handler
            return DefWindowProc(m_hwnd, WM_CONTEXTMENU, 0, MAKELPARAM(pt.x, pt.y));
        }
        
        HMENU menu = CreatePopupMenu();
        
        // Direct play/add commands for selected items
        if (!m_selected_indices.empty()) {
            if (m_selected_indices.size() == 1) {
                AppendMenu(menu, MF_STRING, 1, TEXT("Play"));
            } else {
                pfc::string8 play_text;
                play_text << "Play " << m_selected_indices.size() << " items";
                pfc::stringcvt::string_os_from_utf8 play_text_w(play_text.c_str());
                AppendMenu(menu, MF_STRING, 1, play_text_w);
            }
            AppendMenu(menu, MF_STRING, 2, TEXT("Add to Current Playlist"));
            AppendMenu(menu, MF_STRING, 3, TEXT("Add to New Playlist"));
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        }
        
        // Double-Click Action configuration submenu
        HMENU doubleclick_menu = CreatePopupMenu();
        AppendMenu(doubleclick_menu, MF_STRING | (m_config.doubleclick == grid_config::DOUBLECLICK_PLAY ? MF_CHECKED : 0), 
                   110, TEXT("Play (clear playlist)"));
        AppendMenu(doubleclick_menu, MF_STRING | (m_config.doubleclick == grid_config::DOUBLECLICK_ADD_TO_CURRENT ? MF_CHECKED : 0), 
                   111, TEXT("Add to Current Playlist"));
        AppendMenu(doubleclick_menu, MF_STRING | (m_config.doubleclick == grid_config::DOUBLECLICK_ADD_TO_NEW ? MF_CHECKED : 0), 
                   112, TEXT("Add to New Playlist"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)doubleclick_menu, TEXT("Double-Click Action"));
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        
        // Grouping options
        HMENU group_menu = CreatePopupMenu();
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_FOLDER ? MF_CHECKED : 0), 
                   10, TEXT("By Folder"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_DIRECTORY ? MF_CHECKED : 0), 
                   11, TEXT("By Directory"));
        AppendMenu(group_menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_ALBUM ? MF_CHECKED : 0), 
                   12, TEXT("By Album"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_ARTIST ? MF_CHECKED : 0), 
                   13, TEXT("By Artist"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_ALBUM_ARTIST ? MF_CHECKED : 0), 
                   14, TEXT("By Album Artist"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_ARTIST_ALBUM ? MF_CHECKED : 0), 
                   15, TEXT("By Artist/Album"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_PERFORMER ? MF_CHECKED : 0), 
                   16, TEXT("By Performer"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_COMPOSER ? MF_CHECKED : 0), 
                   17, TEXT("By Composer"));
        AppendMenu(group_menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_GENRE ? MF_CHECKED : 0), 
                   18, TEXT("By Genre"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_YEAR ? MF_CHECKED : 0), 
                   19, TEXT("By Year"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_LABEL ? MF_CHECKED : 0), 
                   20, TEXT("By Label"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_RATING ? MF_CHECKED : 0), 
                   21, TEXT("By Rating"));
        AppendMenu(group_menu, MF_STRING | (m_config.grouping == grid_config::GROUP_BY_COMMENT ? MF_CHECKED : 0), 
                   22, TEXT("By Comment"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)group_menu, TEXT("Group"));
        
        // Sort options
        HMENU sort_menu = CreatePopupMenu();
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_NAME ? MF_CHECKED : 0), 
                   50, TEXT("By Name"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_ARTIST ? MF_CHECKED : 0), 
                   51, TEXT("By Artist"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_ALBUM ? MF_CHECKED : 0), 
                   52, TEXT("By Album"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_YEAR ? MF_CHECKED : 0), 
                   53, TEXT("By Year"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_GENRE ? MF_CHECKED : 0), 
                   54, TEXT("By Genre"));
        AppendMenu(sort_menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_DATE ? MF_CHECKED : 0), 
                   55, TEXT("By Date Modified"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_SIZE ? MF_CHECKED : 0), 
                   56, TEXT("By Total Size"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_TRACK_COUNT ? MF_CHECKED : 0), 
                   57, TEXT("By Track Count"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_RATING ? MF_CHECKED : 0), 
                   58, TEXT("By Rating"));
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_PATH ? MF_CHECKED : 0), 
                   59, TEXT("By Path"));
        AppendMenu(sort_menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_RANDOM ? MF_CHECKED : 0), 
                   69, TEXT("Random/Shuffle"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)sort_menu, TEXT("Sort"));
        
        // Column options
        HMENU col_menu = CreatePopupMenu();
        for (int i = 3; i <= 10; i++) {
            pfc::string8 text;
            text << i << " columns";
            pfc::stringcvt::string_os_from_utf8 text_w(text.c_str());
            AppendMenu(col_menu, MF_STRING | (m_config.columns == i ? MF_CHECKED : 0), 20 + i, text_w);
        }
        AppendMenu(menu, MF_POPUP, (UINT_PTR)col_menu, TEXT("Columns"));
        
        // Text options
        HMENU text_menu = CreatePopupMenu();
        AppendMenu(text_menu, MF_STRING | (m_config.text_lines == 1 ? MF_CHECKED : 0), 60, TEXT("Single Line"));
        AppendMenu(text_menu, MF_STRING | (m_config.text_lines == 2 ? MF_CHECKED : 0), 61, TEXT("Two Lines"));
        AppendMenu(text_menu, MF_STRING | (m_config.text_lines == 3 ? MF_CHECKED : 0), 62, TEXT("Three Lines"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)text_menu, TEXT("Text Lines"));
        
        AppendMenu(menu, MF_STRING | (m_config.show_text ? MF_CHECKED : 0), 35, TEXT("Show Labels"));
        AppendMenu(menu, MF_STRING | (m_config.show_track_count ? MF_CHECKED : 0), 36, TEXT("Show Track Count"));
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        
        // View Mode options
        HMENU view_menu = CreatePopupMenu();
        AppendMenu(view_menu, MF_STRING | (m_config.view == grid_config::VIEW_LIBRARY ? MF_CHECKED : 0), 
                   100, TEXT("Library"));
        AppendMenu(view_menu, MF_STRING | (m_config.view == grid_config::VIEW_PLAYLIST ? MF_CHECKED : 0), 
                   101, TEXT("Current Playlist"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)view_menu, TEXT("View Mode"));
        
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu, MF_STRING | (m_search_visible ? MF_CHECKED : 0), 42, TEXT("Search (Ctrl+Shift+S)"));
        AppendMenu(menu, MF_STRING, 40, TEXT("Select All (Ctrl+A)"));
        AppendMenu(menu, MF_STRING, 41, TEXT("Refresh (F5)"));
        
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, 
            pt.x, pt.y, 0, m_hwnd, NULL);
        DestroyMenu(group_menu);
        DestroyMenu(sort_menu);
        DestroyMenu(col_menu);
        DestroyMenu(text_menu);
        DestroyMenu(view_menu);
        DestroyMenu(doubleclick_menu);
        DestroyMenu(menu);
        
        static_api_ptr_t<playlist_manager> pm;
        t_size playlist = pm->get_active_playlist();
        
        bool config_changed = false;
        bool needs_refresh = false;
        bool needs_sort = false;
        
        switch (cmd) {
            case 1: // Play
                if (playlist != pfc::infinite_size && !m_selected_indices.empty()) {
                    if (m_config.view == grid_config::VIEW_PLAYLIST) {
                        // In playlist view: don't clear playlist, find all tracks from selected albums
                        // and play from the first matching track in the playlist
                        
                        // Collect all tracks from selected items
                        metadb_handle_list all_selected_tracks;
                        for (int sel_index : m_selected_indices) {
                            auto* item = get_item_at(sel_index);
                            if (item) {
                                all_selected_tracks.add_items(item->tracks);
                            }
                        }
                        
                        if (all_selected_tracks.get_count() > 0) {
                            // Find the first position in playlist of any of the selected tracks
                            t_size playlist_count = pm->playlist_get_item_count(playlist);
                            t_size first_match_position = pfc::infinite_size;
                            
                            for (t_size i = 0; i < playlist_count; i++) {
                                metadb_handle_ptr playlist_track;
                                if (pm->playlist_get_item_handle(playlist_track, playlist, i)) {
                                    // Check if this playlist track is in our selected tracks
                                    for (t_size j = 0; j < all_selected_tracks.get_count(); j++) {
                                        if (playlist_track == all_selected_tracks[j]) {
                                            first_match_position = i;
                                            break;
                                        }
                                    }
                                    if (first_match_position != pfc::infinite_size) {
                                        break;
                                    }
                                }
                            }
                            
                            // If we found a matching track, play from that position
                            if (first_match_position != pfc::infinite_size) {
                                pm->set_playing_playlist(playlist);
                                // Use playlist_execute_default_action to play from specific position
                                pm->playlist_execute_default_action(playlist, first_match_position);
                            }
                        }
                    } else {
                        // In library view: clear playlist and add selected items as before
                        pm->playlist_clear(playlist);
                        for (int sel_index : m_selected_indices) {
                            auto* item = get_item_at(sel_index);
                            if (item) {
                                pm->playlist_add_items(playlist, item->tracks, bit_array_false());
                            }
                        }
                        pm->set_playing_playlist(playlist);
                        static_api_ptr_t<playback_control> pc;
                        pc->play_start();
                    }
                }
                break;
                
            case 2: // Add to current playlist
                if (playlist != pfc::infinite_size && !m_selected_indices.empty()) {
                    // Copy all tracks before adding to avoid invalidation during refresh
                    metadb_handle_list tracks_to_add;
                    for (int sel_index : m_selected_indices) {
                        auto* item = get_item_at(sel_index);
                        if (item && item->tracks.get_count() > 0) {
                            tracks_to_add.add_items(item->tracks);
                        }
                    }
                    // Now add all tracks at once - this prevents multiple refreshes
                    if (tracks_to_add.get_count() > 0) {
                        pm->playlist_add_items(playlist, tracks_to_add, bit_array_false());
                    }
                }
                break;
                
            case 3: // Add to new playlist
                if (!m_selected_indices.empty()) {
                    pfc::string8 playlist_name = "Album Art Grid Selection";
                    metadb_handle_list tracks_to_add;
                    
                    // Copy all tracks and get name before creating playlist
                    for (int sel_index : m_selected_indices) {
                        auto* item = get_item_at(sel_index);
                        if (item) {
                            if (m_selected_indices.size() == 1) {
                                playlist_name = item->display_name;
                            }
                            if (item->tracks.get_count() > 0) {
                                tracks_to_add.add_items(item->tracks);
                            }
                        }
                    }
                    
                    if (tracks_to_add.get_count() > 0) {
                        t_size new_playlist = pm->create_playlist(
                            playlist_name.c_str(),
                            pfc::infinite_size,
                            pfc::infinite_size
                        );
                        pm->playlist_add_items(new_playlist, tracks_to_add, bit_array_false());
                    }
                }
                break;
                
            case 10: m_config.grouping = grid_config::GROUP_BY_FOLDER; needs_refresh = true; config_changed = true; break;
            case 11: m_config.grouping = grid_config::GROUP_BY_DIRECTORY; needs_refresh = true; config_changed = true; break;
            case 12: m_config.grouping = grid_config::GROUP_BY_ALBUM; needs_refresh = true; config_changed = true; break;
            case 13: m_config.grouping = grid_config::GROUP_BY_ARTIST; needs_refresh = true; config_changed = true; break;
            case 14: m_config.grouping = grid_config::GROUP_BY_ALBUM_ARTIST; needs_refresh = true; config_changed = true; break;
            case 15: m_config.grouping = grid_config::GROUP_BY_ARTIST_ALBUM; needs_refresh = true; config_changed = true; break;
            case 16: m_config.grouping = grid_config::GROUP_BY_PERFORMER; needs_refresh = true; config_changed = true; break;
            case 17: m_config.grouping = grid_config::GROUP_BY_COMPOSER; needs_refresh = true; config_changed = true; break;
            case 18: m_config.grouping = grid_config::GROUP_BY_GENRE; needs_refresh = true; config_changed = true; break;
            case 19: m_config.grouping = grid_config::GROUP_BY_YEAR; needs_refresh = true; config_changed = true; break;
            case 20: m_config.grouping = grid_config::GROUP_BY_LABEL; needs_refresh = true; config_changed = true; break;
            case 21: m_config.grouping = grid_config::GROUP_BY_RATING; needs_refresh = true; config_changed = true; break;
            case 22: m_config.grouping = grid_config::GROUP_BY_COMMENT; needs_refresh = true; config_changed = true; break;
            // Column options (23-30 for columns 3-10)
            case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30:
                m_config.columns = cmd - 20;  // 23-20=3, 24-20=4, etc.
                calculate_layout();
                config_changed = true; 
                break;
            case 35: m_config.show_text = !m_config.show_text; config_changed = true; break;
            case 36: m_config.show_track_count = !m_config.show_track_count; config_changed = true; break;
            case 40: // Select all
                m_selected_indices.clear();
                for (size_t i = 0; i < m_items.size(); i++) {
                    m_selected_indices.insert(i);
                }
                InvalidateRect(m_hwnd, NULL, FALSE);
                break;
            case 41: needs_refresh = true; break;
            case 50: m_config.sorting = grid_config::SORT_BY_NAME; needs_sort = true; config_changed = true; break;
            case 51: m_config.sorting = grid_config::SORT_BY_ARTIST; needs_sort = true; config_changed = true; break;
            case 52: m_config.sorting = grid_config::SORT_BY_ALBUM; needs_sort = true; config_changed = true; break;
            case 53: m_config.sorting = grid_config::SORT_BY_YEAR; needs_sort = true; config_changed = true; break;
            case 54: m_config.sorting = grid_config::SORT_BY_GENRE; needs_sort = true; config_changed = true; break;
            case 55: m_config.sorting = grid_config::SORT_BY_DATE; needs_sort = true; config_changed = true; break;
            case 56: m_config.sorting = grid_config::SORT_BY_SIZE; needs_sort = true; config_changed = true; break;
            case 57: m_config.sorting = grid_config::SORT_BY_TRACK_COUNT; needs_sort = true; config_changed = true; break;
            case 58: m_config.sorting = grid_config::SORT_BY_RATING; needs_sort = true; config_changed = true; break;
            case 59: m_config.sorting = grid_config::SORT_BY_PATH; needs_sort = true; config_changed = true; break;
            case 69: m_config.sorting = grid_config::SORT_BY_RANDOM; needs_sort = true; config_changed = true; break;
            case 60: m_config.text_lines = 1; config_changed = true; break;
            case 61: m_config.text_lines = 2; config_changed = true; break;
            case 62: m_config.text_lines = 3; config_changed = true; break;
            
            // View Mode cases
            case 100: // Library view
                if (m_config.view != grid_config::VIEW_LIBRARY) {
                    m_config.view = grid_config::VIEW_LIBRARY;
                    needs_refresh = true;
                    config_changed = true;
                }
                break;
            case 101: // Playlist view
                if (m_config.view != grid_config::VIEW_PLAYLIST) {
                    m_config.view = grid_config::VIEW_PLAYLIST;
                    needs_refresh = true;
                    config_changed = true;
                }
                break;
                
            case 110: // Double-click: Play
                if (m_config.doubleclick != grid_config::DOUBLECLICK_PLAY) {
                    m_config.doubleclick = grid_config::DOUBLECLICK_PLAY;
                    config_changed = true;
                }
                break;
                
            case 111: // Double-click: Add to Current
                if (m_config.doubleclick != grid_config::DOUBLECLICK_ADD_TO_CURRENT) {
                    m_config.doubleclick = grid_config::DOUBLECLICK_ADD_TO_CURRENT;
                    config_changed = true;
                }
                break;
                
            case 112: // Double-click: Add to New
                if (m_config.doubleclick != grid_config::DOUBLECLICK_ADD_TO_NEW) {
                    m_config.doubleclick = grid_config::DOUBLECLICK_ADD_TO_NEW;
                    config_changed = true;
                }
                break;
                
            case 42: // Toggle search
                toggle_search(!m_search_visible);
                break;
        }
        
        if (needs_refresh) {
            refresh_items();
        } else if (needs_sort) {
            sort_items();
            InvalidateRect(m_hwnd, NULL, FALSE);
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
            
            // Update tooltip - only show when labels are hidden (original behavior)
            if (m_tooltip && !m_config.show_text && index >= 0 && index < (int)get_item_count()) {
                // Show tooltip with full text only when labels are hidden
                // Convert display index to real index when filtering
                size_t real_idx = get_real_index(index);
                if (real_idx >= m_items.size()) return 0;
                auto& item = m_items[real_idx];
                pfc::string8 tooltip_text = item->display_name;
                // Don't add track count to tooltip since it's visible in badge
                
                pfc::stringcvt::string_os_from_utf8 tooltip_w(tooltip_text.c_str());
                
                TOOLINFO ti = {};
                ti.cbSize = sizeof(TOOLINFO);
                ti.hwnd = m_hwnd;
                ti.uId = 0;
                ti.lpszText = const_cast<LPTSTR>(tooltip_w.get_ptr());
                SendMessage(m_tooltip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
                
                // Position tooltip below the item, centered
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                calculate_layout();
                int text_height = calculate_text_height();
                int item_total_height = m_item_size + text_height;
                int cols = m_config.columns;
                int total_width = cols * m_item_size + (cols + 1) * PADDING;
                int start_x = (rc.right - total_width) / 2;
                
                int col = index % cols;
                int row = index / cols;
                int item_x = start_x + col * (m_item_size + PADDING) + PADDING + m_item_size / 2;
                int item_y = row * (item_total_height + PADDING) + PADDING + item_total_height - m_scroll_pos + 5;
                
                POINT pt = {item_x, item_y};
                ClientToScreen(m_hwnd, &pt);
                SendMessage(m_tooltip, TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));
                SendMessage(m_tooltip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
                
                m_tooltip_index = index;
            } else if (m_tooltip) {
                // Hide tooltip
                TOOLINFO ti = {};
                ti.cbSize = sizeof(TOOLINFO);
                ti.hwnd = m_hwnd;
                ti.uId = 0;
                SendMessage(m_tooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
                m_tooltip_index = -1;
            }
        }
        
        return 0;
    }
    
    LRESULT on_mouseleave() {
        m_tracking = false;
        if (m_hover_index != -1) {
            m_hover_index = -1;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        // Hide tooltip
        if (m_tooltip) {
            TOOLINFO ti = {};
            ti.cbSize = sizeof(TOOLINFO);
            ti.hwnd = m_hwnd;
            ti.uId = 0;
            SendMessage(m_tooltip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&ti);
            m_tooltip_index = -1;
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
        
        calculate_layout();  // Update m_item_size
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        size_t item_count = get_item_count();
        int rows = (item_count + cols - 1) / cols;
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
        }
        
        m_scroll_pos = max(0, min(m_scroll_pos, max(0, total_height - rc.bottom)));
        
        if (m_scroll_pos != old_pos) {
            SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
        
        return 0;
    }
    
    LRESULT on_mousewheel(int delta, int keys) {
        // Ctrl+Wheel to adjust columns
        if (keys & MK_CONTROL) {
            int new_cols = m_config.columns;
            if (delta > 0) {
                new_cols = min(10, new_cols + 1);
            } else {
                new_cols = max(3, new_cols - 1);
            }
            
            if (new_cols != m_config.columns) {
                m_config.columns = new_cols;
                calculate_layout();  // Recalculate item size
                
                // Mark thumbnails for regeneration if size changed significantly
                for (auto& item : m_items) {
                    if (item->thumbnail->bitmap && item->thumbnail->cached_size > 0) {
                        int size_diff = abs(item->thumbnail->cached_size - m_item_size);
                        if (size_diff > 20) {
                            // Size changed enough to warrant regeneration
                            item->thumbnail->loading = false;  // Allow regeneration
                        }
                    }
                }
                
                m_callback->on_min_max_info_change();
                update_scrollbar();
                InvalidateRect(m_hwnd, NULL, FALSE);
            }
        } else {
            // Smooth scrolling
            int lines = 3;
            for (int i = 0; i < lines; i++) {
                on_vscroll(delta > 0 ? SB_LINEUP : SB_LINEDOWN);
            }
        }
        return 0;
    }
    
    void update_scrollbar() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        calculate_layout();  // Update m_item_size
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int cols = m_config.columns;
        size_t item_count = get_item_count();
        int rows = (item_count + cols - 1) / cols;
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
        static const GUID guid = { 0x5a6b4f80, 0x9b3d, 0x4c85, { 0xa8, 0xf2, 0x7c, 0x3d, 0x9b, 0x5e, 0x2a, 0x10 } };
        return guid;
    }
    
    // Playlist callback methods for auto-refresh - implement all required abstract methods
    void on_items_added(t_size p_base, metadb_handle_list_cref p_data, const bit_array & p_selection) override {
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_items_reordered(const t_size * p_order, t_size p_count) override {
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_items_removing(const bit_array & p_mask, t_size p_old_count, t_size p_new_count) override {
        // Called before removal - no action needed
    }
    
    void on_items_removed(const bit_array & p_mask, t_size p_old_count, t_size p_new_count) override {
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_items_selection_change(const bit_array & p_affected, const bit_array & p_state) override {
        // Selection changes don't affect our grid view
    }
    
    void on_item_focus_change(t_size p_from, t_size p_to) override {
        // Focus changes don't affect our grid view
    }
    
    void on_items_modified(const bit_array & p_mask) override {
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_items_modified_fromplayback(const bit_array & p_mask, play_control::t_display_level p_level) override {
        // Playback modifications don't affect our grid view
    }
    
    void on_items_replaced(const bit_array & p_mask, const pfc::list_base_const_t<playlist_callback::t_on_items_replaced_entry> & p_data) override {
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_item_ensure_visible(t_size p_idx) override {
        // Not relevant for our grid view
    }
    
    void on_playlist_switch() override {
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_playlist_renamed(const char * p_new_name, t_size p_new_name_len) override {
        // Playlist name changes don't affect our grid view
    }
    
    void on_playlist_locked(bool p_locked) override {
        // Playlist lock changes don't affect our grid view
    }
    
    void on_default_format_changed() override {
        // Format changes don't affect our grid view
    }
    
    void on_playback_order_changed(t_size p_new_index) override {
        // Playback order changes don't affect our grid view
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
        p_out = "Album art grid v8 with multi-select and improved text. Ctrl+Click to multi-select, Ctrl+A to select all.";
        return true;
    }
    
    ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) override {
        return NULL;
    }
};

static service_factory_single_t<album_grid_ui_element> g_album_grid_ui_element_factory;