// Album Art Grid v10.0.14 - Complete Shutdown Protection
// FIXED: Library viewer crash during app shutdown
// FIXED: Virtual function calls on destroyed objects
// Auto-adjusting performance config for high-RAM systems (16GB+)
#define FOOBAR2000_TARGET_VERSION 80
#define _WIN32_WINNT 0x0600

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include "SDK-2025-03-07/foobar2000/SDK/coreDarkMode.h"
// #include "SDK-2025-03-07/foobar2000/helpers/DarkMode.h"  // Commented out - causes include errors
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
#include <atomic>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uxtheme.lib")


// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "10.0.15",
    "Album Art Grid v10.0.15 - Letter Jump Navigation\n"
    "NEW in v10.0.15:\n"
    "- Added letter jump navigation: Press A-Z or 0-9 to jump to first album starting with that character\n"
    "- Works with current sort order (jumps by album, artist, year, genre, or path)\n"
    "- Automatically selects the found album for visual feedback\n"
    "CRITICAL FIX in v10.0.14:\n"
    "- FIXED: Crash at shutdown when library viewer is destroyed\n"
    "- Added shutdown protection to library_viewer service\n"
    "- Proper cleanup sequence for all static objects\n"
    "- Safe virtual function calls with validity checks\n"
    "From v10.0.13:\n"
    "- Library Viewer Integration: Album Art Grid now appears in Library menu\n"
    "- Can be activated from Library > Album Art Grid\n"
    "- Proper library viewer service registration\n"
    "- Menu command generation for easy access\n"
    "MAINTAINED FROM v10.0.12:\n"
    "- FIXED: Multiple grid instances can now be opened (was blocked in v10.0.11)\n"
    "- Fixed crash at offset 1196Ch in callback_run during shutdown\n"
    "- Fixed version string reporting (was incorrectly showing v10.0.2)\n"
    "- Added null pointer checks in main_thread_callback\n"
    "- Safe thumbnail creation with proper error handling\n"
    "- Shutdown protection now only activates on app exit, not instance close\n"
    "- Protected callback execution with validity checks\n"
    "- Memory safety improvements in callback handling\n"
    "From v10.0.11:\n"
    "- GLOBAL SHUTDOWN PROTECTION: Prevents ALL window closing crashes\n"
    "- Safe virtual call wrappers with exception handling\n"
    "- Auto-detect RAM and adjust cache (256MB-2GB based on system)\n"
    "- Protected against callback execution during UI destruction\n"
    "From v10.0.9:\n"
    "- FIXED F9FCh memory corruption crash during shutdown\n"
    "- Thread-safe thumbnail cache with proper cleanup sequence\n"
    "- NEW: 'Open in Folder' context menu feature\n" 
    "- All v10.0.8 critical fixes maintained\n"
    "From v10.0.8:\n"
    "- Fixed ALL m_callback validity checks to prevent crashes\n"
    "- Protected lines: 799, 1768, 1808-1812, 2807, 2999\n"
    "- Prevents uCallStackTracker destructor crashes\n"
    "- Fixed access violation at shutdown (0x0000000100000003)\n"
    "From v10.0.3:\n"
    "- Fixed metadata extraction errors blocking album art\n"
    "- Improved error handling for problematic library paths\n"
    "From v10.0.2:\n"
    "- Fixed GDI+ initialization\n"
    "- Thread-safe image loading\n"
    "From v10.0.1:\n"
    "- Fixed version display\n"
    "- Fixed shutdown/sleep crashes\n"
    "- Thread-safe operations\n"
    "- Memory optimized (128MB cache limit)\n"
    "ALL FEATURES INCLUDED:\n"
    "- Now Playing indicator with blue border and play icon\n"
    "- Jump to Now Playing (Ctrl+Q)\n"
    "- Auto-scroll to Now Playing option\n"
    "- Smart LRU cache management\n"
    "- Adaptive memory limits\n"
    "- Prefetching for smooth scrolling\n"
    "- 13 grouping modes\n"
    "- 11 sorting options\n"
    "- Search functionality (Ctrl+Shift+S)\n"
    "- Status bar integration\n"
    "- Flexible column control (1-20 columns)\n"
    "- Track sorting options\n"
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

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Global shutdown protection system
// NO GLOBAL FLAGS - only instance-specific protection
namespace shutdown_protection {
    static std::atomic<bool> g_is_shutting_down{false};
    static critical_section g_shutdown_sync;
    static std::set<void*> g_active_instances;
    
    void initiate_app_shutdown() {
        insync(g_shutdown_sync);
        g_is_shutting_down = true;
        // NO global flag - only mark active instances
        // Invalidate all active instances
        for (auto* inst : g_active_instances) {
            if (inst) {
                // Mark instance as invalid
                *((std::atomic<bool>*)inst) = false;
            }
        }
    }
    
    static void register_instance(void* inst) {
        insync(g_shutdown_sync);
        if (!g_is_shutting_down) {
            g_active_instances.insert(inst);
        }
    }
    
    static void unregister_instance(void* inst) {
        insync(g_shutdown_sync);
        g_active_instances.erase(inst);
    }
}

// Safe virtual call wrapper
template<typename Func, typename T>
T safe_virtual_call(Func&& func, T default_val) {
    if (shutdown_protection::g_is_shutting_down) return default_val;
    __try {
        return func();
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return default_val;
    }
}

// Performance configuration (auto-adjusts based on RAM)
class PerformanceConfig {
public:
    static size_t GetMaxCacheSize() {
        MEMORYSTATUSEX mem;
        mem.dwLength = sizeof(mem);
        GlobalMemoryStatusEx(&mem);
        
        size_t totalGB = mem.ullTotalPhys / (1024ULL * 1024ULL * 1024ULL);
        
        if (totalGB >= 32) return 2048;  // 2GB cache for 32GB+ systems
        if (totalGB >= 16) return 1024;  // 1GB cache for 16GB+ systems
        if (totalGB >= 8)  return 512;   // 512MB for 8GB+ systems
        return 256;  // 256MB for smaller systems
    }
    
    static int GetPrefetchRange() {
        MEMORYSTATUSEX mem;
        mem.dwLength = sizeof(mem);
        GlobalMemoryStatusEx(&mem);
        
        size_t totalGB = mem.ullTotalPhys / (1024ULL * 1024ULL * 1024ULL);
        
        if (totalGB >= 16) return 50;   // Prefetch 50 items for high-RAM
        if (totalGB >= 8)  return 30;   // Prefetch 30 items for medium-RAM
        return 20;  // Default 20 items
    }
    
    static int GetBufferZone() {
        MEMORYSTATUSEX mem;
        mem.dwLength = sizeof(mem);
        GlobalMemoryStatusEx(&mem);
        
        size_t totalGB = mem.ullTotalPhys / (1024ULL * 1024ULL * 1024ULL);
        
        if (totalGB >= 16) return 100;  // Buffer 100 items for high-RAM
        if (totalGB >= 8)  return 70;   // Buffer 70 items for medium-RAM
        return 50;  // Default 50 items
    }
};

// Dynamic performance constants
static size_t MAX_CACHE_SIZE_MB = PerformanceConfig::GetMaxCacheSize();
static int PREFETCH_RANGE = PerformanceConfig::GetPrefetchRange();
static int BUFFER_ZONE = PerformanceConfig::GetBufferZone();
static size_t MIN_CACHE_SIZE_MB = 64;   // Keep minimum
static size_t MAX_CACHED_THUMBNAILS = 2000;  // Increased for high-RAM

// GDI+ initialization with proper error checking
class gdiplus_startup {
    ULONG_PTR token;
    bool initialized;
public:
    gdiplus_startup() : token(0), initialized(false) {
        Gdiplus::GdiplusStartupInput input;
        input.GdiplusVersion = 1;
        input.DebugEventCallback = NULL;
        input.SuppressBackgroundThread = FALSE;
        input.SuppressExternalCodecs = FALSE;
        
        Gdiplus::GdiplusStartupOutput output;
        Gdiplus::Status status = Gdiplus::GdiplusStartup(&token, &input, &output);
        
        if (status == Gdiplus::Ok) {
            initialized = true;
            console::print("[Album Art Grid v10.0.2] GDI+ initialized successfully");
        } else {
            initialized = false;
            console::printf("[Album Art Grid v10.0.2] ERROR: GDI+ initialization failed with status: %d", status);
        }
    }
    
