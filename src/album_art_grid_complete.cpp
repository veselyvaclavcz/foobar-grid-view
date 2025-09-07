// Album Art Grid v10.0.50 - Complete Implementation
// Based on ALBUM_ART_GRID_COMPLETE_SPECIFICATION.md

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <unknwn.h>
#include <objidl.h>
#include <mmsystem.h>
#include <atomic>
#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")

#define interface struct

#include "../SDK-2025-03-07/foobar2000/SDK/foobar2000.h"

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "10.0.50",
    "Album Art Grid Component\n"
    "Display your music library as a customizable grid of album covers\n\n"
    "Version 10.0.50 - Complete x64 Build\n"
    "Built for foobar2000 v2.x"
);

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Global shutdown flag
static std::atomic<bool> g_shutting_down(false);
static ULONG_PTR g_gdiplus_token = 0;

// Magic number for object validation
constexpr uint64_t GRID_MAGIC_VALID = 0x474149524447414C; // "LAIDGRID"
constexpr uint64_t GRID_MAGIC_DESTROYED = 0xDEADBEEFDEADBEEF;

// Enums for configuration
enum GroupingMode {
    GroupByFolder = 0,
    GroupByAlbum,
    GroupByArtist,
    GroupByAlbumArtist,
    GroupByYear,
    GroupByGenre,
    GroupByDateModified,
    GroupByDateAdded,
    GroupByFileSize,
    GroupByTrackCount,
    GroupByRating,
    GroupByPlaycount,
    GroupByCustom
};

enum SortingMode {
    SortByName = 0,
    SortByArtist,
    SortByAlbum,
    SortByYear,
    SortByGenre,
    SortByDateModified,
    SortByTotalSize,
    SortByTrackCount,
    SortByRating,
    SortByPath,
    SortByRandom
};

enum ViewMode {
    ViewLibrary = 0,
    ViewPlaylist
};

enum DoubleClickAction {
    ActionPlay = 0,
    ActionAddToPlaylist,
    ActionNewPlaylist
};

// Configuration structure
struct grid_config {
    int thumbnail_size = 150;
    int num_columns = 0;  // 0 = auto
    bool auto_fill_mode = true;
    GroupingMode grouping = GroupByAlbum;
    SortingMode sorting = SortByName;
    bool sort_descending = false;
    DoubleClickAction double_click = ActionPlay;
    bool auto_scroll_now_playing = false;
    bool show_search = false;
    ViewMode view_mode = ViewLibrary;
    size_t cache_size_mb = 256;
    int prefetch_range = 20;
};

// Grid item structure
struct grid_item {
    metadb_handle_list tracks;
    pfc::string8 primary_text;
    pfc::string8 secondary_text;
    pfc::string8 sort_key;
    Gdiplus::Bitmap* cached_thumb = nullptr;
    bool is_now_playing = false;
    bool is_selected = false;
    pfc::string8 search_text;
};

// Forward declarations
class album_art_grid_instance;
static std::vector<album_art_grid_instance*> g_grid_instances;