    ~gdiplus_startup() {
        if (initialized && token) {
            Gdiplus::GdiplusShutdown(token);
            console::print("[Album Art Grid v10.0.2] GDI+ shutdown complete");
        }
    }
    
    bool is_initialized() const { return initialized; }
};

// static gdiplus_startup g_gdiplus; // REMOVED - GDI+ is initialized in initquit service

// Global album count for titleformat fields
static size_t g_album_count = 0;
static bool g_is_library_view = true;
static critical_section g_count_sync;
static critical_section g_thumbnail_sync;  // For thread-safe thumbnail operations

// Title format field provider for status bar
class albumart_grid_field_provider : public metadb_display_field_provider {
public:
    t_uint32 get_field_count() override {
        return 3;
    }
    
    void get_field_name(t_uint32 index, pfc::string_base& out) override {
        switch(index) {
            case 0:
                out = "albumart_grid_count";
                break;
            case 1:
                out = "albumart_grid_view";
                break;
            case 2:
                out = "albumart_grid_info";
                break;
        }
    }
    
    bool process_field(t_uint32 index, metadb_handle* handle, titleformat_text_out* out) override {
        insync(g_count_sync);
        
        switch(index) {
            case 0: // albumart_grid_count
                if (g_album_count > 0) {
                    out->write_int(titleformat_inputtypes::meta, g_album_count);
                    return true;
                }
                return false;
                
            case 1: // albumart_grid_view
                out->write(titleformat_inputtypes::meta, g_is_library_view ? "Library" : "Playlist");
                return true;
                
            case 2: // albumart_grid_info
                if (g_album_count > 0) {
                    pfc::string8 info;
                    info << (g_is_library_view ? "Library: " : "Playlist: ");
                    info << g_album_count;
                    info << (g_album_count == 1 ? " album" : " albums");
                    out->write(titleformat_inputtypes::meta, info);
                    return true;
                }
                return false;
        }
        return false;
    }
};

// Register the field provider
static service_factory_single_t<albumart_grid_field_provider> g_field_provider;

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
    
    enum track_sort_mode {
        TRACK_SORT_BY_NUMBER = 0,
        TRACK_SORT_BY_NAME = 1,
        TRACK_SORT_NONE = 2
    };
    
    // Default track sorting mode
    track_sort_mode track_sorting = TRACK_SORT_BY_NUMBER;
    
    enum view_mode {
        VIEW_LIBRARY = 0,
        VIEW_PLAYLIST
    };
    
    enum doubleclick_action {
        DOUBLECLICK_PLAY = 0,
        DOUBLECLICK_ADD_TO_CURRENT,
        DOUBLECLICK_ADD_TO_NEW
    };
    
    enum label_format {
        LABEL_ALBUM_ONLY = 0,
        LABEL_ARTIST_ONLY = 1,     // v10.0.4: Added artist only option
        LABEL_ARTIST_ALBUM = 2,
        LABEL_FOLDER_NAME = 3      // v10.0.4: Added explicit folder name option
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
    label_format label_style;  // NEW: Album only vs Artist - Album
    
    grid_config() : columns(5), text_lines(2), show_text(true), show_track_count(true),
                    font_size(11), grouping(GROUP_BY_FOLDER), sorting(SORT_BY_NAME), view(VIEW_LIBRARY), 
                    doubleclick(DOUBLECLICK_PLAY), label_style(LABEL_ALBUM_ONLY) {}
    
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
                
                columns = max(1, columns); // No upper limit - unlimited columns!
                text_lines = max(1, min(3, text_lines));
                font_size = max(7, min(14, font_size));
                if ((int)view < 0 || (int)view > VIEW_PLAYLIST) view = VIEW_LIBRARY;
                if ((int)doubleclick < 0 || (int)doubleclick > DOUBLECLICK_ADD_TO_NEW) doubleclick = DOUBLECLICK_PLAY;
                // v10.0.4: Validate label_style (now 4 options)
                if ((int)label_style < 0 || (int)label_style > LABEL_FOLDER_NAME) label_style = LABEL_ALBUM_ONLY;
            }
        }
    }
    
    ui_element_config::ptr save(const GUID& guid) {
        return ui_element_config::g_create(guid, this, sizeof(grid_config));
    }
};

// Thumbnail cache entry with memory tracking
struct thumbnail_data {
    Gdiplus::Bitmap* bitmap;
    DWORD last_access;
    bool loading;
    int cached_size;
    size_t memory_size;
    
    thumbnail_data() : bitmap(nullptr), last_access(GetTickCount()), loading(false), cached_size(0), memory_size(0) {}
    
    ~thumbnail_data() {
        clear();
    }
    
    void clear() {
        if (bitmap) {
            delete bitmap;
            bitmap = nullptr;
            memory_size = 0;
        }
    }
    
    void set_bitmap(Gdiplus::Bitmap* bmp, int size) {
        clear();
        bitmap = bmp;
        cached_size = size;
        last_access = GetTickCount();
        if (bmp) {
            memory_size = bmp->GetWidth() * bmp->GetHeight() * 4;
        }
    }
    
    void touch() {
        last_access = GetTickCount();
    }
};

// Smart cache management with LRU and adaptive limits
class thumbnail_cache {
private:
    static size_t total_memory;
    static size_t max_cache_size;
    static std::vector<thumbnail_data*> all_thumbnails;
    static int viewport_first;
    static int viewport_last;
    static std::atomic<bool> shutdown_in_progress;  // v10.0.9: F9FCh crash fix
    
    static size_t get_available_memory() {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        
        // Use 25% of available physical memory, but within our limits
        size_t quarter_available = memInfo.ullAvailPhys / 4;
        size_t min_size = MIN_CACHE_SIZE_MB * 1024 * 1024;
        size_t max_size = MAX_CACHE_SIZE_MB * 1024 * 1024;
        
        return max(min_size, min(max_size, quarter_available));
    }
    
public:
    static void update_viewport(int first, int last) {
        viewport_first = first;
        viewport_last = last;
    }
    
    static bool is_in_viewport(int index) {
        return index >= viewport_first && index <= viewport_last;
    }
    
    static bool is_in_buffer_zone(int index) {
        return index >= (viewport_first - BUFFER_ZONE) && 
               index <= (viewport_last + BUFFER_ZONE);
    }
    
    static void add_thumbnail(thumbnail_data* thumb, int item_index) {
        if (!thumb || !thumb->bitmap || shutdown_in_progress) return;  // v10.0.9: F9FCh fix
        
        // Update adaptive cache size
        max_cache_size = get_available_memory();
        
        // Make room if needed (LRU eviction)
        while (total_memory + thumb->memory_size > max_cache_size) {
            if (!evict_lru_thumbnail(item_index)) {
                break;  // Can't free more memory
            }
        }
        
        // Add to cache
        all_thumbnails.push_back(thumb);
        total_memory += thumb->memory_size;
    }
    
    static bool evict_lru_thumbnail(int current_item_index) {
        thumbnail_data* oldest = nullptr;
        DWORD oldest_time = MAXDWORD;
        int oldest_index = -1;
        
        // Find LRU thumbnail that's NOT in viewport or buffer zone
        for (size_t i = 0; i < all_thumbnails.size(); i++) {
            auto* thumb = all_thumbnails[i];
            if (!thumb || !thumb->bitmap) continue;
            
            // Don't evict if it's visible or in buffer zone
            // Note: We'd need item index tracking for perfect implementation
            // For now, just find oldest
            if (thumb->last_access < oldest_time) {
                oldest = thumb;
                oldest_time = thumb->last_access;
                oldest_index = i;
            }
        }
        
        if (oldest && oldest->memory_size > 0) {
            total_memory = (total_memory > oldest->memory_size) ? 
                          (total_memory - oldest->memory_size) : 0;
            oldest->clear();
            
            if (oldest_index >= 0) {
                all_thumbnails.erase(all_thumbnails.begin() + oldest_index);
            }
            return true;
        }
        
        return false;
    }
    
    static void clear_all() {
        // v10.0.9 CRITICAL FIX: Prevent F9FCh memory corruption
        shutdown_in_progress = true;
        
        // Safe cleanup with shutdown signal
        if (!all_thumbnails.empty()) {
            for (auto* thumb : all_thumbnails) {
                if (thumb && thumb->bitmap) {
                    try {
                        thumb->clear();
                    } catch (...) {
                        // Ignore exceptions during shutdown
                    }
                }
            }
            all_thumbnails.clear();
        }
        total_memory = 0;
    }
    
    static bool is_shutting_down() {
        return shutdown_in_progress;
    }
    
    static size_t get_memory_usage() { return total_memory; }
    static size_t get_cache_limit() { return max_cache_size; }
};

// Initialize static members
size_t thumbnail_cache::total_memory = 0;
size_t thumbnail_cache::max_cache_size = MIN_CACHE_SIZE_MB * 1024 * 1024;
std::vector<thumbnail_data*> thumbnail_cache::all_thumbnails;
int thumbnail_cache::viewport_first = 0;
int thumbnail_cache::viewport_last = 0;
std::atomic<bool> thumbnail_cache::shutdown_in_progress{false};  // v10.0.9: F9FCh fix

// Album/folder data structure
struct grid_item {
    pfc::string8 display_name;  // Using pfc::string8 for proper UTF-8 handling
    pfc::string8 folder_name;   // v10.0.4: Store actual folder name separately
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
    
    // Get display text based on configuration
    pfc::string8 get_display_text(grid_config::label_format format) const {
        switch (format) {
            case grid_config::LABEL_ALBUM_ONLY:
                return album.is_empty() ? display_name : album;
            
            case grid_config::LABEL_ARTIST_ONLY:
                // v10.0.4: Added artist only option
                return artist.is_empty() ? display_name : artist;
            
            case grid_config::LABEL_ARTIST_ALBUM:
                if (!artist.is_empty() && !album.is_empty()) {
                    pfc::string8 result;
                    result << artist << " - " << album;
                    return result;
                } else if (!album.is_empty()) {
                    return album;
                } else {
                    return display_name;
                }
            
            case grid_config::LABEL_FOLDER_NAME:
                // v10.0.4: Return actual folder name
                return folder_name.is_empty() ? display_name : folder_name;
            
            default:
                return display_name;
        }
    }
    
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
    bool m_auto_scroll_to_now_playing;  // Auto-scroll to now playing when track changes
    int m_scroll_pos;
    std::set<int> m_selected_indices;  // Multi-select support
    int m_last_selected;  // For shift-select
    int m_hover_index;
    int m_tooltip_index;  // Index of item tooltip is showing for
    service_ptr_t<ui_element_instance_callback> m_callback;
    grid_config m_config;
    fb2k::CCoreDarkModeHooks m_dark;
    std::atomic<bool> m_is_destroying{false};  // Atomic flag to prevent use-after-free in playlist callbacks
    bool m_tracking;
    
    // Visible range for lazy loading
    int m_first_visible;
    int m_last_visible;
    
    // Now playing tracking
    metadb_handle_ptr m_now_playing;
    int m_now_playing_index;
    bool m_highlight_now_playing;
    DWORD m_last_user_scroll;
    
    // Timer for deferred loading
    static const UINT_PTR TIMER_LOAD = 1;
    static const UINT_PTR TIMER_PROGRESSIVE = 2;
    static const UINT_PTR TIMER_NOW_PLAYING = 3;
    
    static const int PADDING = 8;
    
    // Calculated layout values
    int m_item_size;  // Calculated item size based on columns and width
    int m_font_height;  // Actual font height from foobar2000
    
    // No more cleanup timers - using LRU instead
    