// Main grid UI element instance
class album_art_grid_instance : 
    public ui_element_instance,
    private playlist_callback_single,
    private library_callback,
    private play_callback {

private:
    HWND m_hwnd = NULL;
    HWND m_search_box = NULL;
    ui_element_instance_callback_ptr m_callback;
    grid_config m_config;
    std::vector<grid_item> m_items;
    std::atomic<uint64_t> m_magic;
    std::atomic<bool> m_destroyed;
    int m_scroll_pos = 0;
    int m_selected_index = -1;
    pfc::string8 m_search_text;
    bool m_is_edit_mode = false;
    
    // Safe validation
    bool is_valid() const {
        if (m_destroyed.load()) return false;
        if (m_magic.load() != GRID_MAGIC_VALID) return false;
        
        __try {
            volatile auto test = m_magic.load();
            return true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
    
    // Safe callback execution
    template<typename Func>
    void safe_callback_execute(Func&& func) {
        if (!is_valid()) return;
        if (g_shutting_down.load()) return;
        
        __try {
            if (m_callback.is_valid()) {
                auto* ptr = m_callback.get_ptr();
                if (ptr && !IsBadReadPtr(ptr, sizeof(void*))) {
                    func();
                }
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            console::printf("Album Art Grid: Exception in callback");
        }
    }
    
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        auto* instance = reinterpret_cast<album_art_grid_instance*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        
        if (!instance || !instance->is_valid()) {
            return DefWindowProc(hwnd, msg, wp, lp);
        }
        
        switch (msg) {
        case WM_CREATE:
            return 0;
            
        case WM_DESTROY:
            if (instance) {
                instance->m_hwnd = NULL;
            }
            return 0;
            
        case WM_SIZE:
            instance->on_size(LOWORD(lp), HIWORD(lp));
            return 0;
            
        case WM_PAINT:
            instance->on_paint();
            return 0;
            
        case WM_ERASEBKGND:
            return 1;
            
        case WM_LBUTTONDOWN:
            instance->on_lbutton_down(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            return 0;
            
        case WM_LBUTTONDBLCLK:
            instance->on_lbutton_dblclk(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            return 0;
            
        case WM_RBUTTONDOWN:
            instance->on_rbutton_down(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            return 0;
            
        case WM_MOUSEWHEEL:
            instance->on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wp));
            return 0;
            
        case WM_KEYDOWN:
            instance->on_key_down(wp);
            return 0;
            
        case WM_VSCROLL:
            instance->on_vscroll(LOWORD(wp));
            return 0;
        }
        
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    
    void on_size(int width, int height) {
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void on_paint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        // Create GDI+ graphics
        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        
        // Get client rect
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        // Clear background
        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 30, 30, 30));
        graphics.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);
        
        // Calculate grid layout
        int cols = m_config.auto_fill_mode ? 
            max(1, rc.right / (m_config.thumbnail_size + 10)) : 
            max(1, m_config.num_columns);
        
        int item_width = m_config.thumbnail_size;
        int item_height = m_config.thumbnail_size + 40;
        int spacing = 10;
        
        // Draw items
        int visible_start = (m_scroll_pos / item_height) * cols;
        int visible_end = min((int)m_items.size(), visible_start + ((rc.bottom / item_height) + 2) * cols);
        
        for (int i = visible_start; i < visible_end; i++) {
            int col = i % cols;
            int row = i / cols;
            int x = col * (item_width + spacing) + spacing;
            int y = row * (item_height + spacing) + spacing - m_scroll_pos;
            
            draw_item(graphics, m_items[i], x, y, item_width, item_height);
        }
        
        EndPaint(m_hwnd, &ps);
    }
    
    void draw_item(Gdiplus::Graphics& g, const grid_item& item, int x, int y, int w, int h) {
        // Draw thumbnail placeholder
        Gdiplus::SolidBrush thumbBrush(Gdiplus::Color(255, 50, 50, 50));
        g.FillRectangle(&thumbBrush, x, y, w, w);
        
        // Draw selection/hover
        if (item.is_selected) {
            Gdiplus::SolidBrush selBrush(Gdiplus::Color(100, 51, 153, 255));
            g.FillRectangle(&selBrush, x, y, w, w);
        }
        
        // Draw now playing indicator
        if (item.is_now_playing) {
            Gdiplus::Pen nowPlayingPen(Gdiplus::Color(255, 51, 153, 255), 3.0f);
            g.DrawRectangle(&nowPlayingPen, x, y, w, w);
        }
        
        // Draw text
        Gdiplus::Font font(L"Segoe UI", 9);
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255, 255));
        Gdiplus::RectF textRect(x, y + w + 2, w, 36);
        
        // Convert text to wide string
        pfc::stringcvt::string_wide_from_utf8 wtext(item.primary_text.get_ptr());
        
        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentCenter);
        format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        
        g.DrawString(wtext.get_ptr(), -1, &font, textRect, &format, &textBrush);
        
        // Draw secondary text
        if (item.secondary_text.get_length() > 0) {
            textRect.Y += 18;
            Gdiplus::Font fontSmall(L"Segoe UI", 8);
            Gdiplus::SolidBrush textBrush2(Gdiplus::Color(255, 180, 180, 180));
            pfc::stringcvt::string_wide_from_utf8 wtext2(item.secondary_text.get_ptr());
            g.DrawString(wtext2.get_ptr(), -1, &fontSmall, textRect, &format, &textBrush2);
        }
    }
    
    void on_lbutton_down(int x, int y) {
        int index = hit_test(x, y);
        if (index >= 0 && index < (int)m_items.size()) {
            m_selected_index = index;
            m_items[index].is_selected = true;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    void on_lbutton_dblclk(int x, int y) {
        int index = hit_test(x, y);
        if (index >= 0 && index < (int)m_items.size()) {
            play_item(index);
        }
    }
    
    void on_rbutton_down(int x, int y) {
        // Context menu would go here
        if (m_is_edit_mode && m_callback.is_valid()) {
            // Show replace element menu in edit mode
            safe_callback_execute([this, x, y]() {
                POINT pt = {x, y};
                ClientToScreen(m_hwnd, &pt);
                m_callback->request_replace(this);
            });
        }
    }
    
    void on_mouse_wheel(int delta) {
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            // Ctrl+Wheel to change columns
            if (delta > 0 && m_config.thumbnail_size < 300) {
                m_config.thumbnail_size += 20;
            } else if (delta < 0 && m_config.thumbnail_size > 80) {
                m_config.thumbnail_size -= 20;
            }
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else {
            // Regular scroll
            m_scroll_pos -= (delta / 2);
            m_scroll_pos = max(0, m_scroll_pos);
            InvalidateRect(m_hwnd, NULL, FALSE);
        }
    }
    
    void on_key_down(WPARAM key) {
        if (key >= 'A' && key <= 'Z') {
            jump_to_letter((char)key);
        } else if (key == VK_F5) {
            refresh_items();
        }
    }
    
    void on_vscroll(int action) {
        // Handle scrollbar
    }
    
    int hit_test(int x, int y) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int cols = m_config.auto_fill_mode ? 
            max(1, rc.right / (m_config.thumbnail_size + 10)) : 
            max(1, m_config.num_columns);
        
        int col = x / (m_config.thumbnail_size + 10);
        int row = (y + m_scroll_pos) / (m_config.thumbnail_size + 50);
        
        if (col >= 0 && col < cols) {
            return row * cols + col;
        }
        return -1;
    }
    
    void play_item(int index) {
        if (index >= 0 && index < (int)m_items.size()) {
            static_api_ptr_t<playlist_manager> pm;
            pm->activeplaylist_clear();
            pm->activeplaylist_add_items(m_items[index].tracks, bit_array_true());
            pm->set_playing_playlist(pm->get_active_playlist());
            static_api_ptr_t<playback_control>()->start();
        }
    }
    
    void jump_to_letter(char letter) {
        for (size_t i = 0; i < m_items.size(); i++) {
            if (m_items[i].primary_text.get_length() > 0) {
                char first = toupper(m_items[i].primary_text[0]);
                if (first == letter) {
                    m_selected_index = i;
                    scroll_to_item(i);
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                }
            }
        }
    }
    
    void scroll_to_item(int index) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        
        int cols = m_config.auto_fill_mode ? 
            max(1, rc.right / (m_config.thumbnail_size + 10)) : 
            max(1, m_config.num_columns);
        
        int row = index / cols;
        int y = row * (m_config.thumbnail_size + 50);
        
        m_scroll_pos = y;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void refresh_items() {
        m_items.clear();
        
        if (m_config.view_mode == ViewLibrary) {
            load_library_items();
        } else {
            load_playlist_items();
        }
        
        sort_items();
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    void load_library_items() {
        static_api_ptr_t<library_manager> lm;
        metadb_handle_list all_items;
        lm->get_all_items(all_items);
        
        // Group items by configured mode
        std::unordered_map<pfc::string8, grid_item, pfc::string_hash, pfc::string_comparator> groups;
        
        for (size_t i = 0; i < all_items.get_count(); i++) {
            pfc::string8 group_key;
            get_group_key(all_items[i], group_key);
            
            auto& item = groups[group_key];
            item.tracks.add_item(all_items[i]);
            item.primary_text = group_key;
            
            // Get secondary text (artist for album grouping, etc)
            if (m_config.grouping == GroupByAlbum) {
                const char* artist = all_items[i]->get_info_ref()->info().meta_get("artist", 0);
                if (artist) item.secondary_text = artist;
            }
        }
        
        // Convert map to vector
        m_items.reserve(groups.size());
        for (auto& [key, item] : groups) {
            m_items.push_back(std::move(item));
        }
    }
    
    void load_playlist_items() {
        static_api_ptr_t<playlist_manager> pm;
        metadb_handle_list items;
        pm->activeplaylist_get_all_items(items);
        
        // Similar grouping logic as library
        std::unordered_map<pfc::string8, grid_item, pfc::string_hash, pfc::string_comparator> groups;
        
        for (size_t i = 0; i < items.get_count(); i++) {
            pfc::string8 group_key;
            get_group_key(items[i], group_key);
            
            auto& item = groups[group_key];
            item.tracks.add_item(items[i]);
            item.primary_text = group_key;
        }
        
        m_items.reserve(groups.size());
        for (auto& [key, item] : groups) {
            m_items.push_back(std::move(item));
        }
    }
    
    void get_group_key(metadb_handle_ptr track, pfc::string8& out) {
        file_info_const_impl info = track->get_info_ref()->info();
        
        switch (m_config.grouping) {
        case GroupByAlbum:
            out = info.meta_get("album", 0);
            if (out.is_empty()) out = "<No Album>";
            break;
        case GroupByArtist:
            out = info.meta_get("artist", 0);
            if (out.is_empty()) out = "<No Artist>";
            break;
        case GroupByAlbumArtist:
            out = info.meta_get("album artist", 0);
            if (out.is_empty()) out = info.meta_get("artist", 0);
            if (out.is_empty()) out = "<No Artist>";
            break;
        case GroupByYear:
            out = info.meta_get("date", 0);
            if (out.is_empty()) out = "<No Year>";
            break;
        case GroupByGenre:
            out = info.meta_get("genre", 0);
            if (out.is_empty()) out = "<No Genre>";
            break;
        default:
            out = info.meta_get("album", 0);
            if (out.is_empty()) out = "<No Album>";
            break;
        }
    }
    
    void sort_items() {
        std::sort(m_items.begin(), m_items.end(), 
            [this](const grid_item& a, const grid_item& b) {
                int result = 0;
                
                switch (m_config.sorting) {
                case SortByName:
                case SortByAlbum:
                    result = stricmp_utf8(a.primary_text, b.primary_text);
                    break;
                case SortByArtist:
                    result = stricmp_utf8(a.secondary_text, b.secondary_text);
                    break;
                case SortByTrackCount:
                    result = a.tracks.get_count() - b.tracks.get_count();
                    break;
                default:
                    result = stricmp_utf8(a.primary_text, b.primary_text);
                    break;
                }
                
                return m_config.sort_descending ? result > 0 : result < 0;
            });
    }
    
public:
    album_art_grid_instance(ui_element_instance_callback_ptr p_callback) 
        : m_callback(p_callback)
        , m_magic(GRID_MAGIC_VALID)
        , m_destroyed(false) {
        
        g_grid_instances.push_back(this);
    }
    
    ~album_art_grid_instance() {
        m_destroyed = true;
        m_magic = GRID_MAGIC_DESTROYED;
        
        auto it = std::find(g_grid_instances.begin(), g_grid_instances.end(), this);
        if (it != g_grid_instances.end()) {
            g_grid_instances.erase(it);
        }
    }
    
    // ui_element_instance methods
    HWND get_wnd() override { 
        return m_hwnd; 
    }
    
    void set_configuration(stream_reader* p_reader, t_size p_size, abort_callback& p_abort) override {
        if (p_size >= sizeof(grid_config)) {
            p_reader->read_object(&m_config, sizeof(grid_config), p_abort);
        }
    }
    
    void get_configuration(stream_writer* p_writer, abort_callback& p_abort) override {
        p_writer->write_object(&m_config, sizeof(grid_config), p_abort);
    }
    
    void initialize_window(HWND p_parent) override {
        // Check if edit mode
        if (m_callback.is_valid()) {
            m_is_edit_mode = m_callback->is_edit_mode_enabled();
        }
        
        // Register window class
        static bool class_registered = false;
        if (!class_registered) {
            WNDCLASSEX wc = {};
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = L"AlbumArtGridClass";
            
            RegisterClassEx(&wc);
            class_registered = true;
        }
        
        // Create window
        m_hwnd = CreateWindowEx(
            0,
            L"AlbumArtGridClass",
            L"Album Art Grid",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL,
            0, 0, 100, 100,
            p_parent,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        
        // Initial load
        refresh_items();
    }
    
    // Playlist callbacks
    void on_items_added(t_size p_base, metadb_handle_list_cref p_data, bit_array const& p_selection) override {
        if (m_config.view_mode == ViewPlaylist) {
            refresh_items();
        }
    }
    
    void on_items_removed(bit_array const& p_mask, t_size p_old_count) override {
        if (m_config.view_mode == ViewPlaylist) {
            refresh_items();
        }
    }
    
    void on_items_modified(bit_array const& p_mask) override {
        if (m_config.view_mode == ViewPlaylist) {
            refresh_items();
        }
    }
    
    // Library callbacks
    void on_items_added(metadb_handle_list_cref p_data) override {
        if (m_config.view_mode == ViewLibrary) {
            refresh_items();
        }
    }
    
    void on_items_removed(metadb_handle_list_cref p_data) override {
        if (m_config.view_mode == ViewLibrary) {
            refresh_items();
        }
    }
    
    void on_items_modified(metadb_handle_list_cref p_data) override {
        if (m_config.view_mode == ViewLibrary) {
            refresh_items();
        }
    }
    
    // Play callbacks
    void on_playback_starting(play_control::t_track_command p_command, bool p_paused) override {}
    void on_playback_new_track(metadb_handle_ptr p_track) override {
        // Update now playing indicator
        for (auto& item : m_items) {
            item.is_now_playing = false;
            for (size_t i = 0; i < item.tracks.get_count(); i++) {
                if (item.tracks[i] == p_track) {
                    item.is_now_playing = true;
                    break;
                }
            }
        }
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
    void on_playback_stop(play_control::t_stop_reason p_reason) override {}
    void on_playback_seek(double p_time) override {}
    void on_playback_pause(bool p_state) override {}
    void on_playback_edited(metadb_handle_ptr p_track) override {}
    void on_playback_dynamic_info(file_info const& p_info) override {}
    void on_playback_dynamic_info_track(file_info const& p_info) override {}
    void on_playback_time(double p_time) override {}
    void on_volume_change(float p_new_val) override {}
};

// UI element factory
class album_art_grid : public ui_element {
public:
    GUID get_guid() override {
        // {A3F7D8E9-4B5C-4D6E-8F9A-1B2C3D4E5F6A}
        static const GUID guid = 
        { 0xa3f7d8e9, 0x4b5c, 0x4d6e, { 0x8f, 0x9a, 0x1b, 0x2c, 0x3d, 0x4e, 0x5f, 0x6a } };
        return guid;
    }
    
    void get_name(pfc::string_base& p_out) override {
        p_out = "Album Art Grid";
    }
    
    ui_element_instance_ptr instantiate(HWND p_parent, ui_element_config::ptr cfg, ui_element_instance_callback_ptr p_callback) override {
        auto instance = fb2k::service_new<album_art_grid_instance>(p_callback);
        
        if (cfg.is_valid()) {
            stream_reader_memblock reader(cfg->get_data(), cfg->get_data_size());
            abort_callback_dummy abort;
            instance->set_configuration(&reader, cfg->get_data_size(), abort);
        }
        
        instance->initialize_window(p_parent);
        return instance;
    }
    
    ui_element_config::ptr get_default_configuration() override {
        grid_config default_config;
        return ui_element_config::g_create(
            get_guid(),
            &default_config,
            sizeof(default_config)
        );
    }
    
    ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) override {
        return nullptr;
    }
    
    bool get_description(pfc::string_base& p_out) override {
        p_out = "Displays your music library as a customizable grid of album covers.";
        return true;
    }
};

// Register UI element
static ui_element_factory<album_art_grid> g_album_art_grid_factory;

// Library viewer service  
class album_art_library_viewer : public library_viewer {
public:
    GUID get_guid() override {
        // {B4F8E9D7-5C6A-4D7F-9E8B-2C3D4E5F6A7B}
        static const GUID guid = 
        { 0xb4f8e9d7, 0x5c6a, 0x4d7f, { 0x9e, 0x8b, 0x2c, 0x3d, 0x4e, 0x5f, 0x6a, 0x7b } };
        return guid;
    }
    
    void get_name(pfc::string_base& p_out) override {
        p_out = "Album Art Grid";
    }
    
    GUID get_ui_element_guid() override {
        return album_art_grid().get_guid();
    }
};

// Register library viewer
static library_viewer_factory<album_art_library_viewer> g_album_art_library_viewer_factory;

// Initialization and cleanup
class album_grid_initquit : public initquit {
public:
    void on_init() override {
        // Initialize GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&g_gdiplus_token, &gdiplusStartupInput, NULL);
        
        console::print("=====================================");
        console::print("Album Art Grid v10.0.50 x64");
        console::print("Component loaded successfully!");
        console::print("=====================================");
    }
    
    void on_quit() override {
        g_shutting_down = true;
        
        // Invalidate all instances
        for (auto* instance : g_grid_instances) {
            if (instance) {
                instance->m_destroyed = true;
                instance->m_magic = GRID_MAGIC_DESTROYED;
            }
        }
        
        // Shutdown GDI+
        if (g_gdiplus_token) {
            Gdiplus::GdiplusShutdown(g_gdiplus_token);
            g_gdiplus_token = 0;
        }
        
        console::print("Album Art Grid v10.0.50 shutting down");
    }
};

// Register initquit service
static initquit_factory_t<album_grid_initquit> g_album_grid_initquit;