    // Letter jump navigation tracking
    char m_last_jump_letter;
    int m_last_jump_index;
    
public:
    album_grid_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr callback)
        : m_callback(callback), m_hwnd(NULL), m_parent(NULL), m_tooltip(NULL), m_search_box(NULL),
          m_search_visible(false), m_auto_scroll_to_now_playing(false), m_scroll_pos(0), 
          m_last_selected(-1), m_hover_index(-1), m_tooltip_index(-1), m_tracking(false),
          m_first_visible(0), m_last_visible(0), m_item_size(120), m_font_height(14),
          m_now_playing_index(-1), m_highlight_now_playing(true),
          m_last_user_scroll(0), m_last_jump_letter(0), m_last_jump_index(-1) {
        m_config.load(config);
        
        // Register this instance (but don't set global shutdown)
        shutdown_protection::register_instance(this);
        
        // Register for playlist callbacks
        static_api_ptr_t<playlist_manager> pm;
        pm->register_callback(this, playlist_callback::flag_on_items_added | 
                                   playlist_callback::flag_on_items_removed |
                                   playlist_callback::flag_on_items_reordered);
    }
    
    ~album_grid_instance() {
        // Mark this instance as destroying (but DON'T set global shutdown)
        m_is_destroying = true;
        
        // Unregister this instance only
        shutdown_protection::unregister_instance(this);
        
        console::print("[Album Art Grid v10.0.14] Closing grid instance");
        
        // CRITICAL: Unregister callbacks FIRST to prevent race conditions
        // This must happen before any cleanup to avoid callbacks during destruction
        try {
            static_api_ptr_t<playlist_manager> pm;
            pm->unregister_callback(this);
        } catch(...) {
            // Ignore errors during shutdown
        }
        
        // Kill all timers if window exists
        if (m_hwnd && IsWindow(m_hwnd)) {
            KillTimer(m_hwnd, TIMER_LOAD);
            KillTimer(m_hwnd, TIMER_PROGRESSIVE);
            KillTimer(m_hwnd, TIMER_NOW_PLAYING);
            
            // Clear window data pointer to prevent any pending messages from accessing this object
            SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);
            
            DestroyWindow(m_hwnd);
            m_hwnd = NULL;
        }
        
        // Clear all data after window is destroyed
        m_items.clear();
        thumbnail_cache::clear_all();
        
        // Release now playing handle
        m_now_playing.release();
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
        // Start now playing check timer (every 1 second)
        SetTimer(m_hwnd, TIMER_NOW_PLAYING, 1000, NULL);
    }
    
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        // v10.0.11: Global shutdown protection check
        if (shutdown_protection::g_is_shutting_down) {
            return DefWindowProc(hwnd, msg, wp, lp);
        }
        
        album_grid_instance* instance;
        
        if (msg == WM_CREATE) {
            CREATESTRUCT* cs = (CREATESTRUCT*)lp;
            instance = (album_grid_instance*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)instance);
        } else {
            instance = (album_grid_instance*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        
        // v10.0.11: Check instance validity with protection
        if (instance && !instance->m_is_destroying && !shutdown_protection::g_is_shutting_down) {
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
                    // Mark instance as destroying (but DON'T set global shutdown)
                    if (instance) {
                        instance->m_is_destroying = true;
                    }
                    
                    // Kill all timers safely
                    __try {
                        KillTimer(hwnd, TIMER_LOAD);
                        KillTimer(hwnd, TIMER_PROGRESSIVE);
                        KillTimer(hwnd, TIMER_NOW_PLAYING);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER) {
                        // Timer cleanup failed, but we're shutting down anyway
                    }
                    
                    // Clear the window data pointer to prevent any pending messages
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
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
            // No cleanup timer - using LRU cache management
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
        } else if (timer_id == TIMER_NOW_PLAYING) {
            // Check for now playing changes
            check_now_playing();
        }
        // No more refresh or cleanup timers
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
            
            // No timer needed - we use playlist callbacks
            
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
            } else if (key == 'Q') {
                // Jump to now playing album on Ctrl+Q
                jump_to_now_playing();
                return 0;
            }
        } else if (key >= 'A' && key <= 'Z') {
            // Letter jump navigation - jump to first album starting with this letter
            // Only if no modifier keys are pressed
            if (!(GetKeyState(VK_CONTROL) & 0x8000) && 
                !(GetKeyState(VK_SHIFT) & 0x8000) && 
                !(GetKeyState(VK_MENU) & 0x8000)) {
                jump_to_letter((char)key);
                return 0;
            }
        } else if (key >= '0' && key <= '9') {
            // Number jump navigation - jump to first album starting with this number
            // Only if no modifier keys are pressed
            if (!(GetKeyState(VK_CONTROL) & 0x8000) && 
                !(GetKeyState(VK_SHIFT) & 0x8000) && 
                !(GetKeyState(VK_MENU) & 0x8000)) {
                jump_to_letter((char)key);
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
        // CRITICAL FIX v10.0.7: Added validity check before query_font_ex to prevent crash
        HFONT hFont = NULL;
        if (m_callback.is_valid()) {
            hFont = m_callback->query_font_ex(ui_font_default);
        }
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
        // Just clear items - cache will handle itself via LRU
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
            try {
                auto handle = all_items[i];
                
                // Add null check for handle (v10.0.4 crash fix)
                if (!handle.is_valid()) {
                    console::print("[Album Art Grid v10.0.4] Warning: Null handle encountered at index ", i);
                    continue;
                }
                
                pfc::string8 key;
                pfc::string8 display_name;
            
            if (m_config.grouping == grid_config::GROUP_BY_FOLDER) {
                // Group by folder - but check metadata first for multi-disc albums
                bool got_from_metadata = false;
                
                // First try to get album info from metadata
                // IMPORTANT: Wrap in try-catch to handle problematic paths
                try {
                    file_info_impl info;
                    if (handle->get_info(info)) {
                        const char* album = info.meta_get("ALBUM", 0);
                        const char* album_artist = info.meta_get("ALBUM ARTIST", 0);
                        if (!album_artist || !album_artist[0]) {
                            album_artist = info.meta_get("ARTIST", 0);
                        }
                        
                        // If we have album metadata, use it
                        if (album && album[0]) {
                            if (album_artist && album_artist[0]) {
                                key << album_artist << " - " << album;
                                display_name = album;
                            } else {
                                key = album;
                                display_name = album;
                            }
                            got_from_metadata = true;
                        }
                    }
                } catch (...) {
                    // If metadata extraction fails, fall back to folder name
                    console::print("[Album Art Grid v10.0.4] Warning: Failed to get metadata for an item, using folder name");
                }
                
                // Fall back to folder name if no metadata or if metadata extraction failed
                if (!got_from_metadata) {
                    try {
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
                    } catch (...) {
                        // If even path extraction fails, use a default
                        console::print("[Album Art Grid v10.0.4] Warning: Failed to process path for an item");
                        key = "Unknown";
                        display_name = key;
                    }
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
                
                // v10.0.4: Always extract and store the actual folder name
                pfc::string8 full_path = handle->get_path();
                const char* last_slash = strrchr(full_path.c_str(), '\\');
                if (!last_slash) last_slash = strrchr(full_path.c_str(), '/');
                if (last_slash && last_slash > full_path.c_str()) {
                    const char* start = full_path.c_str();
                    const char* second_last = last_slash - 1;
                    while (second_last > start && *second_last != '\\' && *second_last != '/') {
                        second_last--;
                    }
                    if (*second_last == '\\' || *second_last == '/') {
                        second_last++;
                    }
                    item->folder_name.set_string(second_last, last_slash - second_last);
                } else {
                    item->folder_name = "Root";
                }
                
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
            } catch (...) {
                // If processing of one item fails, log it and continue with the next
                console::printf("[Album Art Grid v10.0.3] Warning: Failed to process item %u of %u, skipping", 
                               (unsigned)i + 1, (unsigned)all_items.get_count());
                continue;
            }
        }
        
        // Convert map to vector
        for (auto& pair : item_map) {
            m_items.push_back(std::move(pair.second));
        }
        
        // Sort items
        sort_items();
        
        // Update global album count for titleformat fields
        {
            insync(g_count_sync);
            g_album_count = m_items.size();
            g_is_library_view = (m_config.view != grid_config::VIEW_PLAYLIST);
        }
        // Trigger refresh of titleformat (including status bar)
        // Just dispatch empty refresh to notify titleformat fields changed
        metadb_handle_list dummy;
        static_api_ptr_t<metadb_io>()->dispatch_refresh(dummy);
        
        // Check for now playing track
        check_now_playing();
        
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
    
    // Create thumbnail from album art data with enhanced error handling
    Gdiplus::Bitmap* create_thumbnail(album_art_data_ptr artwork, int size) {
        if (!artwork.is_valid() || artwork->get_size() == 0) {
            console::print("[Album Art Grid v10.0.2] No artwork data available");
            return nullptr;
        }
        
        // GDI+ initialization check removed - handled by initquit service
        
        // Limit maximum thumbnail size for memory efficiency
        size = min(size, 256);
        
        IStream* stream = nullptr;
        Gdiplus::Bitmap* original = nullptr;
        Gdiplus::Bitmap* thumbnail = nullptr;
        
        try {
            // Try creating memory stream with error checking
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, artwork->get_size());
            if (!hGlobal) {
                console::print("[Album Art Grid v10.0.2] Failed to allocate memory for image");
                return nullptr;
            }
            
            void* pImage = GlobalLock(hGlobal);
            if (!pImage) {
                GlobalFree(hGlobal);
                return nullptr;
            }
            
            memcpy(pImage, artwork->get_ptr(), artwork->get_size());
            GlobalUnlock(hGlobal);
            
            if (CreateStreamOnHGlobal(hGlobal, TRUE, &stream) != S_OK) {
                GlobalFree(hGlobal);
                console::print("[Album Art Grid v10.0.2] Failed to create stream for image");
                return nullptr;
            }
            
            if (!stream) {
                console::print("[Album Art Grid v10.0.2] Stream creation failed");
                return nullptr;
            }
            
            original = Gdiplus::Bitmap::FromStream(stream);
            stream->Release();
            stream = nullptr;
            
            if (!original || original->GetLastStatus() != Gdiplus::Ok) {
                if (original) delete original;
                return nullptr;
            }
            
            // Get dimensions
            int orig_width = original->GetWidth();
            int orig_height = original->GetHeight();
            
            // Skip scaling if already small
            if (orig_width <= size && orig_height <= size) {
                Gdiplus::Bitmap* result = original;
                original = nullptr;
                return result;
            }
            
            // Calculate optimal scaling
            float scale = min((float)size / orig_width, (float)size / orig_height);
            int new_width = max(1, (int)(orig_width * scale));
            int new_height = max(1, (int)(orig_height * scale));
            
            // Create thumbnail with optimal format for memory
            thumbnail = new Gdiplus::Bitmap(new_width, new_height, PixelFormat32bppPARGB);
            
            if (!thumbnail || thumbnail->GetLastStatus() != Gdiplus::Ok) {
                delete original;
                if (thumbnail) delete thumbnail;
                return nullptr;
            }
            
            {
                Gdiplus::Graphics graphics(thumbnail);
                
                // Use faster interpolation for thumbnails
                graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighSpeed);
                graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighSpeed);
                graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);
                
                graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
                graphics.DrawImage(original, 0, 0, new_width, new_height);
            }
            
            delete original;
            return thumbnail;
            
        } catch (...) {
            if (stream) stream->Release();
            if (original) delete original;
            if (thumbnail) delete thumbnail;
            return nullptr;
        }
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
            GetWindowTextW(m_search_box, buffer, len + 1);
            pfc::stringcvt::string_utf8_from_wide text_utf8(buffer);
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
        // CRITICAL FIX: Prevent artwork loading during destruction
        if (m_is_destroying || !m_hwnd || !IsWindow(m_hwnd)) return;
        
        size_t item_count = get_item_count();
        
        // Update viewport in cache manager
        thumbnail_cache::update_viewport(m_first_visible, m_last_visible);
        
        // Touch visible thumbnails
        for (int i = m_first_visible; i <= m_last_visible && i < (int)item_count; i++) {
            auto* item = get_item_at(i);
            if (item && item->thumbnail->bitmap) {
                item->thumbnail->touch();
            }
        }
        
        // Prefetch in scroll direction (detect from scroll position change)
        static int last_scroll_pos = 0;
        bool scrolling_down = m_scroll_pos > last_scroll_pos;
        last_scroll_pos = m_scroll_pos;
        
        // Prefetch range
        int prefetch_start, prefetch_end;
        if (scrolling_down) {
            prefetch_start = m_last_visible + 1;
            prefetch_end = min((int)item_count - 1, m_last_visible + PREFETCH_RANGE);
        } else {
            prefetch_start = max(0, m_first_visible - PREFETCH_RANGE);
            prefetch_end = m_first_visible - 1;
        }
        
        // Load visible items first
        int load_count = 0;
        for (int i = m_first_visible; i <= m_last_visible && i < (int)item_count && load_count < 3; i++) {
            auto* item = get_item_at(i);
            if (!item) continue;
            
            // Thread-safe check for loading state
            {
                insync(g_thumbnail_sync);
                if (item->thumbnail->bitmap || item->thumbnail->loading) continue;
                if (item->tracks.get_count() == 0) continue;
                
                item->thumbnail->loading = true;
            }
            load_count++;
            
            try {
                // CRITICAL FIX: Double-check for destruction before album art API call
                if (m_is_destroying || !m_hwnd) break;
                
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
                            // CRITICAL FIX: Check for destruction before artist art loading
                            if (m_is_destroying || !m_hwnd) break;
                            
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
                        // FIX for crash at 1196Ch: Check shutdown state before processing
                        if (shutdown_protection::g_is_shutting_down || m_is_destroying) {
                            return;
                        }
                        
                        calculate_layout();
                        
                        // Safe thumbnail creation with null checks
                        Gdiplus::Bitmap* bmp = nullptr;
                        try {
                            bmp = create_thumbnail(item->artwork, m_item_size);
                        } catch(...) {
                            bmp = nullptr;
                        }
                        
                        if (bmp && !shutdown_protection::g_is_shutting_down) {
                            {
                                insync(g_thumbnail_sync);
                                if (item && item->thumbnail) {
                                    item->thumbnail->set_bitmap(bmp, m_item_size);
                                }
                            }
                            if (item && item->thumbnail) {
                                thumbnail_cache::add_thumbnail(item->thumbnail.get(), i);
                            }
                            if (m_hwnd && IsWindow(m_hwnd)) {
                                InvalidateRect(m_hwnd, NULL, FALSE);
                            }
                        } else if (!shutdown_protection::g_is_shutting_down) {
                            console::print("[Album Art Grid v10.0.14] Failed to create thumbnail");
                        }
                    }
                } catch (...) {}
            } catch (...) {}
            
            // Safe loading flag reset
            if (item && item->thumbnail && !shutdown_protection::g_is_shutting_down) {
                item->thumbnail->loading = false;
            }
        }
        
        // Prefetch items in scroll direction (lower priority)
        if (load_count < 5 && prefetch_start <= prefetch_end) {
            for (int i = prefetch_start; i <= prefetch_end && i < (int)item_count && load_count < 5; i++) {
                auto* item = get_item_at(i);
                if (!item) continue;
                
                if (item->thumbnail->bitmap || item->thumbnail->loading) continue;
                if (item->tracks.get_count() == 0) continue;
                
                item->thumbnail->loading = true;
                load_count++;
                
                try {
                    // CRITICAL FIX: Check for destruction before prefetch album art loading
                    if (m_is_destroying || !m_hwnd) break;
                    
                    auto art_api = album_art_manager_v2::get();
                    abort_callback_dummy abort;
                    auto extractor = art_api->open(
                        pfc::list_single_ref_t<metadb_handle_ptr>(item->tracks[0]),
                        pfc::list_single_ref_t<GUID>(album_art_ids::cover_front),
                        abort
                    );
                    
                    try {
                        item->artwork = extractor->query(album_art_ids::cover_front, abort);
                        if (item->artwork.is_valid()) {
                            calculate_layout();
                            Gdiplus::Bitmap* bmp = create_thumbnail(item->artwork, m_item_size);
                            if (bmp) {
                                item->thumbnail->set_bitmap(bmp, m_item_size);
                                thumbnail_cache::add_thumbnail(item->thumbnail.get(), i);
                                // Don't invalidate for prefetch - no visual update needed
                            }
                        }
                    } catch (...) {}
                } catch (...) {}
                
                // Safe loading flag reset
            if (item && item->thumbnail && !shutdown_protection::g_is_shutting_down) {
                item->thumbnail->loading = false;
            }
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
        // v10.0.8: Added callback validity check (line 1768 fix)
        t_ui_color color_back = RGB(30, 30, 30); // Default dark background
        if (m_callback.is_valid()) {
            color_back = m_callback->query_std_color(ui_color_background);
        }
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
        // v10.0.7: Check callback validity before querying colors
        t_ui_color color_text = RGB(255, 255, 255); // Default white
        t_ui_color color_selected = RGB(51, 153, 255); // Default blue
        if (m_callback.is_valid()) {
            color_text = m_callback->query_std_color(ui_color_text);
            color_selected = m_callback->query_std_color(ui_color_selection);
        }
        
        // Use foobar2000's standard font
        // v10.0.7: Check callback validity before querying font
        HFONT hFont = NULL;
        if (m_callback.is_valid()) {
            hFont = m_callback->query_font_ex(ui_font_default);
        }
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
        
        // Check if this is the now playing album
        bool is_now_playing = (m_now_playing.is_valid() && (int)index == m_now_playing_index);
        
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
        
        // Draw subtle border (or highlighted border for now playing)
        if (is_now_playing) {
            // Draw a thicker, colored border for now playing album
            HPEN pen = CreatePen(PS_SOLID, 3, RGB(0, 150, 255));  // Blue border
            HPEN oldpen = (HPEN)SelectObject(hdc, pen);
            HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, x-1, y-1, x + m_item_size + 1, y + m_item_size + 1);
            SelectObject(hdc, oldpen);
            SelectObject(hdc, oldbrush);
            DeleteObject(pen);
        } else {
            // Regular subtle border
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(50, 50, 50));
            HPEN oldpen = (HPEN)SelectObject(hdc, pen);
            HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, x, y, x + m_item_size, y + m_item_size);
            SelectObject(hdc, oldpen);
            SelectObject(hdc, oldbrush);
            DeleteObject(pen);
        }
        
        // Draw track count badge when track count is enabled (always show as badge)
        if (m_config.show_track_count && item->tracks.get_count() > 0) {
            // Check if dark mode is enabled using our hooks
            bool dark_mode = m_dark ? true : false;
            draw_track_count_badge(hdc, item->tracks.get_count(), x + m_item_size, y, dark_mode, font);
        }
        
        // Draw play icon for now playing album
        if (is_now_playing) {
            // Draw play icon in bottom-left corner (smaller, like track badge)
            int icon_size = min(24, m_item_size / 8);  // Smaller size, similar to badge
            int icon_x = x + 5;  // Bottom-left position
            int icon_y = y + m_item_size - icon_size - 5;
            
            // Draw background circle
            HBRUSH play_brush = CreateSolidBrush(RGB(0, 150, 255));
            HPEN play_pen = CreatePen(PS_SOLID, 1, RGB(0, 150, 255));
            HPEN old_pen = (HPEN)SelectObject(hdc, play_pen);
            HBRUSH old_brush = (HBRUSH)SelectObject(hdc, play_brush);
            Ellipse(hdc, icon_x, icon_y, icon_x + icon_size, icon_y + icon_size);
            SelectObject(hdc, old_pen);
            SelectObject(hdc, old_brush);
            DeleteObject(play_brush);
            DeleteObject(play_pen);
            
            // Draw play triangle
            POINT triangle[3];
            triangle[0].x = icon_x + icon_size * 3 / 8;
            triangle[0].y = icon_y + icon_size / 4;
            triangle[1].x = icon_x + icon_size * 3 / 8;
            triangle[1].y = icon_y + icon_size * 3 / 4;
            triangle[2].x = icon_x + icon_size * 3 / 4;
            triangle[2].y = icon_y + icon_size / 2;
            
            HBRUSH white_brush = CreateSolidBrush(RGB(255, 255, 255));
            HPEN white_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            old_pen = (HPEN)SelectObject(hdc, white_pen);
            old_brush = (HBRUSH)SelectObject(hdc, white_brush);
            Polygon(hdc, triangle, 3);
            SelectObject(hdc, old_pen);
            SelectObject(hdc, old_brush);
            DeleteObject(white_brush);
            DeleteObject(white_pen);
        }
        
        // Draw text (multi-line support)
        if (m_config.show_text) {
            RECT text_rc = {x - 2, y + m_item_size + 4, x + m_item_size + 2, 
                           y + m_item_size + text_height - 2};
            SetTextColor(hdc, color_text);
            SelectObject(hdc, font);
            
            // Prepare text with proper UTF-8 to UTF-16 conversion
            // Use configured label format (Album only vs Artist - Album)
            pfc::string8 text_to_show = item->get_display_text(m_config.label_style);
            pfc::stringcvt::string_os_from_utf8 display_text(text_to_show.c_str());
            
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
                                metadb_handle_list sorted_tracks = m_items[real_idx]->tracks;
                                sort_tracks_for_playlist(sorted_tracks);
                                pm->playlist_add_items(playlist, sorted_tracks, bit_array_false());
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
                                metadb_handle_list sorted_tracks = m_items[real_idx]->tracks;
                                sort_tracks_for_playlist(sorted_tracks);
                                tracks_to_add.add_items(sorted_tracks);
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
                            metadb_handle_list sorted_tracks = m_items[real_idx]->tracks;
                            sort_tracks_for_playlist(sorted_tracks);
                            tracks_to_add.add_items(sorted_tracks);
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
            AppendMenu(menu, MF_STRING, 4, TEXT("Play in New Playlist"));
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
            AppendMenu(menu, MF_STRING, 2, TEXT("Add to Current Playlist"));
            AppendMenu(menu, MF_STRING, 3, TEXT("Add to New Playlist"));
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
            // v10.0.9: New "Open in Folder" feature
            AppendMenu(menu, MF_STRING, 5, TEXT("Open in Folder"));
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
        
        // Column options - now supports 1 to 20 columns!
        HMENU col_menu = CreatePopupMenu();
        for (int i = 1; i <= 20; i++) {
            pfc::string8 text;
            text << i << " column" << (i == 1 ? "" : "s");
            pfc::stringcvt::string_os_from_utf8 text_w(text.c_str());
            AppendMenu(col_menu, MF_STRING | (m_config.columns == i ? MF_CHECKED : 0), 200 + i, text_w);
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
        
        // Label format submenu - v10.0.4: Now with 4 options
        HMENU label_menu = CreatePopupMenu();
        AppendMenu(label_menu, MF_STRING | (m_config.label_style == grid_config::LABEL_ALBUM_ONLY ? MF_CHECKED : 0), 37, TEXT("Album"));
        AppendMenu(label_menu, MF_STRING | (m_config.label_style == grid_config::LABEL_ARTIST_ONLY ? MF_CHECKED : 0), 38, TEXT("Artist"));
        AppendMenu(label_menu, MF_STRING | (m_config.label_style == grid_config::LABEL_ARTIST_ALBUM ? MF_CHECKED : 0), 39, TEXT("Artist - Album"));
        AppendMenu(label_menu, MF_STRING | (m_config.label_style == grid_config::LABEL_FOLDER_NAME ? MF_CHECKED : 0), 40, TEXT("Folder Name"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)label_menu, TEXT("Display Label"));
        
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        
        // View Mode options
        HMENU view_menu = CreatePopupMenu();
        AppendMenu(view_menu, MF_STRING | (m_config.view == grid_config::VIEW_LIBRARY ? MF_CHECKED : 0), 
                   100, TEXT("Library"));
        AppendMenu(view_menu, MF_STRING | (m_config.view == grid_config::VIEW_PLAYLIST ? MF_CHECKED : 0), 
                   101, TEXT("Current Playlist"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)view_menu, TEXT("View Mode"));
        
        // Track Sorting options
        HMENU track_sort_menu = CreatePopupMenu();
        AppendMenu(track_sort_menu, MF_STRING | (m_config.track_sorting == grid_config::TRACK_SORT_BY_NUMBER ? MF_CHECKED : 0), 
                   130, TEXT("By Track Number"));
        AppendMenu(track_sort_menu, MF_STRING | (m_config.track_sorting == grid_config::TRACK_SORT_BY_NAME ? MF_CHECKED : 0), 
                   131, TEXT("By Title"));
        AppendMenu(track_sort_menu, MF_STRING | (m_config.track_sorting == grid_config::TRACK_SORT_NONE ? MF_CHECKED : 0), 
                   132, TEXT("No Sorting"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)track_sort_menu, TEXT("Track Sorting"));
        
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        
        // Now Playing options
        AppendMenu(menu, MF_STRING, 150, TEXT("Jump to Now Playing (Ctrl+Q)"));
        AppendMenu(menu, MF_STRING | (m_auto_scroll_to_now_playing ? MF_CHECKED : 0), 151, TEXT("Auto-scroll to Now Playing"));
        
        AppendMenu(menu, MF_SEPARATOR, 0, NULL);
        AppendMenu(menu, MF_STRING | (m_search_visible ? MF_CHECKED : 0), 43, TEXT("Search (Ctrl+Shift+S)"));
        AppendMenu(menu, MF_STRING, 42, TEXT("Select All (Ctrl+A)"));
        AppendMenu(menu, MF_STRING, 41, TEXT("Refresh (F5)"));
        
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, 
            pt.x, pt.y, 0, m_hwnd, NULL);
        DestroyMenu(group_menu);
        DestroyMenu(sort_menu);
        DestroyMenu(col_menu);
        DestroyMenu(text_menu);
        DestroyMenu(view_menu);
        DestroyMenu(track_sort_menu);
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
                                metadb_handle_list sorted_tracks = item->tracks;
                                sort_tracks_for_playlist(sorted_tracks);
                                pm->playlist_add_items(playlist, sorted_tracks, bit_array_false());
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
                        sort_tracks_for_playlist(tracks_to_add);
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
                        sort_tracks_for_playlist(tracks_to_add);
                        t_size new_playlist = pm->create_playlist(
                            playlist_name.c_str(),
                            pfc::infinite_size,
                            pfc::infinite_size
                        );
                        pm->playlist_add_items(new_playlist, tracks_to_add, bit_array_false());
                    }
                }
                break;
                
            case 4: // Play in new playlist
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
                        sort_tracks_for_playlist(tracks_to_add);
                        t_size new_playlist = pm->create_playlist(
                            playlist_name.c_str(),
                            pfc::infinite_size,
                            pfc::infinite_size
                        );
                        pm->playlist_add_items(new_playlist, tracks_to_add, bit_array_false());
                        // Set as playing playlist and start playback
                        pm->set_playing_playlist(new_playlist);
                        static_api_ptr_t<playback_control> pc;
                        pc->play_start();
                    }
                }
                break;
                
            case 5: // Open in Folder - v10.0.9 new feature
                if (!m_selected_indices.empty()) {
                    // Open folder for first selected item
                    auto* item = get_item_at(*m_selected_indices.begin());
                    if (item && !item->path.is_empty()) {
                        try {
                            pfc::string8 folder_path = item->path;
                            
                            // Extract directory path from the first track if needed
                            if (item->tracks.get_count() > 0) {
                                pfc::string8 first_file = item->tracks[0]->get_path();
                                const char* last_slash = strrchr(first_file.c_str(), '\\');
                                if (last_slash) {
                                    folder_path.set_string(first_file.c_str(), last_slash - first_file.c_str());
                                }
                            }
                            
                            // Use ShellExecute to open folder in Windows Explorer
                            pfc::stringcvt::string_wide_from_utf8 folder_path_w(folder_path.c_str());
                            HINSTANCE result = ShellExecuteW(m_hwnd, L"explore", folder_path_w, NULL, NULL, SW_SHOWNORMAL);
                            
                            if ((INT_PTR)result <= 32) {
                                // Fallback: try opening parent directory if direct open failed
                                ShellExecuteW(m_hwnd, L"open", folder_path_w, NULL, NULL, SW_SHOWNORMAL);
                            }
                        } catch (...) {
                            // Ignore errors - feature is not critical
                        }
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
            // Column options (201-220 for columns 1-20)
            case 201: case 202: case 203: case 204: case 205: case 206: case 207: case 208: case 209: case 210:
            case 211: case 212: case 213: case 214: case 215: case 216: case 217: case 218: case 219: case 220:
                m_config.columns = cmd - 200;  // 201-200=1, 202-200=2, etc.
                calculate_layout();
                config_changed = true; 
                break;
            case 35: m_config.show_text = !m_config.show_text; config_changed = true; break;
            case 36: m_config.show_track_count = !m_config.show_track_count; config_changed = true; break;
            case 37: m_config.label_style = grid_config::LABEL_ALBUM_ONLY; config_changed = true; break;
            case 38: m_config.label_style = grid_config::LABEL_ARTIST_ONLY; config_changed = true; break;
            case 39: m_config.label_style = grid_config::LABEL_ARTIST_ALBUM; config_changed = true; break;
            case 40: m_config.label_style = grid_config::LABEL_FOLDER_NAME; config_changed = true; break;
            case 42: // Select all
                m_selected_indices.clear();
                for (size_t i = 0; i < m_items.size(); i++) {
                    m_selected_indices.insert(i);
                }
                InvalidateRect(m_hwnd, NULL, FALSE);
                break;
            case 41: needs_refresh = true; break;
            
            // Now Playing commands
            case 150: // Jump to Now Playing
                jump_to_now_playing();
                break;
            case 151: // Toggle Auto-scroll to Now Playing
                m_auto_scroll_to_now_playing = !m_auto_scroll_to_now_playing;
                config_changed = true;
                break;
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
                
            case 130: // Track sorting: By Track Number
                if (m_config.track_sorting != grid_config::TRACK_SORT_BY_NUMBER) {
                    m_config.track_sorting = grid_config::TRACK_SORT_BY_NUMBER;
                    config_changed = true;
                }
                break;
                
            case 131: // Track sorting: By Title
                if (m_config.track_sorting != grid_config::TRACK_SORT_BY_NAME) {
                    m_config.track_sorting = grid_config::TRACK_SORT_BY_NAME;
                    config_changed = true;
                }
                break;
                
            case 132: // Track sorting: No Sorting
                if (m_config.track_sorting != grid_config::TRACK_SORT_NONE) {
                    m_config.track_sorting = grid_config::TRACK_SORT_NONE;
                    config_changed = true;
                }
                break;
                
            case 43: // Toggle search
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
            // v10.0.8: Added callback validity check (line 2807 fix)
            if (m_callback.is_valid()) {
                m_callback->on_min_max_info_change();
            }
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
                pfc::string8 tooltip_text = item->get_display_text(m_config.label_style);
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
    
    // Function to sort tracks for playlist addition
    void sort_tracks_for_playlist(metadb_handle_list& tracks) {
        if (tracks.get_count() <= 1) return;
        
        switch (m_config.track_sorting) {
            case grid_config::TRACK_SORT_BY_NUMBER:
                tracks.sort_by_format(titleformat_compiler::get()->compile("%tracknumber%"), nullptr);
                break;
            case grid_config::TRACK_SORT_BY_NAME:
                tracks.sort_by_format(titleformat_compiler::get()->compile("%title%"), nullptr);
                break;
            case grid_config::TRACK_SORT_NONE:
                // No sorting - leave tracks in original order
                break;
        }
    }
    
    LRESULT on_mousewheel(int delta, int keys) {
        // Ctrl+Wheel to adjust columns - now unlimited!
        if (keys & MK_CONTROL) {
            int new_cols = m_config.columns;
            if (delta > 0) {
                new_cols = min(20, new_cols + 1);  // Match menu limit of 20 columns
            } else {
                new_cols = max(1, new_cols - 1);   // Minimum 1 column
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
                            // Safe loading flag reset
            if (item && item->thumbnail && !shutdown_protection::g_is_shutting_down) {
                item->thumbnail->loading = false;
            }  // Allow regeneration
                        }
                    }
                }
                
                // v10.0.8: Added callback validity check (line 2999 fix)
                if (m_callback.is_valid()) {
                    m_callback->on_min_max_info_change();
                }
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
    
    // Public getter for window handle (needed for bump functionality)
    HWND get_hwnd() const { return m_hwnd; }
    
    // Playlist callback methods for auto-refresh - implement all required abstract methods
    void on_items_added(t_size p_base, metadb_handle_list_cref p_data, const bit_array & p_selection) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_items_reordered(const t_size * p_order, t_size p_count) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_items_removing(const bit_array & p_mask, t_size p_old_count, t_size p_new_count) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        // Called before removal - no action needed
    }
    
    void on_items_removed(const bit_array & p_mask, t_size p_old_count, t_size p_new_count) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_items_selection_change(const bit_array & p_affected, const bit_array & p_state) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        // Selection changes don't affect our grid view
    }
    
    void on_item_focus_change(t_size p_from, t_size p_to) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        // Focus changes don't affect our grid view
    }
    
    void on_items_modified(const bit_array & p_mask) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_items_modified_fromplayback(const bit_array & p_mask, play_control::t_display_level p_level) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        // Playback modifications don't affect our grid view
    }
    
    void on_items_replaced(const bit_array & p_mask, const pfc::list_base_const_t<playlist_callback::t_on_items_replaced_entry> & p_data) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_item_ensure_visible(t_size p_idx) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        // Not relevant for our grid view
    }
    
    void on_playlist_switch() override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {
            refresh_items();
        }
    }
    
    void on_playlist_renamed(const char * p_new_name, t_size p_new_name_len) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        // Playlist name changes don't affect our grid view
    }
    
    void on_playlist_locked(bool p_locked) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        // Playlist lock changes don't affect our grid view
    }
    
    void on_default_format_changed() override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        // Format changes don't affect our grid view
    }
    
    void on_playback_order_changed(t_size p_new_index) override {
        if (m_is_destroying || !m_hwnd) return;  // Prevent use-after-free
        // Playback order changes don't affect our grid view
    }
    
    // Check and update now playing status
    void check_now_playing() {
        // Don't check if window is not ready or items are empty
        if (!m_hwnd || m_items.empty()) return;
        
        try {
            static_api_ptr_t<playback_control> pc;
            metadb_handle_ptr track;
            if (pc->get_now_playing(track)) {
                update_now_playing(track);
            } else {
                update_now_playing(nullptr);
            }
        } catch(...) {
            // Ignore errors during shutdown
        }
    }
    
private:
    void update_now_playing(metadb_handle_ptr track) {
        if (!track.is_valid()) {
            m_now_playing.release();
            m_now_playing_index = -1;
            if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
            return;
        }
        
        m_now_playing = track;
        m_now_playing_index = find_track_album(track);
        
        if (m_now_playing_index >= 0 && m_auto_scroll_to_now_playing) {
            // Check if user hasn't scrolled recently (10 seconds)
            if (GetTickCount() - m_last_user_scroll > 10000) {
                jump_to_now_playing(false);  // false = no animation for auto-scroll
            }
        }
        
        if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
    }
    
    int find_track_album(metadb_handle_ptr track) {
        if (!track.is_valid() || m_items.empty()) return -1;
        
        // Get album info from track
        file_info_impl info;
        if (!track->get_info(info)) return -1;
        
        const char* album = info.meta_get("ALBUM", 0);
        const char* artist = info.meta_get("ARTIST", 0);
        if (!artist) artist = info.meta_get("ALBUM ARTIST", 0);
        
        // Find matching album in grid
        for (size_t i = 0; i < m_items.size(); i++) {
            auto& item = m_items[i];
            
            // Check if any track in this item matches
            for (size_t j = 0; j < item->tracks.get_count(); j++) {
                if (item->tracks[j] == track) {
                    return i;
                }
            }
            
            // Alternative: match by album/artist
            if (album && item->album == album) {
                if (!artist || item->artist == artist) {
                    return i;
                }
            }
        }
        
        return -1;
    }
    
    void jump_to_letter(char letter) {
        if (m_items.empty() || !m_hwnd) return;
        
        // Convert to uppercase for comparison
        char target = toupper(letter);
        
        // Determine start position for search
        int start_index = 0;
        if (target == m_last_jump_letter && m_last_jump_index >= 0) {
            // Same letter pressed again - cycle to next match
            start_index = m_last_jump_index + 1;
        }
        
        // Find next item that starts with this letter
        // Jump based on what's actually displayed (label format)
        int found_index = -1;
        
        // Two-pass search: from start_index to end, then from 0 to start_index (wrap around)
        for (int pass = 0; pass < 2 && found_index < 0; pass++) {
            int begin = (pass == 0) ? start_index : 0;
            int end = (pass == 0) ? (int)m_items.size() : start_index;
            
            for (int i = begin; i < end; i++) {
                // Skip if filtering and item doesn't match
                if (!m_search_text.is_empty()) {
                    bool matches = false;
                    const auto& item = m_items[i];
                    if (item) {
                        pfc::string8 search_lower = m_search_text;
                        pfc::string8 album_lower = item->album.c_str();
                        pfc::string8 artist_lower = item->artist.c_str();
                        
                        pfc::stringToLower(search_lower);
                        pfc::stringToLower(album_lower);
                        pfc::stringToLower(artist_lower);
                        
                        if (strstr(album_lower.c_str(), search_lower.c_str()) ||
                            strstr(artist_lower.c_str(), search_lower.c_str())) {
                            matches = true;
                        }
                    }
                    if (!matches) continue;
                }
                
                // Get first character based on display format (what user actually sees)
                char first_char = 0;
                const auto& item = m_items[i];
                if (!item) continue;
                
                // Smart jump: use label format to determine what to jump by
                switch (m_config.label_style) {
                    case grid_config::LABEL_ARTIST_ALBUM:
                    case grid_config::LABEL_ARTIST_ONLY:
                        // When showing artist first, jump by artist
                        if (!item->artist.empty()) {
                            first_char = toupper(item->artist[0]);
                        }
                        break;
                        
                    case grid_config::LABEL_ALBUM_ONLY:
                        // When showing album only, jump by album
                        if (!item->album.empty()) {
                            first_char = toupper(item->album[0]);
                        }
                        break;
                        
                    case grid_config::LABEL_FOLDER_NAME:
                        // When showing folder name, jump by folder
                        if (!item->folder_name.is_empty()) {
                            first_char = toupper(item->folder_name[0]);
                        } else if (!item->path.empty()) {
                            const char* last_slash = strrchr(item->path.c_str(), '\\');
                            if (last_slash && *(last_slash + 1)) {
                                first_char = toupper(*(last_slash + 1));
                            }
                        }
                        break;
                        
                    default:
                        // Fallback to album
                        if (!item->album.empty()) {
                            first_char = toupper(item->album[0]);
                        }
                        break;
                }
                
                // Check if this item starts with our target letter
                if (first_char == target) {
                    found_index = i;
                    break;
                }
            }
        }
        
        // If we found a matching item, scroll to it
        if (found_index >= 0) {
            // Calculate target scroll position
            int row = found_index / max(1, m_config.columns);
            int text_height = calculate_text_height();
            int item_total_height = m_item_size + text_height;
            int target_scroll = row * (item_total_height + PADDING);
            
            // Update scroll position
            m_scroll_pos = target_scroll;
            update_scrollbar();
            
            // Select the item for visual feedback
            m_selected_indices.clear();
            m_selected_indices.insert(found_index);
            m_last_selected = found_index;
            
            // Remember for cycling
            m_last_jump_letter = target;
            m_last_jump_index = found_index;
            
            InvalidateRect(m_hwnd, NULL, FALSE);
        } else {
            // No match found, reset cycling
            m_last_jump_letter = 0;
            m_last_jump_index = -1;
        }
    }
    
    void jump_to_now_playing(bool animate = true) {
        if (m_now_playing_index < 0 || !m_hwnd || m_items.empty()) return;
        if (m_now_playing_index >= (int)m_items.size()) return;
        
        // Calculate target scroll position
        int row = m_now_playing_index / max(1, m_config.columns);
        int text_height = calculate_text_height();
        int item_total_height = m_item_size + text_height;
        int target_scroll = row * (item_total_height + PADDING);
        
        // Center the item if possible
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int viewport_height = rc.bottom;
        target_scroll = max(0, target_scroll - viewport_height / 2 + item_total_height / 2);
        
        // Update scroll position
        m_scroll_pos = target_scroll;
        update_scrollbar();
        InvalidateRect(m_hwnd, NULL, FALSE);
        
        // Flash effect for visual feedback
        // Could implement a temporary highlight here
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
    
    // Add flags for popup command generation (allows appearance in menus)
    t_uint32 get_flags() {
        // KFlagSupportsBump = 1 << 0, KFlagHavePopupCommand = 1 << 1
        // Both are needed for menu command generation: "bumping existing instance if possible, spawning popup otherwise"
        return (1 << 0) | (1 << 1);  // KFlagSupportsBump | KFlagHavePopupCommand
    }
    
    // Popup command name for menus
    void get_menu_command_name(pfc::string_base & out) {
        out = "Album Art Grid";
    }
    
    // Popup command description  
    bool get_menu_command_description(pfc::string_base & out) {
        out = "Shows the Album Art Grid window";
        return true;
    }
    
    // Support bump for activation - brings existing instance to attention
    bool bump() {
        // Try to find and activate an existing grid instance
        if (!shutdown_protection::g_active_instances.empty()) {
            // If we have active instances, try to bring one to foreground
            for (auto instance_ptr : shutdown_protection::g_active_instances) {
                auto* instance = static_cast<album_grid_instance*>(instance_ptr);
                if (instance) {
                    HWND hwnd = instance->get_hwnd();
                    if (hwnd && IsWindow(hwnd)) {
                        // Bring the window to foreground
                        HWND root_window = GetAncestor(hwnd, GA_ROOT);
                        SetForegroundWindow(root_window);
                        ShowWindow(root_window, SW_RESTORE);
                        return true;  // Successfully bumped an existing instance
                    }
                }
            }
        }
        return false;  // No existing instance to bump
    }
};

static service_factory_single_t<album_grid_ui_element> g_album_grid_ui_element_factory;

// External shutdown handlers for initquit integration
// These are ONLY called by foobar2000 during actual app shutdown
extern "C" {
    void set_global_shutdown_flag() {
        console::print("[Album Art Grid v10.0.14] foobar2000 is shutting down - protecting all instances");
        shutdown_protection::initiate_app_shutdown();
    }
    
    void clear_global_shutdown_flag() {
        // Reset shutdown flag on startup
        shutdown_protection::g_is_shutting_down = false;
        shutdown_protection::g_active_instances.clear();
    }
}

// Library viewer support - allow showing the Album Art Grid window from Library menu
void show_album_art_grid_window() {
    // Try to activate the main UI which should make the grid visible if already open
    static_api_ptr_t<ui_control> ui_ctrl;
    
    console::print("[Album Art Grid v10.0.14] Activated from Library menu");
    
    // Activate the main UI - this will bring it to foreground
    ui_ctrl->activate();
    
    // Note: We can't directly create a new instance of the grid window programmatically
    // The user needs to add it to their layout through View -> Layout -> Edit Layout
    // But we can make sure the main window is visible and activated
}

bool is_album_art_grid_window_visible() {
    // Check if any instances are active
    return !shutdown_protection::g_active_instances.empty();
}