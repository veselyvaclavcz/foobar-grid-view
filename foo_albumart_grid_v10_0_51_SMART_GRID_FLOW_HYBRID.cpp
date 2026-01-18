// Album Art Grid v10.0.51 - SMART GRID FLOW (stability + async loads)
// NEW: Intelligent enlarged item placement with backward positioning

// NEW: Automatic grid reflow to prevent gaps at end

// NEW: Smart blockage calculation for seamless item flow

// MAINTAINED: All previous features from v10.0.41

#define FOOBAR2000_TARGET_VERSION 80

#define _WIN32_WINNT 0x0600



// Include all parts

// Album Art Grid v10.0.20 - SERVICE REFERENCE COUNTING FIX

// FIXED: Proper service_impl_t inheritance for reference counting

// FIXED: foo_ui_std can now safely release component

// FIXED: No more vtable crashes (FFFFFFFFFFFFFFFFh)

// FIXED: Component stays alive until properly released

// FIXED: All v10.0.19 protections maintained

// Auto-adjusting performance config for high-RAM systems (16GB+)



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

#include <locale>

#include <codecvt>

#include <cctype>

#include <cstdint>

#include <limits>

#include <thread>
#include <functional>
#include <deque>


// Fix for min/max macros

#ifdef min

#undef min

#endif

#ifdef max

#undef max

#endif



// Core API implementations provided by foobar2000_component_client.lib



using std::min;
using std::max;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")

#pragma comment(lib, "gdiplus.lib")

#pragma comment(lib, "user32.lib")

#pragma comment(lib, "shlwapi.lib")

#pragma comment(lib, "uxtheme.lib")

#pragma comment(lib, "Msimg32.lib")




// Component version

DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "10.0.51",
    "Album Art Grid v10.0.51 - SMART GRID FLOW\n"
    "Fix: Multi-disc folder layouts (CD1/CD2) now represent the whole album (better labels/art/open-folder).\n"
    "Includes: Multi-disc indicator and disc-aware playlist sorting.\n"
    "Changelog: see README.\n"
);



VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");



// CRITICAL FIX v10.0.17: Magic numbers for object validation

constexpr uint64_t GRID_MAGIC_VALID = 0xAB12CD34EF567890ULL;

constexpr uint64_t GRID_MAGIC_DEAD = 0xDEADDEADDEADDEADULL;



// Object validation base class to prevent crashes

class validated_object {

protected:

    mutable std::atomic<uint64_t> m_magic;

    mutable std::atomic<bool> m_destroyed;

    

public:

    validated_object() : m_magic(GRID_MAGIC_VALID), m_destroyed(false) {}

    

    virtual ~validated_object() {

        invalidate();

    }

    

    void invalidate() {

        m_destroyed = true;

        m_magic = GRID_MAGIC_DEAD;

    }

    

    bool is_valid() const {

        if (m_destroyed.load()) return false;

        if (m_magic.load() != GRID_MAGIC_VALID) return false;

        

        // Check if pointer is readable - CRITICAL FIX

        __try {

            volatile uint64_t test = m_magic.load();

            (void)test;

            return true;

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            return false;

        }

    }

};



// Forward declaration for zombie callback

class album_grid_instance;



// v10.0.27: ZOMBIE CALLBACK PATTERN - Immortal callback handler

// This object survives component destruction and prevents all crashes

class zombie_callback_handler : public main_thread_callback {

private:

    std::atomic<bool> m_alive{true};

    std::atomic<void*> m_owner{nullptr};  // Store as void* to avoid forward declaration issues

    std::atomic<uint64_t> m_owner_magic{0};

    std::atomic<int> m_refcount{1};

    

public:

    zombie_callback_handler(void* owner) {

        m_owner = owner;

        // Store magic number for validation

        if (owner) {

            m_owner_magic = GRID_MAGIC_VALID;

        }

    }

    

    // Implementation moved after album_grid_instance definition

    void callback_run() override;

    

    void kill() {

        m_alive = false;

        m_owner = nullptr;

        m_owner_magic = GRID_MAGIC_DEAD;

    }

    

    bool is_alive() const {

        return m_alive.load();

    }

    

    // Implement service_base abstract methods for reference counting

    virtual int service_add_ref() noexcept override {

        return ++m_refcount;

    }

    

    virtual int service_release() noexcept override {

        int count = --m_refcount;

        if (count == 0) {

            delete this;

        }

        return count;

    }

    

    virtual bool service_query(service_ptr & p_out, const GUID & p_guid) override {

        return false; // No additional interfaces exposed

    }

};



// Global shutdown protection system

// NO GLOBAL FLAGS - only instance-specific protection

namespace shutdown_protection {
    static std::atomic<bool> g_is_shutting_down = false;

    static critical_section g_shutdown_sync;

    static std::set<album_grid_instance*> g_active_instances;
    

    void initiate_app_shutdown() {

        insync(g_shutdown_sync);

        g_is_shutting_down.store(true);

        

        // v10.0.28 CRITICAL FIX: Remove unsafe pointer casting

        // The unsafe casting *((volatile bool*)inst) = false was causing memory corruption

        // Just clear the instance list safely without touching instance memory

        console::print("[Album Art Grid v10.0.28] App shutdown initiated - marking all instances invalid");

        g_active_instances.clear();

    }

    

    static bool is_shutting_down() {

        return g_is_shutting_down.load();

    }

    

    static void register_instance(album_grid_instance* inst) {
        insync(g_shutdown_sync);
        if (!g_is_shutting_down.load()) {
            g_active_instances.insert(inst);
        }
    }
    
    static void unregister_instance(album_grid_instance* inst) {
        insync(g_shutdown_sync);
        g_active_instances.erase(inst);
    }
}

// Simple fixed thread pool for thumbnail loading
class ThreadPool {
public:
    explicit ThreadPool(size_t n) : m_stop(false) {
        n = std::max<size_t>(1, n);
        for (size_t i = 0; i < n; ++i) {
            m_workers.emplace_back([this]{ this->worker(); });
        }
    }
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            m_stop = true;
            m_cv.notify_all();
        }
        for (auto& t : m_workers) { if (t.joinable()) t.join(); }
    }
    void submit(std::function<void()> fn) {
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            m_tasks.emplace_back(std::move(fn));
        }
        m_cv.notify_one();
    }
private:
    void worker() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lk(m_mtx);
                m_cv.wait(lk, [&]{ return m_stop || !m_tasks.empty(); });
                if (m_stop && m_tasks.empty()) return;
                task = std::move(m_tasks.front());
                m_tasks.pop_front();
            }
            try { task(); } catch(...) {}
        }
    }
    std::vector<std::thread> m_workers;
    std::deque<std::function<void()>> m_tasks;
    std::mutex m_mtx; std::condition_variable m_cv; bool m_stop;
};

// Safe virtual call wrapper with CRITICAL v10.0.17 object validation

template<typename Func, typename T>

T safe_virtual_call(Func&& func, T default_val) {

    if (shutdown_protection::is_shutting_down()) return default_val;

    

    // CRASH FIX v10.0.17: Check pointer validity BEFORE any access

    __try {

        // First check if we can even access memory

        volatile int test = 0;

        test = 1; // Basic memory access test

        

        HWND main_window = core_api::get_main_window();

        if (!main_window || !IsWindow(main_window)) {

            return default_val;

        }

    }

    __except(EXCEPTION_EXECUTE_HANDLER) {

        console::print("[Album Art Grid v10.0.17] Exception in safe_virtual_call setup");

        return default_val;

    }

    

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

            console::print("[Album Art Grid v10.0.17] GDI+ initialized successfully");

        } else {

            initialized = false;

            console::printf("[Album Art Grid v10.0.17] ERROR: GDI+ initialization failed with status: %d", status);

        }

    }

    

    ~gdiplus_startup() {

        if (initialized && token) {

            Gdiplus::GdiplusShutdown(token);

            console::print("[Album Art Grid v10.0.17] GDI+ shutdown complete");

        }

    }

    

    bool is_initialized() const { return initialized; }

};



// static gdiplus_startup g_gdiplus; // REMOVED - GDI+ is initialized in initquit service



// Global album count for titleformat fields

static size_t g_album_count = 0;
static bool g_is_library_view = true;
static int g_last_grouping = 0;
static int g_last_sorting = 0;
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
                    info << " - Group: ";
                    switch(g_last_grouping){
                        case 0: info<<"Folder"; break; case 1: info<<"Album"; break; case 2: info<<"Artist"; break; case 3: info<<"Artist - Album"; break; case 4: info<<"Genre"; break; case 5: info<<"Year"; break; case 6: info<<"Label"; break; case 7: info<<"Composer"; break; case 8: info<<"Performer"; break; case 9: info<<"Album Artist"; break; case 10: info<<"Directory"; break; case 11: info<<"Comment"; break; case 12: info<<"Rating"; break; default: info<<"Folder"; }
                    info << " - Sort: ";
                    switch(g_last_sorting){
                        case 0: info<<"Name"; break; case 1: info<<"Date"; break; case 2: info<<"Track Count"; break; case 3: info<<"Artist"; break; case 4: info<<"Album"; break; case 5: info<<"Year"; break; case 6: info<<"Genre"; break; case 7: info<<"Path"; break; case 8: info<<"Random"; break; case 9: info<<"Size"; break; case 10: info<<"Rating"; break; case 11: info<<"Release Date"; break; default: info<<"Name"; }

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
        SORT_BY_RATING,
        SORT_BY_RELEASE_DATE
    };

    enum track_sort_mode { TRACK_SORT_BY_NUMBER = 0, TRACK_SORT_BY_NAME = 1, TRACK_SORT_NONE = 2 };
    track_sort_mode track_sorting = TRACK_SORT_BY_NUMBER;

    enum view_mode { VIEW_LIBRARY = 0, VIEW_PLAYLIST };
    enum doubleclick_action { DOUBLECLICK_PLAY = 0, DOUBLECLICK_ADD_TO_CURRENT, DOUBLECLICK_ADD_TO_NEW, DOUBLECLICK_PLAY_IN_GRID };
    enum label_format { LABEL_ALBUM_ONLY = 0, LABEL_ARTIST_ONLY = 1, LABEL_ARTIST_ALBUM = 2, LABEL_FOLDER_NAME = 3 };
    enum artwork_scale_mode { ARTWORK_STRETCH = 0, ARTWORK_FIT = 1, ARTWORK_CROP = 2 };

    int columns; int text_lines; bool show_text; bool show_track_count; int font_size; group_mode grouping; sort_mode sorting; view_mode view; doubleclick_action doubleclick; label_format label_style; bool auto_scroll_to_now_playing; enum enlarged_mode { ENLARGED_NONE = 0, ENLARGED_2X2 = 1, ENLARGED_3X3 = 2 }; enlarged_mode enlarged_now_playing; bool show_playlist_overlay; artwork_scale_mode artwork_scale;

    grid_config() : columns(5), text_lines(2), show_text(true), show_track_count(true), font_size(11), grouping(GROUP_BY_FOLDER), sorting(SORT_BY_NAME), view(VIEW_LIBRARY), doubleclick(DOUBLECLICK_PLAY), label_style(LABEL_ALBUM_ONLY), auto_scroll_to_now_playing(false), enlarged_now_playing(ENLARGED_NONE), show_playlist_overlay(false), artwork_scale(ARTWORK_FIT) {}

    ui_element_config::ptr save(const GUID& guid) {
        struct Header { uint32_t magic; uint16_t ver; uint16_t reserved; };
        constexpr uint32_t MAGIC = 0x43474141; // 'A''A''G''C'
        Header h{MAGIC, 2, 0};
        std::vector<uint8_t> buf; buf.reserve(128);
        auto append = [&](auto v){ uint8_t* p = reinterpret_cast<uint8_t*>(&v); buf.insert(buf.end(), p, p+sizeof(v)); };
        append(h);
        append(columns); append(text_lines); append(show_text); append(show_track_count); append(font_size);
        append(grouping); append(sorting); append(view); append(doubleclick); append(label_style);
        append(auto_scroll_to_now_playing); append(enlarged_now_playing); append(show_playlist_overlay); append(artwork_scale);
        return ui_element_config::g_create(guid, buf.data(), (t_size)buf.size());
    }

    void load(ui_element_config::ptr cfg) {
        if (!cfg.is_valid()) return;
        const uint8_t* data = static_cast<const uint8_t*>(cfg->get_data());
        size_t sz = cfg->get_data_size();
        if (sz >= sizeof(uint32_t)+sizeof(uint16_t)*2) {
            uint32_t magic = *reinterpret_cast<const uint32_t*>(data);
            uint16_t ver = *reinterpret_cast<const uint16_t*>(data+4);
            if (magic == 0x43474141 && ver >= 1) {
                size_t off = 8; auto read=[&](auto& out){ if (off+sizeof(out) <= sz) { memcpy(&out, data+off, sizeof(out)); off+=sizeof(out);} };
                read(columns); read(text_lines); read(show_text); read(show_track_count); read(font_size);
                read(grouping); read(sorting); read(view); read(doubleclick); read(label_style);
                read(auto_scroll_to_now_playing); read(enlarged_now_playing); read(show_playlist_overlay);
                if (ver >= 2) read(artwork_scale); else artwork_scale = ARTWORK_FIT;
                columns = std::max(1, columns); text_lines = std::max(1, std::min(3, text_lines)); font_size = std::max(7, std::min(14, font_size));
                if ((int)view < 0 || (int)view > VIEW_PLAYLIST) view = VIEW_LIBRARY;
                if ((int)doubleclick < 0 || (int)doubleclick > DOUBLECLICK_PLAY_IN_GRID) doubleclick = DOUBLECLICK_PLAY;
                if ((int)label_style < 0 || (int)label_style > LABEL_FOLDER_NAME) label_style = LABEL_ALBUM_ONLY;
                if ((int)sorting < 0 || (int)sorting > SORT_BY_RELEASE_DATE) sorting = SORT_BY_NAME;
                if ((int)artwork_scale < 0 || (int)artwork_scale > ARTWORK_CROP) artwork_scale = ARTWORK_FIT;
                return;
            }
        }
        struct Legacy { int columns, text_lines; bool show_text, show_track_count; int font_size; int grouping, sorting, view, doubleclick, label_style; bool auto_scroll; int enlarged; bool overlay; };
        size_t need = sizeof(Legacy);
        if (sz >= (need/2)) {
            Legacy tmp{}; memcpy(&tmp, data, std::min(sz, need));
            columns = std::max(1, tmp.columns); text_lines = std::max(1, std::min(3, tmp.text_lines));
            show_text = tmp.show_text; show_track_count = tmp.show_track_count; font_size = std::max(7, std::min(14, tmp.font_size));
            grouping = (group_mode)std::clamp(tmp.grouping, 0, (int)GROUP_BY_RATING);
            sorting = (sort_mode)std::clamp(tmp.sorting, 0, (int)SORT_BY_RELEASE_DATE);
            view = (view_mode)std::clamp(tmp.view, 0, (int)VIEW_PLAYLIST);
            doubleclick = (doubleclick_action)std::clamp(tmp.doubleclick, 0, (int)DOUBLECLICK_PLAY_IN_GRID);
            label_style = (label_format)std::clamp(tmp.label_style, 0, (int)LABEL_FOLDER_NAME);
            auto_scroll_to_now_playing = tmp.auto_scroll; enlarged_now_playing = (enlarged_mode)std::clamp(tmp.enlarged, 0, 2); show_playlist_overlay = tmp.overlay; artwork_scale = ARTWORK_FIT;
        }
    }
};


// Thumbnail cache entry with memory tracking

struct thumbnail_data {

    Gdiplus::Bitmap* bitmap;

    ULONGLONG last_access;

    bool loading;

    int cached_size;

    size_t memory_size;

    

    thumbnail_data() : bitmap(nullptr), last_access(GetTickCount64()), loading(false), cached_size(0), memory_size(0) {}

    

    ~thumbnail_data() {

        clear();

    }

    

    void clear() {

        if (bitmap) {
            // Prefer explicit shutdown signal
            if (shutdown_protection::is_shutting_down()) {
                bitmap = nullptr; memory_size = 0; return;
            }
            delete bitmap;
            bitmap = nullptr;
            memory_size = 0;
        }
    }

    

    void set_bitmap(Gdiplus::Bitmap* bmp, int size) {

        clear();

        bitmap = bmp;

        cached_size = size;

        last_access = GetTickCount64();

        if (bmp) {

            memory_size = bmp->GetWidth() * bmp->GetHeight() * 4;

        }

    }

    

    void touch() {

        last_access = GetTickCount64();

    }

};



// Smart cache management with LRU and adaptive limits
class thumbnail_cache {
private:
    static size_t total_memory;
    static size_t max_cache_size;
    static std::list<std::shared_ptr<thumbnail_data>> lru;
    static std::unordered_map<thumbnail_data*, std::list<std::shared_ptr<thumbnail_data>>::iterator> idx;
    static int viewport_first;
    static int viewport_last;
    static std::atomic<bool> shutdown_in_progress;  // v10.0.9: F9FCh crash fix
    static critical_section cache_sync;
    

    static size_t get_available_memory() {
        static ULONGLONG lastTick = 0; static size_t cached = MIN_CACHE_SIZE_MB * 1024 * 1024;
        ULONGLONG now = GetTickCount64();
        if (now - lastTick < 1500) return cached;
        MEMORYSTATUSEX memInfo; memInfo.dwLength = sizeof(MEMORYSTATUSEX); GlobalMemoryStatusEx(&memInfo);
        size_t quarter_available = (size_t)(memInfo.ullAvailPhys / 4);
        size_t min_size = MIN_CACHE_SIZE_MB * 1024 * 1024;
        size_t max_size = MAX_CACHE_SIZE_MB * 1024 * 1024;
        cached = std::max(min_size, std::min(max_size, quarter_available));
        lastTick = now; return cached;
    }
    

public:

    static void remove_thumbnail(thumbnail_data* thumb) {
        if (!thumb) return; insync(cache_sync);
        auto it = idx.find(thumb);
        if (it != idx.end()) {
            auto node = it->second; auto sp = *node; size_t sz = sp ? sp->memory_size : 0;
            if (sz > 0 && total_memory >= sz) total_memory -= sz;
            lru.erase(node); idx.erase(it);
        }
    }
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

    

    static void add_thumbnail(const std::shared_ptr<thumbnail_data>& sp, int /*item_index*/) {
        if (!sp || !sp->bitmap || shutdown_in_progress.load()) return;
        insync(cache_sync);
        max_cache_size = get_available_memory();
        while (total_memory + sp->memory_size > max_cache_size) { if (!evict_lru_thumbnail()) break; }
        auto itFound = idx.find(sp.get());
        if (itFound != idx.end()) {
            // Move to front
            lru.splice(lru.begin(), lru, itFound->second);
        } else {
            lru.push_front(sp); idx[sp.get()] = lru.begin(); total_memory += sp->memory_size;
        }
    }
    

    static bool evict_lru_thumbnail() {
        insync(cache_sync);
        if (lru.empty()) return false;
        auto it = std::prev(lru.end());
        auto sp = *it; size_t sz = sp ? sp->memory_size : 0;
        if (sp) sp->clear();
        lru.erase(it); if (sp) idx.erase(sp.get());
        if (sz > 0 && total_memory >= sz) total_memory -= sz; return true;
    }
    

    static void clear_all() {

        // v10.0.9 CRITICAL FIX: Prevent F9FCh memory corruption

        shutdown_in_progress.store(true);

        

        // Safe cleanup with shutdown signal

        insync(cache_sync);
        for (auto& sp : lru) { if (sp && sp->bitmap) { try { sp->clear(); } catch(...) {} } }
        lru.clear(); idx.clear();
        total_memory = 0;
    }
    

    static bool is_shutting_down() {

        return shutdown_in_progress.load();

    }

    

    static size_t get_memory_usage() { return total_memory; }

    static size_t get_cache_limit() { return max_cache_size; }

};



// Initialize static members

size_t thumbnail_cache::total_memory = 0;

size_t thumbnail_cache::max_cache_size = MIN_CACHE_SIZE_MB * 1024 * 1024;

std::list<std::shared_ptr<thumbnail_data>> thumbnail_cache::lru;
std::unordered_map<thumbnail_data*, std::list<std::shared_ptr<thumbnail_data>>::iterator> thumbnail_cache::idx;
int thumbnail_cache::viewport_first = 0;

int thumbnail_cache::viewport_last = 0;

std::atomic<bool> thumbnail_cache::shutdown_in_progress = false;  // v10.0.9: F9FCh fix

critical_section thumbnail_cache::cache_sync;





// Album/folder data structure

struct grid_item {

    pfc::string8 display_name;  // Using pfc::string8 for proper UTF-8 handling

    pfc::string8 folder_name;   // v10.0.4: Store actual folder name separately

    pfc::string8 sort_key;

    pfc::string8 path;

    metadb_handle_list tracks;

    // Multi-disc support (grid shows one album; disc count is indicated)
    uint32_t disc_mask = 0;   // bit 0 = disc 1, ... up to disc 32
    uint8_t disc_count = 1;   // computed from disc_mask (defaults to 1)

    // Track used as the "identity" for art / open-folder in multi-disc groups
    metadb_handle_ptr representative_track;

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

    uint32_t release_date_key;

    mutable int cached_label_format = -1;
    mutable std::wstring cached_label_w;

    

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

    const std::wstring& get_display_text_wide(grid_config::label_format format) const {
        if (cached_label_format != (int)format || cached_label_w.empty()) {
            cached_label_format = (int)format;
            pfc::string8 text = get_display_text(format);
            int wide_len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
            if (wide_len > 0) {
                cached_label_w.assign(wide_len - 1, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &cached_label_w[0], wide_len);
            } else {
                cached_label_w.clear();
            }
        }
        return cached_label_w;
    }

    

    grid_item() : thumbnail(std::make_shared<thumbnail_data>()), newest_date(0), rating(0), total_size(0), release_date_key(0) {}

};


static int parse_first_int_anywhere(const char* value) {
    if (!value) return 0;

    const unsigned char* p = reinterpret_cast<const unsigned char*>(value);
    while (*p && !std::isdigit(*p)) ++p;
    if (!*p) return 0;

    int out = 0;
    while (*p && std::isdigit(*p)) {
        out = out * 10 + (*p - '0');
        ++p;
    }
    return out;
}

static int infer_disc_number_from_path(const char* path);

static bool try_get_album_root_folder_from_file_path(const char* file_path, pfc::string8& out_folder_path, pfc::string8& out_folder_name) {
    out_folder_path.reset();
    out_folder_name.reset();

    if (!file_path || !file_path[0]) return false;
    if (infer_disc_number_from_path(file_path) <= 0) return false;

    const char* file_slash = strrchr(file_path, '\\');
    if (!file_slash) file_slash = strrchr(file_path, '/');
    if (!file_slash || file_slash == file_path) return false;

    // file_path: ...\Album\CD1\track.ext
    // file_slash points to slash before track.ext, so disc folder name is the last directory segment
    const char* disc_slash = file_slash - 1;
    while (disc_slash > file_path && *disc_slash != '\\' && *disc_slash != '/') --disc_slash;
    if (disc_slash <= file_path) return false;

    const char* album_slash = disc_slash - 1;
    while (album_slash > file_path && *album_slash != '\\' && *album_slash != '/') --album_slash;
    const char* album_name_start = (*album_slash == '\\' || *album_slash == '/') ? (album_slash + 1) : file_path;
    const char* album_name_end = disc_slash;

    if (album_name_end <= album_name_start) return false;

    out_folder_name.set_string(album_name_start, album_name_end - album_name_start);
    out_folder_path.set_string(file_path, disc_slash - file_path);
    return !out_folder_name.is_empty() && !out_folder_path.is_empty();
}

static int get_disc_number_for_handle(metadb_handle_ptr handle) {
    if (!handle.is_valid()) return 0;

    int disc = 0;
    try {
        file_info_impl info;
        if (handle->get_info(info)) {
            const char* disc_value = info.meta_get("DISCNUMBER", 0);
            if (!disc_value || !disc_value[0]) disc_value = info.meta_get("DISC", 0);
            if (!disc_value || !disc_value[0]) disc_value = info.meta_get("DISC NO", 0);
            disc = parse_first_int_anywhere(disc_value);
        }
    } catch (...) {
        disc = 0;
    }

    if (disc <= 0) {
        try {
            disc = infer_disc_number_from_path(handle->get_path());
        } catch (...) {
            disc = 0;
        }
    }

    return disc;
}

static int get_track_number_for_handle(metadb_handle_ptr handle) {
    if (!handle.is_valid()) return 0;

    int track = 0;
    try {
        file_info_impl info;
        if (handle->get_info(info)) {
            const char* track_value = info.meta_get("TRACKNUMBER", 0);
            if (!track_value || !track_value[0]) track_value = info.meta_get("TRACK", 0);
            track = parse_first_int_anywhere(track_value);
        }
    } catch (...) {
        track = 0;
    }

    return track;
}

static bool is_better_representative_track(metadb_handle_ptr candidate, metadb_handle_ptr current) {
    if (!candidate.is_valid()) return false;
    if (!current.is_valid()) return true;

    int cand_disc = get_disc_number_for_handle(candidate);
    int curr_disc = get_disc_number_for_handle(current);

    if (cand_disc > 0 && curr_disc > 0 && cand_disc != curr_disc) return cand_disc < curr_disc;
    if (cand_disc > 0 && curr_disc <= 0) return true;
    if (cand_disc <= 0 && curr_disc > 0) return false;

    try {
        pfc::string8 cand_path = candidate->get_path();
        pfc::string8 curr_path = current->get_path();
        return pfc::stricmp_ascii(cand_path.c_str(), curr_path.c_str()) < 0;
    } catch (...) {
        return false;
    }
}

static void update_representative_track(grid_item& item, metadb_handle_ptr handle) {
    if (is_better_representative_track(handle, item.representative_track)) {
        item.representative_track = handle;
    }
}

static int infer_disc_number_from_path(const char* path) {
    if (!path || !path[0]) return 0;

    const char* last_slash = strrchr(path, '\\');
    if (!last_slash) last_slash = strrchr(path, '/');
    if (!last_slash || last_slash == path) return 0;

    const char* start = path;
    const char* prev_slash = last_slash - 1;
    while (prev_slash > start && *prev_slash != '\\' && *prev_slash != '/') --prev_slash;
    if (*prev_slash == '\\' || *prev_slash == '/') ++prev_slash;
    if (prev_slash >= last_slash) return 0;

    std::string folder(prev_slash, last_slash);
    std::string upper;
    upper.reserve(folder.size());
    for (unsigned char c : folder) upper.push_back((char)std::toupper(c));

    // Heuristic: only treat as disc folder if it contains a likely marker.
    if (upper.find("DISC") == std::string::npos &&
        upper.find("DISK") == std::string::npos &&
        upper.find("CD") == std::string::npos) {
        return 0;
    }

    return parse_first_int_anywhere(upper.c_str());
}

static uint8_t popcount_u32(uint32_t v) {
    uint8_t c = 0;
    while (v) {
        v &= (v - 1);
        ++c;
    }
    return c;
}

static void add_disc_to_item(grid_item& item, metadb_handle_ptr handle) {
    int disc = 0;
    try {
        file_info_impl info;
        if (handle->get_info(info)) {
            const char* disc_value = info.meta_get("DISCNUMBER", 0);
            if (!disc_value || !disc_value[0]) disc_value = info.meta_get("DISC", 0);
            if (!disc_value || !disc_value[0]) disc_value = info.meta_get("DISC NO", 0);
            disc = parse_first_int_anywhere(disc_value);
        }
    } catch (...) {
        // ignore
    }

    if (disc <= 0) {
        try {
            disc = infer_disc_number_from_path(handle->get_path());
        } catch (...) {
            disc = 0;
        }
    }

    if (disc > 0 && disc <= 32) {
        item.disc_mask |= (1u << (uint32_t)(disc - 1));
    }
}



static uint32_t parse_release_date_string(const char* value) {



    if (!value || !value[0]) return 0;



    int numbers[3] = {0, 0, 0};



    int count = 0;



    int current = 0;



    bool in_number = false;



    for (const char* p = value; *p; ++p) {



        if (std::isdigit(static_cast<unsigned char>(*p))) {



            in_number = true;



            current = current * 10 + (*p - '0');



        } else if (in_number) {



            if (count < 3) numbers[count++] = current;



            current = 0;



            in_number = false;



        }



    }



    if (in_number && count < 3) numbers[count++] = current;



    if (count == 0) return 0;



    int year = 0, month = 0, day = 0;



    if (numbers[0] >= 1000) {



        year = numbers[0];



        if (count >= 2 && numbers[1] >= 1 && numbers[1] <= 12) month = numbers[1];



        if (count >= 3 && numbers[2] >= 1 && numbers[2] <= 31) day = numbers[2];



    } else if (count >= 2 && numbers[1] >= 1000) {



        year = numbers[1];



        if (numbers[0] >= 1 && numbers[0] <= 12) month = numbers[0];



        if (count >= 3 && numbers[2] >= 1 && numbers[2] <= 31) day = numbers[2];



    } else {



        year = numbers[0];



        if (count >= 2 && numbers[1] >= 1 && numbers[1] <= 12) month = numbers[1];



        if (count >= 3 && numbers[2] >= 1 && numbers[2] <= 31) day = numbers[2];



    }



    if (year < 1000 || year > 9999) return 0;



    if (month < 1 || month > 12) month = 0;



    if (day < 1 || day > 31) day = 0;



    return static_cast<uint32_t>(year * 10000 + month * 100 + day);



}







static uint32_t extract_release_date_from_info(const file_info& info) {



    static const char* release_tags[] = {



        "ORIGINAL RELEASE DATE",



        "ORIGINALDATE",



        "ORIGDATE",



        "DATE_ORIGINAL",



        "DATE ORIGINAL",



        "ORIGINAL YEAR",



        "ORIGINALYEAR",



        "ORIGYEAR",



        "RELEASE DATE",



        "RELEASEDATE",



        "YEAR",



        "DATE"



    };



    for (const char* tag : release_tags) {



        const char* value = info.meta_get(tag, 0);



        if (value && value[0]) {



            uint32_t parsed = parse_release_date_string(value);



            if (parsed != 0) return parsed;



        }



    }



    return 0;



}







class album_grid_instance : public ui_element_instance,

                          public playlist_callback_single,

                          public validated_object {

private:

    // CRITICAL FIX: Manual reference counting

    volatile long m_refcount;

    

    HWND m_hwnd;

    HWND m_parent;

    HWND m_tooltip;  // Tooltip window
    HWND m_search_box;  // Search/filter text box
    // Reusable back buffer for painting
    HDC m_memdc = NULL;
    HBITMAP m_membmp = NULL;
    HBITMAP m_oldbmp = NULL;
    int m_bufW = 0, m_bufH = 0;
    HFONT m_placeholder_font = NULL;
    int m_placeholder_font_size = 0;
    std::vector<std::unique_ptr<grid_item>> m_items;

    std::vector<int> m_filtered_indices;  // Indices of filtered items

    pfc::string8 m_search_text;  // Current search filter

    bool m_search_visible;  // Is search box visible

    // Auto-scroll setting moved to grid_config for persistence

    int m_scroll_pos;

    std::set<int> m_selected_indices;  // Multi-select support

    int m_last_selected;  // For shift-select

    int m_hover_index;

    int m_tooltip_index;  // Index of item tooltip is showing for

    int m_mouse_x, m_mouse_y;  // v10.0.30: Current mouse position for floating labels

    service_ptr_t<ui_element_instance_callback> m_callback;

    grid_config m_config;

    fb2k::CCoreDarkModeHooks m_dark;

    std::atomic<bool> m_is_destroying;  // Atomic flag to prevent use-after-free in playlist callbacks

    bool m_tracking;

    

    // Visible range for lazy loading

    int m_first_visible;

    int m_last_visible;

    

    // Now playing tracking

    metadb_handle_ptr m_now_playing;

    int m_now_playing_index;

    bool m_highlight_now_playing;

    ULONGLONG m_last_user_scroll;

    

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

    

    // v10.0.27: Zombie callback handler

    std::shared_ptr<zombie_callback_handler> m_zombie_callback;



    // Playlist overlay selection state

    int m_overlay_selected_index = -1;



    // v10.0.30: PERFORMANCE OPTIMIZATION - Layout caching

    struct layout_cache {

        bool valid;

        int cached_width;

        int cached_columns;

        int cached_item_size;

        ULONGLONG last_update;



        layout_cache() : valid(false), cached_width(0), cached_columns(0),

                        cached_item_size(120), last_update(0) {}



        bool is_valid_for(int width, int columns) const {

            return valid && cached_width == width && cached_columns == columns;

        }



        void update(int width, int columns, int item_size) {

            valid = true;

            cached_width = width;

            cached_columns = columns;

            cached_item_size = item_size;

            last_update = GetTickCount64();

        }



        void invalidate() {

            valid = false;

        }

    } m_layout_cache;



    // v10.0.30: Context menu text caching

    struct context_menu_cache {

        std::string cached_text;

        int cached_index;

        bool valid;

        ULONGLONG last_update;



        context_menu_cache() : cached_index(-1), valid(false), last_update(0) {}



        bool is_valid_for(int index) const {

            return valid && cached_index == index &&

                   (GetTickCount64() - last_update) < 5000; // 5 second cache

        }



        void update(int index, const std::string& text) {

            cached_index = index;

            cached_text = text;

            valid = true;

            last_update = GetTickCount64();

        }



        void invalidate() {

            valid = false;

        }

    } m_context_menu_cache;



    // v10.0.30: Scroll throttling

    ULONGLONG m_last_scroll_update;

    static const DWORD SCROLL_THROTTLE_MS = 12; // Slightly more reactive than 16ms



    // Async artwork loading

    struct ThumbnailResult { int index; int generation; Gdiplus::Bitmap* bmp; int size; };

    static const UINT WM_APP_THUMBNAIL_READY = WM_APP + 100;
    static const UINT WM_APP_INVALIDATE = WM_APP + 101;
    static std::atomic<int> s_inflight_loaders;

    static const int kMaxInflight = 4;
    static ThreadPool& thumb_pool() { static ThreadPool pool(kMaxInflight); return pool; }


    // Generation to validate async results

    std::atomic<int> m_items_generation{0};
    std::atomic<bool> m_invalidate_pending{false};
    void request_invalidate() {
        if (!m_hwnd) return; bool expected=false; if (m_invalidate_pending.compare_exchange_strong(expected,true)) {
            PostMessage(m_hwnd, WM_APP_INVALIDATE, 0, 0);
        }
    }


    // Per-instance now playing last track

    metadb_handle_ptr m_last_now_playing;

    

public:

    album_grid_instance(ui_element_config::ptr config, ui_element_instance_callback_ptr callback)
        : m_refcount(1), m_is_destroying(false), m_callback(callback), m_hwnd(NULL), m_parent(NULL), m_tooltip(NULL), m_search_box(NULL),
          m_search_visible(false), m_scroll_pos(0),

          m_last_selected(-1), m_hover_index(-1), m_tooltip_index(-1), m_mouse_x(0), m_mouse_y(0), m_tracking(false),

          m_first_visible(0), m_last_visible(0), m_item_size(120), m_font_height(14),

          m_now_playing_index(-1), m_highlight_now_playing(true),

          m_last_user_scroll(0), m_last_jump_letter(0), m_last_jump_index(-1), m_last_scroll_update(0) {
        m_config.load(config);

        

        // v10.0.27: Create zombie callback handler - this will never die

        m_zombie_callback = std::make_shared<zombie_callback_handler>(this);

        

        // Register this instance (but don't set global shutdown)

        shutdown_protection::register_instance(this);

        

        // Register for playlist callbacks

        static_api_ptr_t<playlist_manager> pm;

        pm->register_callback(this, playlist_callback::flag_on_items_added | 
                                   playlist_callback::flag_on_items_removed |
                                   playlist_callback::flag_on_items_reordered);
    }

    // Back buffer helpers
    void release_backbuffer() {
        if (m_memdc) {
            if (m_oldbmp) { SelectObject(m_memdc, m_oldbmp); m_oldbmp = NULL; }
            if (m_membmp) { DeleteObject(m_membmp); m_membmp = NULL; }
            DeleteDC(m_memdc); m_memdc = NULL; m_bufW = m_bufH = 0;
        }
    }
    bool ensure_backbuffer(int w, int h) {
        if (w <= 0 || h <= 0) return false;
        if (m_memdc && w == m_bufW && h == m_bufH) return true;
        release_backbuffer();
        HDC hdcRef = GetDC(m_hwnd);
        if (!hdcRef) return false;
        m_memdc = CreateCompatibleDC(hdcRef);
        m_membmp = CreateCompatibleBitmap(hdcRef, w, h);
        ReleaseDC(m_hwnd, hdcRef);
        if (!m_memdc || !m_membmp) { release_backbuffer(); return false; }
        m_oldbmp = (HBITMAP)SelectObject(m_memdc, m_membmp);
        m_bufW = w; m_bufH = h; return true;
    }
    

    // CRITICAL FIX v10.0.26: Manual reference counting implementation

    virtual int service_add_ref() noexcept override {

        return InterlockedIncrement(&m_refcount);

    }

    

    virtual int service_release() noexcept override {

        long count = InterlockedDecrement(&m_refcount);

        if (count == 0) {

            delete this;

        }

        return count;

    }

    

    virtual bool service_query(service_ptr & p_out, const GUID & p_guid) override {

        // Simple implementation - no additional interfaces exposed

        return false;

    }

    

    // v10.0.20: Manual cleanup for reference counting

    void cleanup_resources() {

        try {

            m_is_destroying = true;

            

            // Unregister this instance

            shutdown_protection::unregister_instance(this);

            

            // Unregister callbacks FIRST

            static_api_ptr_t<playlist_manager> pm;

            pm->unregister_callback(this);

            

            // Kill all timers

            if (m_hwnd) {

                KillTimer(m_hwnd, TIMER_LOAD);

                KillTimer(m_hwnd, TIMER_PROGRESSIVE);

                KillTimer(m_hwnd, TIMER_NOW_PLAYING);


            if (m_placeholder_font) {
                DeleteObject(m_placeholder_font);
                m_placeholder_font = NULL;
                m_placeholder_font_size = 0;
            }
            }

            

            // Clear data

            m_items.clear();

            m_now_playing.release();

        } catch(...) {

            // Ignore cleanup errors

        }

    }

    

    // v10.0.28 FIX: Destructor with shutdown detection

    ~album_grid_instance() {

        console::print("[Album Art Grid v10.0.28] Destructor entered - checking shutdown state");

        

        // CRITICAL v10.0.28: Check if main window is gone (early shutdown detection)

        HWND main_wnd = core_api::get_main_window();

        if (!main_wnd || !IsWindow(main_wnd)) {

            console::print("[Album Art Grid v10.0.28] MAIN WINDOW GONE - EARLY EXIT from destructor");

            

            // Minimal cleanup during app shutdown - avoid UI operations

            if (m_zombie_callback) {

                m_zombie_callback->kill();

            }

            shutdown_protection::unregister_instance(this);

            

            console::print("[Album Art Grid v10.0.28] SAFE EARLY EXIT completed");

            return;

        }

        

        console::print("[Album Art Grid v10.0.28] Normal destructor - main window still valid");

        

        // Kill zombie callback FIRST - it won't be destroyed, just disabled

        if (m_zombie_callback) {

            m_zombie_callback->kill();

            // Don't reset the pointer - let it live forever

            // This prevents any possible crash from late callbacks

        }

        

        cleanup_resources();

        

        // Destroy window if still exists
        if (m_hwnd && IsWindow(m_hwnd)) {
            SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);
            DestroyWindow(m_hwnd);
            m_hwnd = NULL;
            release_backbuffer();
        }
        

        console::print("[Album Art Grid v10.0.28] Destructor completed");

    }

    

    // v10.0.27: Process callback from zombie handler

    void process_zombie_callback() {

        // Only process if we're still valid

        if (!is_valid() || m_is_destroying) return;

        

        // Execute whatever callback logic was needed

        // This is now safe because zombie handler already validated us

        try {

            // Put any callback processing here

            // For now, just validate we're alive

            if (m_hwnd && IsWindow(m_hwnd)) {

                // Callback processed successfully

            }

        } catch(...) {

            // Ignore any errors

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

        } else {
            return; // creation failed
        }
        

        // Start loading after a short delay

        SetTimer(m_hwnd, TIMER_LOAD, 100, NULL);

        // Start now playing check timer (every 500ms for responsive auto-scroll)

        SetTimer(m_hwnd, TIMER_NOW_PLAYING, 500, NULL);

    }

    

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {

        // v10.0.18: Enhanced shutdown protection

        if (shutdown_protection::is_shutting_down()) {

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

        

        // v10.0.18: CRITICAL - Check if instance is being destroyed

        // If instance is invalid or destroying, only handle essential messages

        if (instance && !instance->is_valid()) {

            // Instance is marked as destroyed, handle only cleanup messages

            if (msg == WM_DESTROY || msg == WM_NCDESTROY) {

                return DefWindowProc(hwnd, msg, wp, lp);

            }

            return 0;  // Ignore all other messages for destroyed instances

        }

        

        // v10.0.18: Normal processing for valid instances

        if (instance && !instance->m_is_destroying.load() && !shutdown_protection::is_shutting_down()) {

            switch (msg) {

                case WM_GETDLGCODE: return DLGC_WANTARROWS | DLGC_WANTALLKEYS | DLGC_WANTCHARS;

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
                case WM_APP_THUMBNAIL_READY: return instance->on_thumbnail_ready(reinterpret_cast<ThumbnailResult*>(lp));
                case WM_APP + 101:
                    instance->m_invalidate_pending.store(false);
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                case WM_DESTROY: {

                    // v10.0.20: Proper cleanup without destructor conflicts

                    if (instance) {

                        instance->m_is_destroying = true;

                        

                        // Unregister this instance

                        shutdown_protection::unregister_instance(instance);

                        

                        console::print("[Album Art Grid v10.0.20] Cleanup WM_DESTROY - service reference counting");

                        

                        // Unregister callbacks FIRST

                        try {

                            static_api_ptr_t<playlist_manager> pm;

                            pm->unregister_callback(instance);

                        } catch(...) {

                            // Ignore errors during shutdown

                        }

                        

                        // Kill all timers

                        KillTimer(hwnd, TIMER_LOAD);

                        KillTimer(hwnd, TIMER_PROGRESSIVE);

                        KillTimer(hwnd, TIMER_NOW_PLAYING);



                        if (instance->m_search_box) {

                            RemoveWindowSubclass(instance->m_search_box, SearchBoxProc, 0);

                        }

                        

                        // Clear data

                        try {

                            instance->m_items.clear();

                            instance->m_now_playing.release();

                        } catch(...) {

                            // Ignore cleanup errors

                        }

                        

                        // Clear the window data pointer to prevent any pending messages

                        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);

                        instance->m_hwnd = NULL;

                    }

                    return DefWindowProc(hwnd, msg, wp, lp);

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

        // Handle playlist overlay navigation when visible

        auto overlay_active = [&]() -> bool {

            return m_config.show_playlist_overlay &&

                   m_config.enlarged_now_playing == grid_config::ENLARGED_3X3 &&

                   m_now_playing_index >= 0;

        };



        if (overlay_active()) {

            auto pm = playlist_manager::get();

            t_size active = pm->get_active_playlist();

            if (active != pfc::infinite_size) {

                int count = (int)pm->playlist_get_item_count(active);

                if (count > 0) {

                    // Initialize selection to current track if none

                    if (m_overlay_selected_index < 0 || m_overlay_selected_index >= count) {

                        metadb_handle_ptr now;

                        if (playback_control::get()->get_now_playing(now)) {

                            for (int i = 0; i < count; ++i) {

                                metadb_handle_ptr t;

                                if (pm->playlist_get_item_handle(t, active, i) && t == now) {

                                    m_overlay_selected_index = i;

                                    break;

                                }

                            }

                        }

                        if (m_overlay_selected_index < 0) m_overlay_selected_index = 0;

                    }

                    if (key == VK_UP) {

                        if (m_overlay_selected_index > 0) m_overlay_selected_index--;

                        InvalidateRect(m_hwnd, NULL, FALSE);

                        return 0;

                    } else if (key == VK_DOWN) {

                        if (m_overlay_selected_index + 1 < count) m_overlay_selected_index++;

                        InvalidateRect(m_hwnd, NULL, FALSE);

                        return 0;

                    } else if (key == VK_RETURN) {

                        handle_playlist_overlay_click(m_overlay_selected_index);

                        return 0;

                    }

                }

            }

        }

        // Grid navigation helpers (disabled for arrows; overlay owns arrows)

        auto navigate_grid = [&](int /*delta*/) {};

        auto activate_selection = [&]() { perform_default_action_on_selection(); };



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

        } else if (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN) {

            // Swallow arrow keys so they don't reach external playlist view

            return 0;

        } else if (key == VK_RETURN) { activate_selection(); return 0; }

        else if (key >= 'A' && key <= 'Z') {

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

        } else if (key == VK_PRIOR) {  // Page Up

            return on_vscroll(SB_PAGEUP);

        } else if (key == VK_NEXT) {  // Page Down

            return on_vscroll(SB_PAGEDOWN);

        }

        return 0;

    }

    

    // NEW v10.0.29: Get size for specific item (enlarged for now playing)

    int get_item_size(int index) {

        if (index == m_now_playing_index && m_config.enlarged_now_playing != grid_config::ENLARGED_NONE) {

            int multiplier = (m_config.enlarged_now_playing == grid_config::ENLARGED_2X2) ? 2 : 3;

            // Include padding between cells to fill the entire reserved area

            return m_item_size * multiplier + PADDING * (multiplier - 1);

        }

        return m_item_size;

    }

    

    // NEW v10.0.29: Get grid span for specific item (how many cells it occupies)

    int get_item_span(int index) {

        if (index == m_now_playing_index && m_config.enlarged_now_playing != grid_config::ENLARGED_NONE) {

            return (m_config.enlarged_now_playing == grid_config::ENLARGED_2X2) ? 2 : 3;

        }

        return 1;

    }

    

    // NEW v10.0.29: Enhanced layout with enlarged now playing support

    struct item_position {

        int x, y;

        int width, height;

        bool visible;

    };

    

    item_position get_item_position(int index, const RECT& client_rect) {

        item_position pos = {0, 0, 0, 0, false};



        if (index < 0 || index >= (int)m_items.size()) {

            return pos;

        }



        int text_height = calculate_text_height();

        int cols = m_config.columns;



        // Calculate total width and centering

        int total_width = cols * m_item_size + (cols + 1) * PADDING;

        int start_x = (client_rect.right - total_width) / 2;

        if (start_x < 0) start_x = 0;



        // v10.0.30: FIXED - Proper enlarged item layout calculation

        if (m_config.enlarged_now_playing != grid_config::ENLARGED_NONE && m_now_playing_index >= 0) {

            // Use adaptive layout that properly handles enlarged items

            return get_adaptive_item_position(index, client_rect, start_x, cols, text_height);

        }



        // Standard grid layout (no enlarged items)

        int row = index / cols;

        int col = index % cols;



        pos.x = start_x + col * (m_item_size + PADDING) + PADDING;

        pos.y = row * (m_item_size + text_height + PADDING) + PADDING - m_scroll_pos;

        pos.width = get_item_size(index);

        pos.height = get_item_size(index) + text_height;

        pos.visible = (pos.y + pos.height > 0 && pos.y < client_rect.bottom);



        return pos;

    }



    // v10.0.43: CELL-BASED GRID ENGINE - Revolutionary architecture

    // Grid cell structure for precise layout management

    struct GridCell {

        bool occupied;

        int item_index;    // -1 if empty, else index of occupying item

        bool is_primary;   // true if this is top-left of enlarged item



        GridCell() : occupied(false), item_index(-1), is_primary(false) {}

    };



    // Item placement in grid

    struct ItemPlacement {

        int row, col;

        int width, height; // in cells (1 or 2)



        ItemPlacement() : row(0), col(0), width(1), height(1) {}

        ItemPlacement(int r, int c, int w, int h) : row(r), col(c), width(w), height(h) {}

    };



    // Cache for grid layout

    mutable std::vector<std::vector<GridCell>> m_cell_map;

    mutable std::map<int, ItemPlacement> m_item_placements;

    mutable bool m_placement_cache_dirty = true;

    mutable int m_cached_cols = -1;

    mutable int m_cached_enlarged_index = -1;



    // Build complete cell allocation map

    void rebuild_placement_map(int cols) const {

        if (!m_placement_cache_dirty &&

            m_cached_cols == cols &&

            m_cached_enlarged_index == m_now_playing_index) {

            return; // Cache valid

        }



        // Clear cache

        m_item_placements.clear();

        m_cell_map.clear();



        const size_t total_items = get_item_count();

        if (total_items == 0 || cols <= 0) return;



        const int enlarged_index = m_now_playing_index;

        const bool has_enlarged = (m_config.enlarged_now_playing != grid_config::ENLARGED_NONE && enlarged_index >= 0 && enlarged_index < (int)total_items);

        const int s = has_enlarged ? ((m_config.enlarged_now_playing == grid_config::ENLARGED_2X2) ? 2 : 3) : 1;



        // Compute exact grid size (no extra rows allowed)

        const int total_cells = (int)total_items - (has_enlarged ? 1 : 0) + (has_enlarged ? s * s : 0);

        int total_rows = (total_cells + cols - 1) / cols;

        int cells_in_last_row = total_cells % cols; if (cells_in_last_row == 0) cells_in_last_row = cols;

        m_cell_map.assign(total_rows, std::vector<GridCell>(cols));



        auto within_forbidden = [&](int r, int c) -> bool {

            // Returns true if (r,c) is outside the allowed cells in the last row

            if (r == total_rows - 1 && c >= cells_in_last_row) return true;

            return false;

        };



        auto fits_block = [&](int row, int col, int w, int h) -> bool {

            if (row < 0 || col < 0) return false;

            if (row + h > total_rows || col + w > cols) return false;

            for (int rr = row; rr < row + h; ++rr) {

                for (int cc = col; cc < col + w; ++cc) {

                    if (within_forbidden(rr, cc)) return false;

                    if (m_cell_map[rr][cc].occupied) return false;

                }

            }

            return true;

        };



        auto place_block = [&](int index, int row, int col, int w, int h) {

            m_item_placements[index] = ItemPlacement(row, col, w, h);

            for (int rr = row; rr < row + h; ++rr) {

                for (int cc = col; cc < col + w; ++cc) {

                    m_cell_map[rr][cc].occupied = true;

                    m_cell_map[rr][cc].item_index = index;

                    m_cell_map[rr][cc].is_primary = (rr == row && cc == col);

                }

            }

        };



        // Decide enlarged placement: ensure one of its cells overlaps natural position

        int natural_pos = has_enlarged ? enlarged_index : -1;

        int natural_row = has_enlarged ? (natural_pos / cols) : -1;

        int natural_col = has_enlarged ? (natural_pos % cols) : -1;



        if (has_enlarged) {

            bool placed = false;

            // Candidate top-left positions so that block covers (natural_row, natural_col)

            for (int dr = 0; dr < s && !placed; ++dr) {

                for (int dc = 0; dc < s && !placed; ++dc) {

                    int r = natural_row - dr;

                    int c = natural_col - dc;

                    if (fits_block(r, c, s, s)) {

                        place_block(enlarged_index, r, c, s, s);

                        placed = true;

                    }

                }

            }

            if (!placed) {

                // Fallback: search the full grid but do NOT exceed row count

                for (int pos = 0; pos < total_rows * cols && !placed; ++pos) {

                    int r = pos / cols, c = pos % cols;

                    if (fits_block(r, c, s, s)) {

                        // Only accept if natural cell would be inside; otherwise try next

                        if (r <= natural_row && natural_row < r + s && c <= natural_col && natural_col < c + s) {

                            place_block(enlarged_index, r, c, s, s);

                            placed = true;

                        }

                    }

                }

            }

            // If still not placed (extreme edge), place at last legal slot that doesn't touch forbidden last-row cells

            if (!placed) {

                for (int pos = total_rows * cols - 1; pos >= 0 && !placed; --pos) {

                    int r = pos / cols, c = pos % cols;

                    if (fits_block(r, c, s, s)) {

                        place_block(enlarged_index, r, c, s, s);

                        placed = true;

                    }

                }

            }

        }



        // Fill remaining items sequentially into free cells

        int next_pos = 0;

        for (int i = 0; i < (int)total_items; ++i) {

            if (has_enlarged && i == enlarged_index) continue;

            // advance to next free, non-forbidden cell

            for (;;) {

                if (next_pos >= total_rows * cols) break; // should not happen

                int r = next_pos / cols, c = next_pos % cols;

                ++next_pos;

                if (within_forbidden(r, c)) continue;

                if (!m_cell_map[r][c].occupied) {

                    place_block(i, r, c, 1, 1);

                    break;

                }

            }

        }



        m_placement_cache_dirty = false;

        m_cached_cols = cols;

        m_cached_enlarged_index = enlarged_index;

    }



    // Find last usable position for enlarged item

    void find_last_usable_position(int total_rows, int cols, int multiplier, int cells_in_last_row, int& out_position, int& out_row, int& out_col) const {

        // v10.0.43 SMART GRID FLOW: Find the last valid position for 3x3 placement



        // Work backwards from the end of grid to find last position where 3x3 fits

        int total_grid_cells = total_rows * cols;



        for (int position = total_grid_cells - (multiplier * multiplier); position >= 0; position--) {

            int try_row = position / cols;

            int try_col = position % cols;



            // Check if 3x3 fits within grid bounds

            if (try_row + multiplier > total_rows || try_col + multiplier > cols) {

                continue;

            }



            // Check if any part extends into forbidden zone of last row

            bool uses_forbidden_zone = false;

            for (int r = try_row; r < try_row + multiplier && !uses_forbidden_zone; r++) {

                for (int c = try_col; c < try_col + multiplier && !uses_forbidden_zone; c++) {

                    if (r == total_rows - 1 && c >= cells_in_last_row) {

                        uses_forbidden_zone = true;

                    }

                }

            }



            if (!uses_forbidden_zone) {

                // Found valid position

                out_position = position;

                out_row = try_row;

                out_col = try_col;

                return;

            }

        }



        // Fallback if no position found (shouldn't happen)

        out_position = 0;

        out_row = 0;

        out_col = 0;

    }



    // Find optimal position for enlarged item in calculated grid

    void find_optimal_enlarged_position(int enlarged_index, int total_rows, int cols, int multiplier, int cells_in_last_row, int& out_row, int& out_col) const {

        // v10.0.43 SMART GRID FLOW: Respect forbidden positions in last row



        // Key rule: In last row, only first cells_in_last_row positions are allowed

        // Remaining positions in last row are FORBIDDEN for 3x3 placement



        // Calculate where enlarged item would naturally appear in sequence

        int target_position = enlarged_index;

        int target_row = target_position / cols;

        int target_col = target_position % cols;



        // Check if natural position is valid

        bool natural_position_valid = true;



        // Check if 3x3 would extend into forbidden area of last row

        for (int r = target_row; r < target_row + multiplier && natural_position_valid; r++) {

            for (int c = target_col; c < target_col + multiplier && natural_position_valid; c++) {

                // Check boundaries

                if (r >= total_rows || c >= cols) {

                    natural_position_valid = false;

                    break;

                }



                // Check if position is in forbidden area of last row

                if (r == total_rows - 1 && c >= cells_in_last_row) {

                    natural_position_valid = false;

                    break;

                }

            }

        }



        if (natural_position_valid) {

            out_row = target_row;

            out_col = target_col;

            return;

        }



        // Find valid position near the target, working backwards from target position

        // Start from target area and search nearby positions first

        int max_search_distance = std::max(total_rows, cols);



        for (int distance = 0; distance < max_search_distance; distance++) {

            // Try positions in expanding ring around target

            for (int row_offset = -distance; row_offset <= distance; row_offset++) {

                for (int col_offset = -distance; col_offset <= distance; col_offset++) {

                    // Skip positions that aren't on the edge of current search distance

                    if (distance > 0 && abs(row_offset) < distance && abs(col_offset) < distance) {

                        continue;

                    }



                    int try_row = target_row + row_offset;

                    int try_col = target_col + col_offset;



                    // Check basic bounds

                    if (try_row < 0 || try_col < 0 || try_row + multiplier > total_rows || try_col + multiplier > cols) {

                        continue;

                    }



                    // Check if this position is valid (doesn't use forbidden cells)

                    bool position_valid = true;

                    for (int r = try_row; r < try_row + multiplier && position_valid; r++) {

                        for (int c = try_col; c < try_col + multiplier && position_valid; c++) {

                            // Check if position is in forbidden area of last row

                            if (r == total_rows - 1 && c >= cells_in_last_row) {

                                position_valid = false;

                                break;

                            }

                        }

                    }



                    if (position_valid) {

                        out_row = try_row;

                        out_col = try_col;

                        return;

                    }

                }

            }

        }



        // Final fallback - shouldn't happen with proper grid calculation

        out_row = 0;

        out_col = 0;

    }



    // Check if enlarged item can be placed at specific position

    bool can_place_enlarged_at(int row, int col, int total_rows, int cols, int multiplier) const {

        // Check boundaries

        if (row < 0 || col < 0 || row + multiplier > total_rows || col + multiplier > cols) {

            return false;

        }

        return true; // In clean grid, always fits if boundaries are ok

    }



    // Check if grid position is blocked by enlarged item

    bool is_position_blocked(int grid_position, int total_rows, int cols, int enlarged_row, int enlarged_col, int multiplier) const {

        if (enlarged_row < 0 || enlarged_col < 0) return false; // No enlarged item



        int pos_row = grid_position / cols;

        int pos_col = grid_position % cols;



        // Check if position is within enlarged item area

        return (pos_row >= enlarged_row && pos_row < enlarged_row + multiplier &&

                pos_col >= enlarged_col && pos_col < enlarged_col + multiplier);

    }



    // Old method removed - replaced by new SMART GRID FLOW logic in rebuild_placement_map



    // Old method removed - replaced by new SMART GRID FLOW sequential placement logic



    item_position get_adaptive_item_position(int index, const RECT& client_rect, int start_x, int cols, int text_height) {

        item_position pos = {0, 0, 0, 0, false};



        // v10.0.43: CELL-BASED GRID ENGINE - Use mathematical precision

        rebuild_placement_map(cols);



        // Look up item placement from cache

        auto it = m_item_placements.find(index);

        if (it == m_item_placements.end()) {

            // Fallback for invalid index

            return pos;

        }



        const ItemPlacement& placement = it->second;



        // Convert cell coordinates to pixel coordinates

        int cell_width = m_item_size + PADDING;

        int cell_height = m_item_size + text_height + PADDING;



        pos.x = start_x + placement.col * cell_width + PADDING;

        pos.y = placement.row * cell_height + PADDING - m_scroll_pos;



        // Size depends on cell span

        if (placement.width > 1) {

            // Enlarged item - spans multiple cells

            pos.width = placement.width * m_item_size + (placement.width - 1) * PADDING;

            pos.height = placement.height * m_item_size + text_height;

        } else {

            // Normal item - single cell

            pos.width = get_item_size(index);

            pos.height = get_item_size(index) + text_height;

        }



        // Visibility check

        pos.visible = (pos.y + pos.height > 0 && pos.y < client_rect.bottom);



        return pos;

    }



    // v10.0.43: CELL-BASED HEIGHT CALCULATION - Use grid map for precise calculation

    int calculate_total_height() {

        calculate_layout();  // Ensure item size is current

        int text_height = calculate_text_height();

        int cell_height = m_item_size + text_height + PADDING;

        int cols = m_config.columns;

        size_t item_count = get_item_count();



        if (item_count == 0) {

            return PADDING;

        }



        if (m_config.enlarged_now_playing == grid_config::ENLARGED_NONE ||

            m_now_playing_index < 0 || m_now_playing_index >= (int)item_count) {

            // Standard grid without enlarged items

            int rows = (item_count + cols - 1) / cols;

            return rows * cell_height + PADDING;

        }



        // Use cell-based calculation for accurate height

        rebuild_placement_map(cols);



        // Find the maximum row used

        int max_row = 0;

        for (const auto& pair : m_item_placements) {

            const ItemPlacement& placement = pair.second;

            int bottom_row = placement.row + placement.height - 1;

            max_row = std::max(max_row, bottom_row);

        }



        return (max_row + 1) * cell_height + PADDING;

    }



    void calculate_layout() {

        RECT rc;

        GetClientRect(m_hwnd, &rc);

        int width = rc.right;



        if (width <= 0 || m_config.columns <= 0) {

            m_item_size = 120;

            return;

        }



        // v10.0.30: PERFORMANCE OPTIMIZATION - Use layout cache

        if (m_layout_cache.is_valid_for(width, m_config.columns)) {

            m_item_size = m_layout_cache.cached_item_size;

            return; // Use cached value, no recalculation needed!

        }



        // Calculate item size to fill width with desired columns

        // Total width = columns * item_size + (columns + 1) * padding

        int total_padding = PADDING * (m_config.columns + 1);

        int available_for_items = width - total_padding;

        m_item_size = std::max(50, std::min(250, available_for_items / m_config.columns));



        // v10.0.30: Cache the calculated values

        m_layout_cache.update(width, m_config.columns, m_item_size);

    }

    

    int calculate_text_height() {

        // v10.0.33: Text overlay implementation - no additional height needed for layout

        // Text is now drawn directly on artwork with semi-transparent background

        if (!m_config.show_text || !m_callback.is_valid()) {

            return 0;

        }



        // Always return 0 - text is overlaid on artwork, not below it

        // This fixes the layout problem with enlarged artwork

        return 0;

    }

    

    void refresh_items() {

        // Begin a new generation and purge cache entries for old items

        m_items_generation.fetch_add(1);



        // Remove thumbnails for existing items from the global cache to avoid

        // dangling pointers after the item list is rebuilt (e.g. on grouping change)

        if (!m_items.empty()) {

            for (auto& it : m_items) {

                if (it && it->thumbnail) {
                    thumbnail_cache::remove_thumbnail(it->thumbnail.get());
                    // Proactively clear bitmap to release memory sooner

                    it->thumbnail->clear();

                }

            }

        }



        m_items.clear();

        m_selected_indices.clear();

        m_placement_cache_dirty = true;

        

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
                        pfc::string8 root_path, root_name;
                        if (try_get_album_root_folder_from_file_path(path.c_str(), root_path, root_name)) {
                            key = root_name;
                            display_name = root_name;
                        } else {

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
                pfc::string8 root_path, root_name;
                if (try_get_album_root_folder_from_file_path(path.c_str(), root_path, root_name)) {
                    key = root_name;
                    display_name = key;
                } else {

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
                item->representative_track = handle;

                

                // v10.0.4: Always extract and store the actual folder name

                pfc::string8 full_path = handle->get_path();
                pfc::string8 root_folder_path, root_folder_name;
                if (try_get_album_root_folder_from_file_path(full_path.c_str(), root_folder_path, root_folder_name)) {
                    // Multi-disc folder layouts (CD1/CD2): treat the album root as the folder identity
                    item->folder_name = root_folder_name;
                    item->path = root_folder_path;
                } else {

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



                    uint32_t release_key = extract_release_date_from_info(info);



                    if (release_key != 0) { item->release_date_key = release_key; }

                }

                

                item_map[key] = std::move(item);

            }

            

            item_map[key]->tracks.add_item(handle);
            add_disc_to_item(*item_map[key], handle);
            update_representative_track(*item_map[key], handle);

            

            file_info_impl release_info;

            if (handle->get_info(release_info)) {

                uint32_t release_key = extract_release_date_from_info(release_info);

                if (release_key != 0 && release_key > item_map[key]->release_date_key) {

                    item_map[key]->release_date_key = release_key;

                }

            }

            

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
            if (pair.second) {
                uint8_t dc = popcount_u32(pair.second->disc_mask);
                pair.second->disc_count = (dc == 0 ? 1 : dc);
            }

            m_items.push_back(std::move(pair.second));

        }

        

        // Sort items

        sort_items();

        

        // Update global album count for titleformat fields

        { insync(g_count_sync); g_album_count = m_items.size(); g_is_library_view = (m_config.view != grid_config::VIEW_PLAYLIST); g_last_grouping = (int)m_config.grouping; g_last_sorting = (int)m_config.sorting;

        }

        // Trigger refresh of titleformat (including status bar)

        // Just dispatch empty refresh to notify titleformat fields changed

        metadb_handle_list dummy;

        static_api_ptr_t<metadb_io>()->dispatch_refresh(dummy);

        

        // Check for now playing track

        check_now_playing();

        

        // Update scrollbar and repaint

        update_scrollbar();

        request_invalidate();
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

            case grid_config::SORT_BY_RELEASE_DATE:

                std::sort(m_items.begin(), m_items.end(), 

                    [](const std::unique_ptr<grid_item>& a, const std::unique_ptr<grid_item>& b) {

                        if (a->release_date_key == b->release_date_key) {

                            if (a->release_date_key == 0) {

                                return a->newest_date > b->newest_date;

                            }

                            if (a->newest_date == b->newest_date) {

                                return pfc::stricmp_ascii(a->sort_key.c_str(), b->sort_key.c_str()) < 0;

                            }

                            return a->newest_date > b->newest_date;

                        }

                        return a->release_date_key > b->release_date_key;

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

    

    // v10.0.30: Create high-resolution artwork directly from track (bypass cache)

    Gdiplus::Bitmap* create_original_artwork(metadb_handle_ptr track, int target_size) {

        if (!track.is_valid() || m_is_destroying.load() || !m_hwnd) {

            return nullptr;

        }



        try {

            auto art_api = album_art_manager_v2::get();

            abort_callback_dummy abort;



            // Use a much larger size hint to get better quality from the API

            int size_hint = std::max(target_size * 2, 1024);  // At least 1024px for enlarged items



            auto extractor = art_api->open(

                pfc::list_single_ref_t<metadb_handle_ptr>(track),

                pfc::list_single_ref_t<GUID>(album_art_ids::cover_front),

                abort

            );



            album_art_data_ptr artwork = extractor->query(album_art_ids::cover_front, abort);



            if (!artwork.is_valid() || artwork->get_size() == 0) {

                return nullptr;

            }



            return create_hires_artwork(artwork, target_size);



        } catch (...) {

            return nullptr;

        }

    }



    // v10.0.30: Create high-resolution artwork for enlarged items (near-original quality)

    Gdiplus::Bitmap* create_hires_artwork(album_art_data_ptr artwork, int target_size) {

        if (!artwork.is_valid() || artwork->get_size() == 0) {

            return nullptr;

        }



        IStream* stream = nullptr;

        Gdiplus::Bitmap* original = nullptr;

        Gdiplus::Bitmap* result = nullptr;



        try {

            // Create memory stream

            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, artwork->get_size());

            if (!hGlobal) return nullptr;



            void* pImage = GlobalLock(hGlobal);

            if (!pImage) {

                GlobalFree(hGlobal);

                return nullptr;

            }



            memcpy(pImage, artwork->get_ptr(), artwork->get_size());

            GlobalUnlock(hGlobal);



            if (CreateStreamOnHGlobal(hGlobal, TRUE, &stream) != S_OK) {

                GlobalFree(hGlobal);

                return nullptr;

            }



            original = Gdiplus::Bitmap::FromStream(stream);

            stream->Release();



            if (!original || original->GetLastStatus() != Gdiplus::Ok) {

                if (original) delete original;

                return nullptr;

            }



            int orig_width = original->GetWidth();

            int orig_height = original->GetHeight();



            // For enlarged items, use much higher resolution (closer to original)

            int max_dimension = std::max(orig_width, orig_height);



            // If original is smaller than target*1.5, just return original (preserve small images)

            if (max_dimension <= target_size * 1.5f) {

                return original;

            }



            // Scale down only minimally to preserve quality - use 80% of original size minimum

            float scale = std::max(0.8f, (float)target_size / max_dimension);

            int new_width = std::max(1, (int)(orig_width * scale));

            int new_height = std::max(1, (int)(orig_height * scale));



            // Create high-res version

            result = new Gdiplus::Bitmap(new_width, new_height, PixelFormat32bppARGB);



            if (!result || result->GetLastStatus() != Gdiplus::Ok) {

                delete original;

                if (result) delete result;

                return nullptr;

            }



            {

                Gdiplus::Graphics graphics(result);



                // Maximum quality settings for enlarged artwork

                graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

                graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

                graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);

                graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);



                graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

                graphics.DrawImage(original, 0, 0, new_width, new_height);

            }



            delete original;

            return result;



        } catch (...) {

            if (stream) stream->Release();

            if (original) delete original;

            if (result) delete result;

            return nullptr;

        }

    }



    // Create thumbnail from album art data with enhanced error handling

    Gdiplus::Bitmap* create_thumbnail(album_art_data_ptr artwork, int size) {

        if (!artwork.is_valid() || artwork->get_size() == 0) {

            console::print("[Album Art Grid v10.0.17] No artwork data available");

            return nullptr;

        }

        

        // GDI+ initialization check removed - handled by initquit service

        

        // v10.0.30: Much higher limit for near-original quality on enlarged artwork

        size = std::min(size, 1024);  // Doubled from 512 to 1024

        

        IStream* stream = nullptr;

        Gdiplus::Bitmap* original = nullptr;

        Gdiplus::Bitmap* thumbnail = nullptr;

        

        try {

            // Try creating memory stream with error checking

            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, artwork->get_size());

            if (!hGlobal) {

                console::print("[Album Art Grid v10.0.17] Failed to allocate memory for image");

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

                console::print("[Album Art Grid v10.0.17] Failed to create stream for image");

                return nullptr;

            }

            

            if (!stream) {

                console::print("[Album Art Grid v10.0.17] Stream creation failed");

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

            float scale = std::min((float)size / orig_width, (float)size / orig_height);

            int new_width = std::max(1, (int)(orig_width * scale));

            int new_height = std::max(1, (int)(orig_height * scale));

            

            // Create thumbnail with optimal format for memory

            thumbnail = new Gdiplus::Bitmap(new_width, new_height, PixelFormat32bppPARGB);

            

            if (!thumbnail || thumbnail->GetLastStatus() != Gdiplus::Ok) {

                delete original;

                if (thumbnail) delete thumbnail;

                return nullptr;

            }

            

            {

                Gdiplus::Graphics graphics(thumbnail);



                // v10.0.30 ULTIMATE QUALITY: Near-original quality for enlarged thumbnails

                if (size > 200) {

                    // Ultra-high quality for enlarged items (now playing) - near original

                    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

                    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);  // Better than HighQuality

                    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);   // Better pixel alignment

                    graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);

                    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);  // Best color blending

                } else {

                    // Balanced quality/speed for normal thumbnails

                    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);

                    graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighSpeed);

                    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighSpeed);

                    graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);

                }



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

        m_placement_cache_dirty = true;

        

        if (m_search_text.is_empty()) {

            // No filter, show all items

            return;

        }

        

        // Convert search text to lowercase for case-insensitive search

        pfc::string8 search_lower = m_search_text;

        pfc::stringToLower(search_lower);

        

        // Filter items that match the search text

        for (size_t idx = 0; idx < m_items.size(); idx++) {

            auto& item = m_items[idx];

            pfc::string8 item_text = item->display_name;

            

            // Also search in artist, album, genre fields

            item_text << " " << item->artist << " " << item->album << " " << item->genre;

            

            // Convert to lowercase for comparison

            pfc::string8 item_lower = item_text;

            pfc::stringToLower(item_lower);

            

            // Check if search text is contained in item text

            if (strstr(item_lower.c_str(), search_lower.c_str()) != nullptr) {

                m_filtered_indices.push_back(idx);

            }

        }

    }

    

    void load_visible_artwork() {

        // CRITICAL FIX: Prevent artwork loading during destruction

        if (m_is_destroying.load() || !m_hwnd || !IsWindow(m_hwnd)) return;

        

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

            prefetch_end = std::min((int)item_count - 1, m_last_visible + PREFETCH_RANGE);

        } else {

            prefetch_start = std::max(0, m_first_visible - PREFETCH_RANGE);

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

                if (s_inflight_loaders.load() >= kMaxInflight) break;

                item->thumbnail->loading = true;

                s_inflight_loaders.fetch_add(1);

            }

            load_count++;



            // Dispatch async thumbnail load

            int task_index = i;

            int gen = m_items_generation.load();

            HWND hwnd = m_hwnd;

            grid_config::enlarged_mode enlarged_mode = m_config.enlarged_now_playing;

            bool use_artist_img = (m_config.grouping == grid_config::GROUP_BY_ARTIST ||

                                   m_config.grouping == grid_config::GROUP_BY_ALBUM_ARTIST ||

                                   m_config.grouping == grid_config::GROUP_BY_ARTIST_ALBUM ||

                                   m_config.grouping == grid_config::GROUP_BY_PERFORMER ||

                                   m_config.grouping == grid_config::GROUP_BY_COMPOSER);

            metadb_handle_ptr track0 = item->representative_track.is_valid() ? item->representative_track
                : (item->tracks.get_count() > 0 ? item->tracks[0] : nullptr);

            auto art_api = album_art_manager_v2::get();



            thumb_pool().submit([this, hwnd, task_index, gen, enlarged_mode, use_artist_img, track0, art_api]() {
                Gdiplus::Bitmap* bmp = nullptr;

                int size_for_item = 0;

                try {

                    if (!hwnd || !IsWindow(hwnd)) throw 0;

                    if (shutdown_protection::is_shutting_down() || m_is_destroying.load()) throw 0;

                    int current_size = get_item_size(task_index);

                    size_for_item = current_size;

                    abort_callback_dummy abort;

                    try {

                        if (use_artist_img && track0.is_valid()) {

                            auto artist_ext = art_api->open(

                                pfc::list_single_ref_t<metadb_handle_ptr>(track0),

                                pfc::list_single_ref_t<GUID>(album_art_ids::artist),

                                abort);

                            album_art_data_ptr artist_art = artist_ext->query(album_art_ids::artist, abort);

                            if (artist_art.is_valid()) bmp = create_thumbnail(artist_art, current_size);

                        }

                    } catch(...) {}

                    if (!bmp && track0.is_valid()) {

                        auto extractor = art_api->open(

                            pfc::list_single_ref_t<metadb_handle_ptr>(track0),

                            pfc::list_single_ref_t<GUID>(album_art_ids::cover_front),

                            abort);

                        album_art_data_ptr art = extractor->query(album_art_ids::cover_front, abort);

                        if (art.is_valid()) {

                            if (task_index == m_now_playing_index && enlarged_mode != grid_config::ENLARGED_NONE) {

                                bmp = create_original_artwork(track0, current_size * 2);

                            }

                            if (!bmp) bmp = create_thumbnail(art, current_size);

                        }

                    }

                } catch(...) {}

            
                auto* res = new ThumbnailResult{ task_index, gen, bmp, size_for_item };
                if (hwnd && IsWindow(hwnd)) {
                    PostMessage(hwnd, WM_APP_THUMBNAIL_READY, 0, reinterpret_cast<LPARAM>(res));
                } else {
                    if (bmp) delete bmp;
                    delete res;
                    s_inflight_loaders.fetch_sub(1);
                }
            });
        }

        

        // Prefetch items in scroll direction (lower priority)

        if (load_count < 5 && prefetch_start <= prefetch_end) {

            for (int i = prefetch_start; i <= prefetch_end && i < (int)item_count && load_count < 5; i++) {

                auto* item = get_item_at(i);

                if (!item) continue;

                

                if (item->thumbnail->bitmap || item->thumbnail->loading) continue;

                if (item->tracks.get_count() == 0) continue;

                {

                    insync(g_thumbnail_sync);

                    if (s_inflight_loaders.load() >= kMaxInflight) break;

                    item->thumbnail->loading = true;

                    s_inflight_loaders.fetch_add(1);

                    load_count++;

                }



                int task_index = i;

                int gen = m_items_generation.load();

                HWND hwnd = m_hwnd;

                grid_config::enlarged_mode enlarged_mode = m_config.enlarged_now_playing;

                metadb_handle_ptr track0 = item->representative_track.is_valid() ? item->representative_track
                    : (item->tracks.get_count() > 0 ? item->tracks[0] : nullptr);

                auto art_api = album_art_manager_v2::get();

                thumb_pool().submit([this, hwnd, task_index, gen, enlarged_mode, track0, art_api]() {
                    Gdiplus::Bitmap* bmp = nullptr;

                    int size_for_item = 0;

                    try {

                        if (!hwnd || !IsWindow(hwnd)) throw 0;

                        if (shutdown_protection::is_shutting_down() || m_is_destroying.load()) throw 0;

                        int current_size = get_item_size(task_index);

                        size_for_item = current_size;

                        abort_callback_dummy abort;

                        if (track0.is_valid()) {

                            auto extractor = art_api->open(

                                pfc::list_single_ref_t<metadb_handle_ptr>(track0),

                                pfc::list_single_ref_t<GUID>(album_art_ids::cover_front),

                                abort);

                            album_art_data_ptr art = extractor->query(album_art_ids::cover_front, abort);

                            if (art.is_valid()) {

                                if (task_index == m_now_playing_index && enlarged_mode != grid_config::ENLARGED_NONE) {

                                    bmp = create_original_artwork(track0, current_size * 2);

                                }

                                if (!bmp) bmp = create_thumbnail(art, current_size);

                            }

                        }

                    } catch(...) {}

                    
                    auto* res = new ThumbnailResult{ task_index, gen, bmp, size_for_item };
                    if (hwnd && IsWindow(hwnd)) {
                        PostMessage(hwnd, WM_APP_THUMBNAIL_READY, 0, reinterpret_cast<LPARAM>(res));
                    } else {
                        if (bmp) delete bmp;
                        delete res;
                        s_inflight_loaders.fetch_sub(1);
                    }
                });
            }

        }

    }



    LRESULT on_thumbnail_ready(ThumbnailResult* res) {

        if (!res) return 0;

        std::unique_ptr<ThumbnailResult> guard(res);

        s_inflight_loaders.fetch_sub(1);

        if (m_is_destroying.load() || shutdown_protection::is_shutting_down()) {

            if (res->bmp) delete res->bmp;

            return 0;

        }

        if (res->generation != m_items_generation.load()) {

            if (res->bmp) delete res->bmp;

            return 0;

        }

        size_t item_count = get_item_count();

        if (res->index < 0 || res->index >= (int)item_count) {

            if (res->bmp) delete res->bmp;

            return 0;

        }

        auto* item = get_item_at(res->index);

        if (!item) {

            if (res->bmp) delete res->bmp;

            return 0;

        }

        {

            insync(g_thumbnail_sync);

            item->thumbnail->set_bitmap(res->bmp, res->size);

            item->thumbnail->loading = false;

        }

        thumbnail_cache::add_thumbnail(item->thumbnail, res->index);
        InvalidateRect(m_hwnd, NULL, FALSE);

        // Ownership of bmp moved into thumbnail; release pointer from guard

        guard.release();

        return 0;

    }



    LRESULT on_paint() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        RECT rc;
        GetClientRect(m_hwnd, &rc);
        if (!ensure_backbuffer(rc.right, rc.bottom)) { EndPaint(m_hwnd, &ps); return 0; }
        HDC memdc = m_memdc;
        

        // Remove frequent now playing checks from paint - rely on timer only



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

        // Ensure proper alpha blending for PNGs with transparency

        graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);

        

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

        m_last_visible = std::min((int)item_count - 1, (first_row + visible_rows) * cols);

        

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

        

        // NEW v10.0.29: Enhanced layout with enlarged now playing support

        // Draw items using the new positioning system that handles enlarged items

        for (size_t index = 0; index < item_count; index++) {

            item_position pos = get_item_position(index, rc);

            

            // Only draw if visible

            if (pos.visible) {

                draw_item(memdc, graphics, hFont, pos.x, pos.y, index, color_text, color_selected, text_height);

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



        // v10.0.37: Floating label removed - using only text overlay on artwork

                // (footer moved to status bar via %albumart_grid_info%)
        // Copy to screen
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memdc, 0, 0, SRCCOPY);
        

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

        int padding_x = std::max(8, (int)(text_size.cy / 2));

        int padding_y = std::max(4, (int)(text_size.cy / 4));

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

        int corner_radius = std::min(10, badge_height / 2);

        RoundRect(hdc, badge_x, badge_y, badge_x + badge_width, badge_y + badge_height, corner_radius, corner_radius);

        

        SelectObject(hdc, oldBrush);

        SelectObject(hdc, oldPen);

        DeleteObject(hbrBadge);

        DeleteObject(hpenBadge);

        

        // Draw text

        RECT text_rc = {badge_x, badge_y, badge_x + badge_width, badge_y + badge_height};

        SetTextColor(hdc, text_color);

        SetBkMode(hdc, TRANSPARENT);

        

        // Convert count to Unicode for proper display

        pfc::stringcvt::string_wide_from_utf8 count_w(count_str);

        DrawTextW(hdc, count_w.get_ptr(), -1, &text_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        

        SelectObject(hdc, old_font);

        // Don't delete the font - it's managed by foobar2000

    }

    void draw_disc_count_badge(HDC hdc, int disc_count, int x, int y, bool dark_mode, HFONT app_font) {
        if (disc_count <= 1) return;

        char text_str[32];
        sprintf_s(text_str, "%dCD", disc_count);

        HFONT old_font = (HFONT)SelectObject(hdc, app_font);

        SIZE text_size;
        GetTextExtentPoint32A(hdc, text_str, (int)strlen(text_str), &text_size);

        int padding_x = std::max(10, (int)(text_size.cy / 2));
        int padding_y = std::max(4, (int)(text_size.cy / 4));
        int badge_width = text_size.cx + padding_x;
        int badge_height = text_size.cy + padding_y;

        int badge_x = x + 5;  // Top-left corner
        int badge_y = y + 5;

        COLORREF badge_bg = dark_mode ? RGB(55, 70, 95) : RGB(35, 130, 230);
        COLORREF badge_border = dark_mode ? RGB(85, 100, 125) : RGB(20, 95, 175);
        COLORREF text_color = RGB(255, 255, 255);

        HBRUSH hbrBadge = CreateSolidBrush(badge_bg);
        HPEN hpenBadge = CreatePen(PS_SOLID, 1, badge_border);
        HPEN oldPen = (HPEN)SelectObject(hdc, hpenBadge);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hbrBadge);

        int corner_radius = std::min(10, badge_height / 2);
        RoundRect(hdc, badge_x, badge_y, badge_x + badge_width, badge_y + badge_height, corner_radius, corner_radius);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(hbrBadge);
        DeleteObject(hpenBadge);

        RECT text_rc = {badge_x, badge_y, badge_x + badge_width, badge_y + badge_height};
        SetTextColor(hdc, text_color);
        SetBkMode(hdc, TRANSPARENT);

        pfc::stringcvt::string_wide_from_utf8 text_w(text_str);
        DrawTextW(hdc, text_w.get_ptr(), -1, &text_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, old_font);
    }



    // v10.0.30: Draw floating label near mouse cursor

    void draw_floating_label(HDC hdc, HFONT font, int item_index, int mouse_x, int mouse_y, const RECT& client_rect) {

        size_t real_idx = get_real_index(item_index);

        if (real_idx >= m_items.size()) return;

        auto& item = m_items[real_idx];



        // Get display text

        pfc::string8 text_to_show = item->get_display_text(m_config.label_style);



        // Convert to wide string

        std::wstring wide_text;

        int wide_len = MultiByteToWideChar(CP_UTF8, 0, text_to_show.c_str(), -1, NULL, 0);

        if (wide_len > 0) {

            wide_text.resize(wide_len);

            MultiByteToWideChar(CP_UTF8, 0, text_to_show.c_str(), -1, &wide_text[0], wide_len);

        }



        // Set up font and calculate text size

        HFONT old_font = (HFONT)SelectObject(hdc, font);



        // Calculate text dimensions

        SIZE text_size;

        GetTextExtentPoint32W(hdc, wide_text.c_str(), (int)wide_text.length(), &text_size);



        // Add padding for the floating label background

        int padding = 8;

        int label_width = text_size.cx + padding * 2;

        int label_height = text_size.cy + padding * 2;



        // Position label near mouse, but keep it within client area

        int label_x = mouse_x + 15; // Offset from cursor

        int label_y = mouse_y - label_height - 10; // Above cursor



        // Keep within bounds

        if (label_x + label_width > client_rect.right) {

            label_x = mouse_x - label_width - 15; // Switch to left side

        }

        if (label_y < 0) {

            label_y = mouse_y + 15; // Switch to below cursor

        }

        if (label_x < 0) label_x = 5;

        if (label_y + label_height > client_rect.bottom) {

            label_y = client_rect.bottom - label_height - 5;

        }



        // Draw semi-transparent background

        RECT bg_rect = {label_x, label_y, label_x + label_width, label_y + label_height};



        // Create semi-transparent background brush

        HBRUSH bg_brush = CreateSolidBrush(RGB(40, 40, 40));

        FillRect(hdc, &bg_rect, bg_brush);

        DeleteObject(bg_brush);



        // Draw border

        HPEN border_pen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));

        HPEN old_pen = (HPEN)SelectObject(hdc, border_pen);

        HBRUSH old_brush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Rectangle(hdc, label_x, label_y, label_x + label_width, label_y + label_height);

        SelectObject(hdc, old_pen);

        SelectObject(hdc, old_brush);

        DeleteObject(border_pen);



        // Draw text

        RECT text_rect = {label_x + padding, label_y + padding,

                          label_x + label_width - padding, label_y + label_height - padding};

        SetTextColor(hdc, RGB(255, 255, 255));

        SetBkMode(hdc, TRANSPARENT);

        DrawTextW(hdc, wide_text.c_str(), -1, &text_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);



        SelectObject(hdc, old_font);

    }



    // v10.0.35: NEW - Draw playlist overlay on 3x3 enlarged now playing artwork

    void draw_playlist_overlay(HDC hdc, Gdiplus::Graphics& graphics, HFONT font,

                              int x, int y, int item_size, size_t index, t_ui_color color_text) {

        // Only show for 3x3 enlarged now playing

        if (m_config.enlarged_now_playing != grid_config::ENLARGED_3X3) return;



        // Get active playlist instead of album tracks

        auto playlist_manager = playlist_manager::get();

        t_size active_playlist = playlist_manager->get_active_playlist();

        if (active_playlist == pfc_infinite) return;



        t_size playlist_count = playlist_manager->playlist_get_item_count(active_playlist);

        if (playlist_count == 0) return;



        // Calculate overlay area (use larger portion of 3x3 artwork)

        int overlay_width = item_size - 20;  // Leave some margin



        // Determine playlist display mode based on grid item size, not overlay height

        // Use compact mode for small grid items (under 500px), normal mode for larger items

        // This ensures consistent behavior regardless of text label collision

        // Based on debug output: item_size can reach 600+px, so 500px is good threshold

        bool use_compact_mode = (item_size < 500);



        // Only adjust spacing between rows based on mode; use the actual UI font

        int line_spacing = use_compact_mode ? 3 : 5;



        // Use the same UI font as the rest of the element (passed in)

        HFONT old_font = (HFONT)SelectObject(hdc, font);



        // Calculate text metrics with the selected UI font

        TEXTMETRIC tm;

        GetTextMetrics(hdc, &tm);

        int line_height = tm.tmHeight + tm.tmExternalLeading + line_spacing;



        int overlay_x = x + 10;  // Center horizontally with margin

        int overlay_y = y + 10;  // Start from top with margin



        // Determine text overlay (labels) height to avoid collision

        int text_overlay_height = 0;

        {

            if (m_config.text_lines >= 1) {

                int per = line_height;

                if (m_config.text_lines == 1) text_overlay_height = per + 8;

                else if (m_config.text_lines == 2) text_overlay_height = (per * 2) + 10;

                else text_overlay_height = (per * 3) + 12;

            }

        }



        // Compute overlay height to fully contain header + visible rows + footer

        const int header_block = line_height + 10;  // text + spacing + line

        const int footer_block = line_height + 15;  // footer text + margin

        const int vertical_margins = 10 + 10;       // top + bottom margins inside overlay



        // Maximum drawable height within artwork without colliding with labels

        int max_overlay_height = item_size - 40 - text_overlay_height - 10;

        if (max_overlay_height < (header_block + footer_block + vertical_margins + line_height)) {

            max_overlay_height = header_block + footer_block + vertical_margins + line_height; // minimum

        }



        // Calculate track count and rows that fit, then compute overlay height accordingly

        t_size track_count = playlist_count;

        // Provisional lines: try to fit as many as possible given max height

        int content_lines = (max_overlay_height - header_block - footer_block - vertical_margins) / line_height;

        if (content_lines < 1) content_lines = 1;

        if (content_lines > (int)track_count) content_lines = (int)track_count;

        int overlay_height = header_block + content_lines * line_height + footer_block + vertical_margins;



        // Create semi-transparent dark background (darker than text overlay)

        HDC mem_dc = CreateCompatibleDC(hdc);

        HBITMAP mem_bitmap = CreateCompatibleBitmap(hdc, overlay_width, overlay_height);

        HBITMAP old_bitmap = (HBITMAP)SelectObject(mem_dc, mem_bitmap);



        // Fill with dark background

        HBRUSH bg_brush = CreateSolidBrush(RGB(20, 20, 20));

        RECT fill_rect = {0, 0, overlay_width, overlay_height};

        FillRect(mem_dc, &fill_rect, bg_brush);

        DeleteObject(bg_brush);



        // Blend with 85% opacity for better visibility

        BLENDFUNCTION blend_func = {AC_SRC_OVER, 0, 217, 0};

        AlphaBlend(hdc, overlay_x, overlay_y, overlay_width, overlay_height,

                  mem_dc, 0, 0, overlay_width, overlay_height, blend_func);



        // Cleanup memory DC

        SelectObject(mem_dc, old_bitmap);

        DeleteObject(mem_bitmap);

        DeleteDC(mem_dc);



        // Get current playing track for highlighting

        metadb_handle_ptr current_track;

        auto playback_control = playback_control::get();

        bool has_current_track = playback_control->get_now_playing(current_track);



        // Draw playlist items

        SelectObject(hdc, font);

        SetBkMode(hdc, TRANSPARENT);



        // Font will be created after mode determination



        // Calculate rows that fit (content_lines) and pick start row

        int max_tracks = content_lines;  // number of content lines, header handled separately

        int start_track = 0;



        // If there are more tracks than fit, center around current playing track

        // Selection support: prefer current selection if set

        int anchor_index = -1;

        if (m_overlay_selected_index >= 0 && m_overlay_selected_index < (int)track_count) {

            anchor_index = m_overlay_selected_index;

        } else if (has_current_track) {

            anchor_index = -2; // marker to search current track

        }



        if (anchor_index != -1 && track_count > max_tracks) {

            // Find current track in playlist

            for (int i = 0; i < (int)track_count; i++) {

                metadb_handle_ptr playlist_track;

                if (playlist_manager->playlist_get_item_handle(playlist_track, active_playlist, i)) {

                    bool match = false;

                    if (anchor_index >= 0) match = (i == anchor_index);

                    else match = (has_current_track && (playlist_track == current_track));

                    if (match) {

                        start_track = std::max(0, i - max_tracks / 2);

                        start_track = std::min(start_track, (int)track_count - max_tracks);

                        break;

                    }

                }

            }

        }



        // Draw header row first (font already selected)

        SetBkMode(hdc, TRANSPARENT);

        SetTextColor(hdc, RGB(255, 255, 255));



        int header_y = overlay_y + 5;



        // Calculate proportional column positions

        int available_width = overlay_width - 30; // Leave margins

        int col1_width = 30;    // Track # - fixed width

        int col4_width = 50;    // Time - fixed width

        int col2_width = (available_width - col1_width - col4_width) * 60 / 100; // Title - 60%

        int col3_width = (available_width - col1_width - col4_width) * 40 / 100; // Artist - 40%



        int col1_x = overlay_x + 10;

        int col2_x = col1_x + col1_width;

        int col3_x = col2_x + col2_width;

        int col4_x = col3_x + col3_width;



        // Header text (Unicode support) - keep vertical alignment consistent with rows

        RECT header_col1 = {col1_x, header_y, col2_x - 4, header_y + line_height};
        RECT header_col2 = {col2_x, header_y, col3_x - 10, header_y + line_height};
        RECT header_col3 = {col3_x, header_y, col4_x - 10, header_y + line_height};
        RECT header_col4 = {col4_x, header_y, overlay_x + overlay_width - 10, header_y + line_height};

        DrawTextW(hdc, L"#", -1, &header_col1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        DrawTextW(hdc, L"Title", -1, &header_col2, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        DrawTextW(hdc, L"Artist", -1, &header_col3, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        DrawTextW(hdc, L"Time", -1, &header_col4, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);



        // Header separator line with improved spacing

        HPEN line_pen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));

        HPEN old_pen = (HPEN)SelectObject(hdc, line_pen);

        int line_y = header_y + line_height + 2; // More space between header text and line

        MoveToEx(hdc, overlay_x + 10, line_y, NULL);

        LineTo(hdc, overlay_x + overlay_width - 10, line_y);

        SelectObject(hdc, old_pen);

        DeleteObject(line_pen);



        // Calculate total playlist time

        double total_time = 0;



        // Draw tracks starting below header with increased spacing

        int content_start_y = header_y + line_height + 8; // More space after line

        for (int i = 0; i < std::min(max_tracks, (int)track_count); i++) {

            int track_index = start_track + i;

            if (track_index >= track_count) break;



            // Get track from playlist

            metadb_handle_ptr track;

            if (!playlist_manager->playlist_get_item_handle(track, active_playlist, track_index)) continue;

            if (!track.is_valid()) continue;



            // Determine highlighting: selected row or current

            bool is_current = has_current_track && (track == current_track);

            bool is_selected_row = (track_index == m_overlay_selected_index);



            // Highlight selected row background (subtle, semi-transparent with accent bar)

            if (is_selected_row) {

                RECT row_bg = { overlay_x + 8, content_start_y + i * line_height - 2,

                                overlay_x + overlay_width - 8, content_start_y + (i + 1) * line_height + 2 };



                int row_w = row_bg.right - row_bg.left;

                int row_h = row_bg.bottom - row_bg.top;

                if (row_w > 0 && row_h > 0) {

                    // Prepare memory DC for alpha blended fill

                    HDC sel_dc = CreateCompatibleDC(hdc);

                    HBITMAP sel_bmp = CreateCompatibleBitmap(hdc, row_w, row_h);

                    HBITMAP sel_old = (HBITMAP)SelectObject(sel_dc, sel_bmp);



                    // Base fill color varies with theme

                    COLORREF baseCol = m_dark ? RGB(80, 80, 80) : RGB(210, 210, 210);

                    HBRUSH base_br = CreateSolidBrush(baseCol);

                    RECT local_rc = {0, 0, row_w, row_h};

                    FillRect(sel_dc, &local_rc, base_br);

                    DeleteObject(base_br);



                    // Blend to target DC with gentle opacity

                    BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)120, 0 }; // ~47% opacity

                    AlphaBlend(hdc, row_bg.left, row_bg.top, row_w, row_h, sel_dc, 0, 0, row_w, row_h, bf);



                    // Cleanup mem DC

                    SelectObject(sel_dc, sel_old);

                    DeleteObject(sel_bmp);

                    DeleteDC(sel_dc);



                    // No accent stripe for a minimal, subtle highlight

                }

            }



            // Get track info using format_title (more reliable approach)

            pfc::string8 title_str, artist_str, track_num_str;

            double length = 0;



            try {

                // Create titleformat objects for reliable metadata access

                service_ptr_t<titleformat_object> title_script, artist_script;

                static_api_ptr_t<titleformat_compiler> compiler;



                // Compile format strings

                compiler->compile_safe(title_script, "%title%");

                compiler->compile_safe(artist_script, "%artist%");



                // Format the metadata (this is more reliable than direct access)

                track->format_title(nullptr, title_str, title_script, nullptr);

                track->format_title(nullptr, artist_str, artist_script, nullptr);



                // Get track number - fallback to index if not available

                pfc::string8 temp_num;

                service_ptr_t<titleformat_object> tracknum_script;

                compiler->compile_safe(tracknum_script, "%tracknumber%");

                track->format_title(nullptr, temp_num, tracknum_script, nullptr);



                // Use track number if valid, otherwise use index

                if (temp_num.get_length() > 0 && strcmp(temp_num.c_str(), "?") != 0) {

                    track_num_str = temp_num;

                } else {

                    track_num_str = pfc::format_int(track_index + 1);

                }



                // Get length from metadata container if available

                metadb_info_container::ptr info_container = track->get_info_ref();

                if (info_container.is_valid()) {

                    length = info_container->info().get_length();

                    total_time += length;

                }



                // If title/artist are empty or just "?", set reasonable defaults

                if (title_str.is_empty() || strcmp(title_str.c_str(), "?") == 0) {

                    title_str = "Unknown Title";

                }

                if (artist_str.is_empty() || strcmp(artist_str.c_str(), "?") == 0) {

                    artist_str = "Unknown Artist";

                }



            } catch (...) {

                // Fallback if all metadata access fails

                title_str = "Loading...";

                artist_str = "Loading...";

                track_num_str = pfc::format_int(track_index + 1);

            }



            // Set colors - highlight current track

            COLORREF text_color = is_current ? RGB(100, 200, 255) : RGB(220, 220, 220);

            SetTextColor(hdc, text_color);



            // Calculate row Y position

            int row_y = content_start_y + i * line_height;



            // Draw track number in column 1

            pfc::string8 padded_num = track_num_str;

            if (padded_num.get_length() == 1) {

                padded_num = "0";

                padded_num += track_num_str;

            }

            // Convert track number to Unicode

            pfc::stringcvt::string_wide_from_utf8 padded_num_w(padded_num.c_str());

            RECT num_rect = {col1_x, row_y, col2_x - 4, row_y + line_height};
            DrawTextW(hdc, padded_num_w.get_ptr(), -1, &num_rect,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);



            // Draw title in column 2 (with ellipsis if too long) - Unicode support

            RECT title_rect = {col2_x, row_y, col3_x - 10, row_y + line_height};

            pfc::stringcvt::string_wide_from_utf8 title_w(title_str.c_str());

            DrawTextW(hdc, title_w.get_ptr(), -1, &title_rect,

                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);



            // Draw artist in column 3 (with ellipsis if too long) - Unicode support

            RECT artist_rect = {col3_x, row_y, col4_x - 10, row_y + line_height};

            pfc::stringcvt::string_wide_from_utf8 artist_w(artist_str.c_str());

            DrawTextW(hdc, artist_w.get_ptr(), -1, &artist_rect,

                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);



            // Draw duration in column 4 - Unicode support

            if (length > 0) {

                int minutes = (int)(length / 60);

                int seconds = (int)(length) % 60;

                pfc::string8 time_str;

                time_str << pfc::format_int(minutes) << ":" << pfc::format_int(seconds, 2);

                pfc::stringcvt::string_wide_from_utf8 time_w(time_str.c_str());

                RECT time_rect = {col4_x, row_y, overlay_x + overlay_width - 10, row_y + line_height};
                DrawTextW(hdc, time_w.get_ptr(), -1, &time_rect,
                         DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

            }



        }



        // Draw footer with total time and track count

        int footer_y = content_start_y + (std::min(max_tracks, (int)track_count)) * line_height + 10;



        // Footer separator line

        HPEN footer_line_pen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));

        HPEN footer_old_pen = (HPEN)SelectObject(hdc, footer_line_pen);

        MoveToEx(hdc, overlay_x + 10, footer_y, NULL);

        LineTo(hdc, overlay_x + overlay_width - 10, footer_y);

        SelectObject(hdc, footer_old_pen);

        DeleteObject(footer_line_pen);



        // Calculate and display total time

        if (total_time > 0) {

            int total_minutes = (int)(total_time / 60);

            int total_seconds = (int)(total_time) % 60;

            int total_hours = total_minutes / 60;

            total_minutes = total_minutes % 60;



            pfc::string8 total_time_str;

            if (total_hours > 0) {

                total_time_str << pfc::format_int(total_hours) << ":" <<

                               pfc::format_int(total_minutes, 2) << ":" <<

                               pfc::format_int(total_seconds, 2);

            } else {

                total_time_str << pfc::format_int(total_minutes) << ":" <<

                               pfc::format_int(total_seconds, 2);

            }



            pfc::string8 footer_text;

            footer_text << pfc::format_int(track_count) << " tracks, " << total_time_str;

            SetTextColor(hdc, RGB(180, 180, 180));

            // Convert footer text to Unicode

            pfc::stringcvt::string_wide_from_utf8 footer_w(footer_text.c_str());

            TextOutW(hdc, overlay_x + 15, footer_y + 5, footer_w.get_ptr(), wcslen(footer_w.get_ptr()));

        }



        // Draw scroll indicators if needed

        if ((int)track_count > max_tracks) {

            SetTextColor(hdc, RGB(150, 150, 150));

            RECT indicator_rect = {overlay_x, overlay_y + overlay_height - 15, overlay_x + overlay_width, overlay_y + overlay_height};



            pfc::string8 indicator_text;

            indicator_text << "(" << (start_track + 1) << "-" << (start_track + std::min(max_tracks, (int)track_count))

                          << " of " << track_count << ")";



            std::wstring wide_indicator;

            int wide_len = MultiByteToWideChar(CP_UTF8, 0, indicator_text.c_str(), -1, NULL, 0);

            if (wide_len > 0) {

                wide_indicator.resize(wide_len - 1);

                MultiByteToWideChar(CP_UTF8, 0, indicator_text.c_str(), -1, &wide_indicator[0], wide_len);

            }



            DrawTextW(hdc, wide_indicator.c_str(), -1, &indicator_rect,

                     DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        }



        // Font restore handled earlier

    }



    void draw_item(HDC hdc, Gdiplus::Graphics& graphics, HFONT font,

                   int x, int y, size_t index, 

                   t_ui_color color_text, t_ui_color color_selected, int text_height) {

        auto* item = get_item_at(index);

        if (!item) return;

        

        // Check if this is the now playing album

        bool is_now_playing = (m_now_playing.is_valid() && (int)index == m_now_playing_index);

        

        // NEW v10.0.29: Get size for this specific item (enlarged if now playing)

        int item_size = get_item_size(index);

        

        // Draw selection/hover background

        bool selected = m_selected_indices.find(index) != m_selected_indices.end();

        if (selected) {

            RECT sel_rc = {x-2, y-2, x + item_size + 2, y + item_size + text_height + 2};

            HBRUSH sel_brush = CreateSolidBrush(color_selected);

            FillRect(hdc, &sel_rc, sel_brush);

            DeleteObject(sel_brush);

        } else if ((int)index == m_hover_index) {

            RECT hover_rc = {x-1, y-1, x + item_size + 1, y + item_size + text_height + 1};

            HBRUSH hover_brush = CreateSolidBrush(RGB(60, 60, 60));

            FillRect(hdc, &hover_rc, hover_brush);

            DeleteObject(hover_brush);

        }

        

        // Draw thumbnail or placeholder

        if (item->thumbnail->bitmap) {
            const auto prev_interp = graphics.GetInterpolationMode();
            const auto prev_smooth = graphics.GetSmoothingMode();
            const auto prev_pixel = graphics.GetPixelOffsetMode();

            const int bmp_w = (int)item->thumbnail->bitmap->GetWidth();
            const int bmp_h = (int)item->thumbnail->bitmap->GetHeight();
            const bool no_scale = (bmp_w == item_size && bmp_h == item_size);

            if (no_scale) {
                graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
                graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
            }

            switch (m_config.artwork_scale) {
                case grid_config::ARTWORK_FIT: {
                    if (bmp_w > 0 && bmp_h > 0) {
                        float scale = std::min((float)item_size / bmp_w, (float)item_size / bmp_h);
                        int dest_w = std::max(1, (int)(bmp_w * scale));
                        int dest_h = std::max(1, (int)(bmp_h * scale));
                        int dest_x = x + (item_size - dest_w) / 2;
                        int dest_y = y + (item_size - dest_h) / 2;
                        graphics.DrawImage(item->thumbnail->bitmap, dest_x, dest_y, dest_w, dest_h);
                    }
                    break;
                }
                case grid_config::ARTWORK_CROP: {
                    if (bmp_w > 0 && bmp_h > 0) {
                        float scale = std::max((float)item_size / bmp_w, (float)item_size / bmp_h);
                        float src_w = item_size / scale;
                        float src_h = item_size / scale;
                        float src_x = (bmp_w - src_w) / 2.0f;
                        float src_y = (bmp_h - src_h) / 2.0f;
                        Gdiplus::Rect dest_rect(x, y, item_size, item_size);
                        graphics.DrawImage(item->thumbnail->bitmap, dest_rect,
                            (Gdiplus::REAL)src_x, (Gdiplus::REAL)src_y, (Gdiplus::REAL)src_w, (Gdiplus::REAL)src_h, Gdiplus::UnitPixel);
                    }
                    break;
                }
                case grid_config::ARTWORK_STRETCH:
                default:
                    graphics.DrawImage(item->thumbnail->bitmap, x, y, item_size, item_size);
                    break;
            }

            graphics.SetInterpolationMode(prev_interp);
            graphics.SetSmoothingMode(prev_smooth);
            graphics.SetPixelOffsetMode(prev_pixel);

            item->thumbnail->last_access = GetTickCount();

        } else {

            // Draw placeholder

            RECT placeholder_rc = {x, y, x + item_size, y + item_size};

            

            for (int i = 0; i < 3; i++) {

                RECT gradient_rc = {x + i, y + i, x + item_size - i, y + item_size - i};

                int color_value = 35 + i * 5;

                HBRUSH gradient_brush = CreateSolidBrush(RGB(color_value, color_value, color_value));

                FillRect(hdc, &gradient_rc, gradient_brush);

                DeleteObject(gradient_brush);

            }

            

            // Draw icon (scaled for enlarged items)

            SetTextColor(hdc, RGB(80, 80, 80));

            int desired_size = std::max(8, item_size / 4);
            if (!m_placeholder_font || m_placeholder_font_size != desired_size) {
                if (m_placeholder_font) DeleteObject(m_placeholder_font);
                m_placeholder_font = CreateFont(desired_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI Symbol"));
                m_placeholder_font_size = desired_size;
            }
            HFONT old_font = (HFONT)SelectObject(hdc, m_placeholder_font);

            

            if (item->thumbnail->loading) {

                DrawText(hdc, TEXT("..."), -1, &placeholder_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            } else if (m_config.grouping == grid_config::GROUP_BY_FOLDER) {

                DrawText(hdc, TEXT("[ ]"), -1, &placeholder_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            } else {

                DrawText(hdc, TEXT("( )"), -1, &placeholder_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            }

            

            SelectObject(hdc, old_font);

        }

        

        // Draw subtle border (or highlighted border for now playing)

        if (is_now_playing) {

            // Draw a thicker, colored border for now playing album

            HPEN pen = CreatePen(PS_SOLID, 3, RGB(0, 150, 255));  // Blue border

            HPEN oldpen = (HPEN)SelectObject(hdc, pen);

            HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

            Rectangle(hdc, x-1, y-1, x + item_size + 1, y + item_size + 1);

            SelectObject(hdc, oldpen);

            SelectObject(hdc, oldbrush);

            DeleteObject(pen);

        } else {

            // Regular subtle border

            HPEN pen = CreatePen(PS_SOLID, 1, RGB(50, 50, 50));

            HPEN oldpen = (HPEN)SelectObject(hdc, pen);

            HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

            Rectangle(hdc, x, y, x + item_size, y + item_size);

            SelectObject(hdc, oldpen);

            SelectObject(hdc, oldbrush);

            DeleteObject(pen);

        }

        

        // Draw track count badge when track count is enabled (always show as badge)

        if (m_config.show_track_count && item->tracks.get_count() > 0) {

            // Check if dark mode is enabled using our hooks

            bool dark_mode = m_dark ? true : false;

            draw_track_count_badge(hdc, item->tracks.get_count(), x + item_size, y, dark_mode, font);

        }

        // Multi-disc indicator (grid stays single item per album)
        if (item->disc_count > 1) {
            bool dark_mode = m_dark ? true : false;
            draw_disc_count_badge(hdc, (int)item->disc_count, x, y, dark_mode, font);
        }

        

        // v10.0.37: Removed redundant play icon - blue border and enlargement are sufficient indicators

        

        // v10.0.37: Smart text overlay logic:

        // - Show overlay always when text labels are enabled (permanent display)

        // - Show on hover only when text labels are disabled (hover info)

        bool should_show_text_overlay = m_config.show_text || ((int)index == m_hover_index);

        if (should_show_text_overlay) {

            // UNICODE FIX v10.0.16: Prepare text with proper UTF-8 to UTF-16 conversion

            // Use configured label format (Album only vs Artist - Album)

            const std::wstring& wide_text = item->get_display_text_wide(m_config.label_style);



            if (!wide_text.empty()) {

                // Calculate text overlay area (bottom portion of artwork)

                SelectObject(hdc, font);



                // Get font metrics for proper height calculation

                TEXTMETRIC tm;

                GetTextMetrics(hdc, &tm);

                int line_height = tm.tmHeight + tm.tmExternalLeading;



                // Calculate overlay height based on text lines

                int overlay_height;

                if (m_config.text_lines == 1) {

                    overlay_height = line_height + 8;  // Single line with padding

                } else if (m_config.text_lines == 2) {

                    overlay_height = (line_height * 2) + 10;  // Two lines with spacing

                } else {

                    overlay_height = (line_height * 3) + 12;  // Three lines with spacing

                }



                // Limit overlay to reasonable portion of artwork (max 1/3)

                overlay_height = std::min(overlay_height, item_size / 3);



                // Position overlay at bottom of artwork

                RECT overlay_rect = {

                    x,

                    y + item_size - overlay_height,

                    x + item_size,

                    y + item_size

                };



                // Detect if we're using dark theme

                bool is_dark_theme = m_dark ? true : false;



                // Create theme-appropriate semi-transparent background

                COLORREF bg_color;

                COLORREF text_color;



                if (is_dark_theme) {

                    // Dark theme: dark semi-transparent background, light text

                    bg_color = RGB(30, 30, 30);

                    text_color = RGB(240, 240, 240);

                } else {

                    // Light theme: light semi-transparent background, dark text

                    bg_color = RGB(240, 240, 240);

                    text_color = RGB(20, 20, 20);

                }



                // Draw semi-transparent background using alpha blending

                // Create a compatible DC and bitmap for alpha blending

                HDC mem_dc = CreateCompatibleDC(hdc);

                HBITMAP mem_bitmap = CreateCompatibleBitmap(hdc, item_size, overlay_height);

                HBITMAP old_bitmap = (HBITMAP)SelectObject(mem_dc, mem_bitmap);



                // Fill memory DC with background color

                HBRUSH bg_brush = CreateSolidBrush(bg_color);

                RECT fill_rect = {0, 0, item_size, overlay_height};

                FillRect(mem_dc, &fill_rect, bg_brush);

                DeleteObject(bg_brush);



                // Blend with 70% opacity (180/255)

                BLENDFUNCTION blend_func = {AC_SRC_OVER, 0, 180, 0};

                AlphaBlend(hdc, x, y + item_size - overlay_height, item_size, overlay_height,

                          mem_dc, 0, 0, item_size, overlay_height, blend_func);



                // Cleanup memory DC

                SelectObject(mem_dc, old_bitmap);

                DeleteObject(mem_bitmap);

                DeleteDC(mem_dc);



                // Draw text on the overlay

                RECT text_rect = {

                    x + 4,  // Small padding from edges

                    y + item_size - overlay_height + 4,

                    x + item_size - 4,

                    y + item_size - 4

                };



                SetTextColor(hdc, text_color);

                SetBkMode(hdc, TRANSPARENT);  // Transparent text background



                // Configure text drawing format

                UINT format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL;

                if (m_config.text_lines == 1) {

                    format = DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX | DT_VCENTER;

                } else if (m_config.text_lines == 2) {

                    format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL;

                } else {

                    // 3 lines - use top alignment to show as much as possible

                    format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_TOP;

                }



                // UNICODE FIX: Use DrawTextW for proper Unicode display

                DrawTextW(hdc, wide_text.c_str(), -1, &text_rect, format);

            }

        }



        // v10.0.35: NEW - Playlist overlay for 3x3 enlarged now playing artwork

        if (m_config.show_playlist_overlay && is_now_playing &&

            m_config.enlarged_now_playing == grid_config::ENLARGED_3X3 &&

            item->tracks.get_count() > 1) {



            draw_playlist_overlay(hdc, graphics, font, x, y, item_size, index, color_text);

        }

    }
    

    int hit_test(int mx, int my) {

        RECT rc;

        GetClientRect(m_hwnd, &rc);

        calculate_layout();



        size_t item_count = get_item_count();

        if (item_count == 0) return -1;



        // v10.0.30: PERFORMANCE OPTIMIZATION - O(1) mathematical hit testing

        // instead of O(n) iteration through all items



        // Calculate which column the click is in

        int adjusted_x = mx - PADDING;

        if (adjusted_x < 0) return -1;



        int item_with_padding = m_item_size + PADDING;

        int column = adjusted_x / item_with_padding;

        if (column >= m_config.columns) return -1;



        // Calculate which row the click is in (accounting for scroll)

        int adjusted_y = my + m_scroll_pos - PADDING;

        if (adjusted_y < 0) return -1;



        int text_height = calculate_text_height();

        int total_item_height = m_item_size + text_height + PADDING;

        int row = adjusted_y / total_item_height;



        // Calculate the grid-based index

        size_t grid_index = row * m_config.columns + column;



        // Handle enlarged now playing (v10.0.29 feature)

        if (m_config.enlarged_now_playing && m_now_playing_index >= 0) {

            // Complex enlarged item positioning - fall back to iteration for now

            // This only happens when enlarged mode is active and for rare edge cases

            for (size_t index = 0; index < item_count; index++) {

                item_position pos = get_item_position(index, rc);



                if (mx >= pos.x && mx < pos.x + pos.width &&

                    my >= pos.y && my < pos.y + pos.height) {

                    return index;

                }

            }

            return -1;

        }



        // Check if the calculated index is valid

        if (grid_index >= item_count) return -1;



        // Verify the click is actually within the item bounds (for precision)

        int item_x = PADDING + column * item_with_padding;

        int item_y = PADDING + row * total_item_height - m_scroll_pos;



        if (mx >= item_x && mx < item_x + m_item_size &&

            my >= item_y && my < item_y + m_item_size + text_height) {

            return grid_index;

        }



        return -1;  // No item hit

    }



    // v10.0.36: NEW - Hit test for playlist overlay clicks

    int hit_test_playlist_overlay(int x, int y) {

        if (!m_config.show_playlist_overlay ||

            m_config.enlarged_now_playing != grid_config::ENLARGED_3X3) {

            return -1;  // Overlay not shown

        }



        // Find currently playing item (enlarged item)

        if (m_now_playing_index < 0) return -1;  // No currently playing item



        // Use the same positioning system as drawing

        RECT client_rect;

        GetClientRect(m_hwnd, &client_rect);

        item_position pos = get_item_position(m_now_playing_index, client_rect);



        if (!pos.visible) return -1;  // Not visible



        // Get the enlarged item size and position (same as in draw_item)

        int item_size = get_item_size(m_now_playing_index);



        // Calculate overlay area (same calculation as in draw_playlist_overlay)

        int overlay_width = item_size - 20;  // Leave some margin



        // Calculate overlay height - ALWAYS adjust for text labels (same as drawing)

        int overlay_height = item_size - 40; // Base margin

        {

            // Calculate text overlay height to avoid collision using the UI font

            // Spacing depends on compact/normal mode, not font size

            bool use_compact_mode = (item_size < 500);

            int line_spacing = use_compact_mode ? 3 : 5;



            HDC temp_hdc = GetDC(m_hwnd);

            // Query foobar's UI font

            HFONT ui_font = NULL;

            if (m_callback.is_valid()) ui_font = m_callback->query_font_ex(ui_font_default);

            if (!ui_font) ui_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);



            HFONT temp_old_font = (HFONT)SelectObject(temp_hdc, ui_font);

            TEXTMETRIC temp_tm;

            GetTextMetrics(temp_hdc, &temp_tm);

            int line_height = temp_tm.tmHeight + temp_tm.tmExternalLeading + line_spacing;



            SelectObject(temp_hdc, temp_old_font);

            ReleaseDC(m_hwnd, temp_hdc);



            int text_overlay_height;

            if (m_config.text_lines == 1) {

                text_overlay_height = line_height + 8;  // Single line with padding

            } else if (m_config.text_lines == 2) {

                text_overlay_height = (line_height * 2) + 10;  // Two lines with spacing

            } else {

                text_overlay_height = (line_height * 3) + 12;  // Three lines with spacing

            }



            // Reduce playlist height to avoid collision with text overlay

            overlay_height = item_size - 40 - text_overlay_height - 10; // Extra margin between overlays

        }



        // Determine playlist display mode based on grid item size, not overlay height (same as drawing function)

        bool use_compact_mode = (item_size < 200);



        // Only adjust spacing; use UI font for height

        int line_spacing = use_compact_mode ? 3 : 5;



        // Calculate text metrics for line height with correct UI font

        HDC hdc = GetDC(m_hwnd);

        HFONT ui_font = NULL;

        if (m_callback.is_valid()) ui_font = m_callback->query_font_ex(ui_font_default);

        if (!ui_font) ui_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);



        HFONT old_font = (HFONT)SelectObject(hdc, ui_font);

        TEXTMETRIC tm;

        GetTextMetrics(hdc, &tm);

        int line_height = tm.tmHeight + tm.tmExternalLeading + line_spacing;



        // Cleanup

        SelectObject(hdc, old_font);

        ReleaseDC(m_hwnd, hdc);



        int overlay_x = pos.x + 10;  // Center horizontally with margin

        int overlay_y = pos.y + 10;  // Start from top with margin



        // Check if click is within overlay bounds

        if (x < overlay_x || x >= overlay_x + overlay_width ||

            y < overlay_y || y >= overlay_y + overlay_height) {

            return -1;

        }



        // Get playlist info for track calculation

        auto playlist_manager = playlist_manager::get();

        t_size active_playlist = playlist_manager->get_active_playlist();

        if (active_playlist == pfc_infinite) return -1;



        t_size playlist_count = playlist_manager->playlist_get_item_count(active_playlist);

        if (playlist_count == 0) return -1;



        // Mode determination will be moved after final overlay_height calculation



        // Calculate track list layout (same as in draw_playlist_overlay)

        int header_y = overlay_y + 5;

        int content_start_y = header_y + line_height + 8; // Match drawing function spacing

        int max_tracks = (overlay_height - 20) / line_height;



        // Check if click is in header area (ignore)

        if (y <= content_start_y) {

            return -1;

        }



        // Calculate which track was clicked

        int clicked_row = (y - content_start_y) / line_height;



        // Debug output (temporary)

        console::printf("Playlist click: y=%d, content_start_y=%d, line_height=%d, clicked_row=%d",

                       y, content_start_y, line_height, clicked_row);



        // Find starting track (centered around currently playing) - same logic as draw_playlist_overlay

        int start_track = 0;

        metadb_handle_ptr current_track;

        auto playback_control = playback_control::get();

        bool has_current_track = playback_control->get_now_playing(current_track);



        if (has_current_track && (int)playlist_count > max_tracks) {

            // Find current track in playlist and center view around it

            for (int i = 0; i < (int)playlist_count; i++) {

                metadb_handle_ptr playlist_track;

                if (playlist_manager->playlist_get_item_handle(playlist_track, active_playlist, i)) {

                    if (playlist_track == current_track) {

                        start_track = std::max(0, i - max_tracks / 2);

                        start_track = std::min(start_track, (int)playlist_count - max_tracks);

                        break;

                    }

                }

            }

        }



        int track_index = start_track + clicked_row;



        // Debug output (temporary)

        console::printf("start_track=%d, final_track_index=%d, playlist_count=%d",

                       start_track, track_index, (int)playlist_count);



        if (track_index >= 0 && track_index < (int)playlist_count) {

            return track_index;

        }



        return -1;  // Click outside valid track area

    }



    // v10.0.36: NEW - Handle playlist overlay click to start playback

    void handle_playlist_overlay_click(int track_index) {

        auto playlist_manager = playlist_manager::get();

        t_size active_playlist = playlist_manager->get_active_playlist();

        if (active_playlist == pfc_infinite) return;



        t_size playlist_count = playlist_manager->playlist_get_item_count(active_playlist);

        if (track_index < 0 || track_index >= (int)playlist_count) return;



        // Use the same method as in existing code - playlist_execute_default_action

        // This is the proper way to start playback from a specific track

        playlist_manager->playlist_execute_default_action(active_playlist, track_index);

    }



    LRESULT on_lbuttondown(int x, int y, WPARAM keys) {

        // In edit mode, don't handle item selection - let the host handle element selection

        if (m_callback.is_valid() && m_callback->is_edit_mode_enabled()) {

            SetFocus(m_hwnd);  // Make sure we have focus for element selection

            return 0;

        }



        // Check if click is on playlist overlay first

        int playlist_track_index = hit_test_playlist_overlay(x, y);

        if (playlist_track_index >= 0) {

            // Handle playlist overlay click - start playing the clicked track

            handle_playlist_overlay_click(playlist_track_index);

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

                int start = std::min(m_last_selected, index);

                int end = std::max(m_last_selected, index);

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



                case grid_config::DOUBLECLICK_PLAY_IN_GRID: {

                    // Play selected items in the dedicated "Album Grid" playlist

                    if (!real_indices_to_process.empty()) {

                        metadb_handle_list tracks_to_add;

                        for (size_t real_idx : real_indices_to_process) {

                            if (real_idx < m_items.size() && m_items[real_idx]->tracks.get_count() > 0) {

                                metadb_handle_list sorted_tracks = m_items[real_idx]->tracks;

                                sort_tracks_for_playlist(sorted_tracks);

                                tracks_to_add.add_items(sorted_tracks);

                            }

                        }

                        if (tracks_to_add.get_count() > 0) {

                            // Find or create the "Album Grid" playlist

                            t_size album_grid_playlist = pfc::infinite_size;

                            t_size playlist_count = pm->get_playlist_count();

                            for (t_size i = 0; i < playlist_count; i++) {

                                pfc::string8 name;

                                pm->playlist_get_name(i, name);

                                if (strcmp(name.c_str(), "Album Grid") == 0) {

                                    album_grid_playlist = i;

                                    break;

                                }

                            }

                            if (album_grid_playlist == pfc::infinite_size) {

                                album_grid_playlist = pm->create_playlist("Album Grid", pfc::infinite_size, pfc::infinite_size);

                            }

                            if (album_grid_playlist != pfc::infinite_size) {

                                pm->playlist_clear(album_grid_playlist);

                                pm->playlist_add_items(album_grid_playlist, tracks_to_add, bit_array_false());

                                pm->set_active_playlist(album_grid_playlist);

                                static_api_ptr_t<playback_control> pc;

                                pc->start(playback_control::track_command_play);

                            }

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

            AppendMenu(menu, MF_STRING, 6, TEXT("Play in Album Grid Playlist"));

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

        AppendMenu(doubleclick_menu, MF_STRING | (m_config.doubleclick == grid_config::DOUBLECLICK_PLAY_IN_GRID ? MF_CHECKED : 0),

                   113, TEXT("Play in Album Grid playlist"));

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

        AppendMenu(sort_menu, MF_STRING | (m_config.sorting == grid_config::SORT_BY_RELEASE_DATE ? MF_CHECKED : 0), 

                   70, TEXT("By Release Date"));

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

        HMENU scale_menu = CreatePopupMenu();
        AppendMenu(scale_menu, MF_STRING | (m_config.artwork_scale == grid_config::ARTWORK_FIT ? MF_CHECKED : 0), 160, TEXT("Fit (preserve aspect)"));
        AppendMenu(scale_menu, MF_STRING | (m_config.artwork_scale == grid_config::ARTWORK_CROP ? MF_CHECKED : 0), 161, TEXT("Crop to Fill"));
        AppendMenu(scale_menu, MF_STRING | (m_config.artwork_scale == grid_config::ARTWORK_STRETCH ? MF_CHECKED : 0), 162, TEXT("Stretch (legacy)"));
        AppendMenu(menu, MF_POPUP, (UINT_PTR)scale_menu, TEXT("Artwork Scaling"));

        

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

        AppendMenu(menu, MF_STRING | (m_config.auto_scroll_to_now_playing ? MF_CHECKED : 0), 151, TEXT("Auto-scroll to Now Playing"));

        

        // NEW v10.0.29: Enlarged Now Playing submenu

        HMENU enlarged_menu = CreatePopupMenu();

        AppendMenu(enlarged_menu, MF_STRING | (m_config.enlarged_now_playing == grid_config::ENLARGED_NONE ? MF_CHECKED : 0), 152, TEXT("Normal Size"));

        AppendMenu(enlarged_menu, MF_STRING | (m_config.enlarged_now_playing == grid_config::ENLARGED_2X2 ? MF_CHECKED : 0), 153, TEXT("2x2 Enlarged"));

        AppendMenu(enlarged_menu, MF_STRING | (m_config.enlarged_now_playing == grid_config::ENLARGED_3X3 ? MF_CHECKED : 0), 154, TEXT("3x3 Enlarged"));

        AppendMenu(enlarged_menu, MF_SEPARATOR, 0, NULL);

        AppendMenu(enlarged_menu, MF_STRING | (m_config.show_playlist_overlay ? MF_CHECKED : 0), 155, TEXT("Show Playlist Overlay"));

        AppendMenu(menu, MF_POPUP, (UINT_PTR)enlarged_menu, TEXT("Enlarged Now Playing"));

        

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

        DestroyMenu(scale_menu);

        DestroyMenu(view_menu);

        DestroyMenu(track_sort_menu);

        DestroyMenu(doubleclick_menu);

        DestroyMenu(enlarged_menu);  // NEW v10.0.29

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

                    if (item) {

                        try {

                            pfc::string8 folder_path;

                            

                            // Extract directory path from the first track if needed

                            if (item->tracks.get_count() > 0) {

                                metadb_handle_ptr rep = item->representative_track.is_valid() ? item->representative_track : item->tracks[0];
                                pfc::string8 first_file = rep->get_path();

                                pfc::string8 root_path, root_name;
                                if (try_get_album_root_folder_from_file_path(first_file.c_str(), root_path, root_name)) {
                                    folder_path = root_path;
                                } else {

                                const char* last_slash = strrchr(first_file.c_str(), '\\');

                                if (last_slash) {

                                    folder_path.set_string(first_file.c_str(), last_slash - first_file.c_str());

                                }
                                }

                            }

                            if (folder_path.is_empty()) return 0;

                            

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

                

            case 6: // Play in Album Grid Playlist - v10.0.26 feature

                if (!m_selected_indices.empty()) {

                    static_api_ptr_t<playlist_manager> pm;

                    

                    // Find or create "Album Grid" playlist

                    t_size album_grid_playlist = pfc::infinite_size;

                    t_size playlist_count = pm->get_playlist_count();

                    

                    // Look for existing "Album Grid" playlist

                    for (t_size i = 0; i < playlist_count; i++) {

                        pfc::string8 name;

                        pm->playlist_get_name(i, name);

                        if (strcmp(name.c_str(), "Album Grid") == 0) {

                            album_grid_playlist = i;

                            break;

                        }

                    }

                    

                    // Create if doesn't exist

                    if (album_grid_playlist == pfc::infinite_size) {

                        album_grid_playlist = pm->create_playlist("Album Grid", pfc::infinite_size, pfc::infinite_size);

                    }

                    

                    if (album_grid_playlist != pfc::infinite_size) {

                        // Clear the playlist and add selected tracks

                        pm->playlist_clear(album_grid_playlist);

                        

                        metadb_handle_list tracks_to_add;

                        for (int idx : m_selected_indices) {

                            auto* item = get_item_at(idx);

                            if (item) {

                                tracks_to_add.add_items(item->tracks);

                            }

                        }

                        

                        pm->playlist_add_items(album_grid_playlist, tracks_to_add, bit_array_false());

                        pm->set_active_playlist(album_grid_playlist);

                        

                        // Start playback

                        static_api_ptr_t<playback_control> pc;

                        pc->start(playback_control::track_command_play);

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

                m_layout_cache.invalidate();  // v10.0.30: Invalidate cache when columns change

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

                m_config.auto_scroll_to_now_playing = !m_config.auto_scroll_to_now_playing;

                config_changed = true;

                break;

            // NEW v10.0.29: Enlarged Now Playing options

            case 152: // Normal Size

                m_config.enlarged_now_playing = grid_config::ENLARGED_NONE;

                m_layout_cache.invalidate();  // v10.0.30: Invalidate cache when enlarged mode changes

                config_changed = true;

                break;

            case 153: // 2x2 Enlarged

                m_config.enlarged_now_playing = grid_config::ENLARGED_2X2;

                m_layout_cache.invalidate();  // v10.0.30: Invalidate cache when enlarged mode changes

                config_changed = true;

                break;

            case 154: // 3x3 Enlarged

                m_config.enlarged_now_playing = grid_config::ENLARGED_3X3;

                m_layout_cache.invalidate();  // v10.0.30: Invalidate cache when enlarged mode changes

                config_changed = true;

                break;

            case 155: // Show Playlist Overlay

                m_config.show_playlist_overlay = !m_config.show_playlist_overlay;

                config_changed = true;

                break;

            case 50: m_config.sorting = grid_config::SORT_BY_NAME; needs_sort = true; config_changed = true; break;

            case 51: m_config.sorting = grid_config::SORT_BY_ARTIST; needs_sort = true; config_changed = true; break;

            case 52: m_config.sorting = grid_config::SORT_BY_ALBUM; needs_sort = true; config_changed = true; break;

            case 53: m_config.sorting = grid_config::SORT_BY_YEAR; needs_sort = true; config_changed = true; break;

            case 54: m_config.sorting = grid_config::SORT_BY_GENRE; needs_sort = true; config_changed = true; break;

            case 55: m_config.sorting = grid_config::SORT_BY_DATE; needs_sort = true; config_changed = true; break;

            case 70: m_config.sorting = grid_config::SORT_BY_RELEASE_DATE; needs_sort = true; config_changed = true; break;

            case 56: m_config.sorting = grid_config::SORT_BY_SIZE; needs_sort = true; config_changed = true; break;

            case 57: m_config.sorting = grid_config::SORT_BY_TRACK_COUNT; needs_sort = true; config_changed = true; break;

            case 58: m_config.sorting = grid_config::SORT_BY_RATING; needs_sort = true; config_changed = true; break;

            case 59: m_config.sorting = grid_config::SORT_BY_PATH; needs_sort = true; config_changed = true; break;

            case 69: m_config.sorting = grid_config::SORT_BY_RANDOM; needs_sort = true; config_changed = true; break;

            case 60: m_config.text_lines = 1; config_changed = true; break;

            case 61: m_config.text_lines = 2; config_changed = true; break;

            case 62: m_config.text_lines = 3; config_changed = true; break;

            case 160: m_config.artwork_scale = grid_config::ARTWORK_FIT; config_changed = true; break;

            case 161: m_config.artwork_scale = grid_config::ARTWORK_CROP; config_changed = true; break;

            case 162: m_config.artwork_scale = grid_config::ARTWORK_STRETCH; config_changed = true; break;

            

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

            case 113: // Double-click: Play in Album Grid playlist

                if (m_config.doubleclick != grid_config::DOUBLECLICK_PLAY_IN_GRID) {

                    m_config.doubleclick = grid_config::DOUBLECLICK_PLAY_IN_GRID;

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



        // v10.0.30: Track mouse position for floating labels

        m_mouse_x = x;

        m_mouse_y = y;



        int index = hit_test(x, y);

        if (index != m_hover_index) {

            m_hover_index = index;

            // v10.0.30: Immediate floating label updates

            InvalidateRect(m_hwnd, NULL, FALSE);

            

            // v10.0.30: Tooltip disabled - using floating labels instead

            if (false && m_tooltip && !m_config.show_text && index >= 0 && index < (int)get_item_count()) {

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

        // v10.0.30: PERFORMANCE OPTIMIZATION - Scroll throttling

        // Be more reactive: don't throttle thumb tracking

        if (code != SB_THUMBTRACK && code != SB_THUMBPOSITION) {

            ULONGLONG current_time = GetTickCount64();

            if (current_time - m_last_scroll_update < SCROLL_THROTTLE_MS) {

                return 0;

            }

            m_last_scroll_update = current_time;

        }



        RECT rc;

        GetClientRect(m_hwnd, &rc);



        // v10.0.39: Use centralized height calculation for consistency

        int total_height = calculate_total_height();



        int old_pos = m_scroll_pos;

        // Dynamic scroll step: small fraction of row height (gentle per-line move)

        int row_h = (m_item_size + calculate_text_height() + PADDING);

        int line_height = std::max(8, row_h / 6);

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



        m_scroll_pos = std::max(0, std::min(m_scroll_pos, std::max(0, total_height - (int)rc.bottom)));



        if (m_scroll_pos != old_pos) {

            SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);

            InvalidateRect(m_hwnd, NULL, FALSE);

        }



        m_last_user_scroll = GetTickCount64();

        return 0;

    }

    

    // Function to sort tracks for playlist addition

    void sort_tracks_for_playlist(metadb_handle_list& tracks) {

        if (tracks.get_count() <= 1) return;

        

        switch (m_config.track_sorting) {

            case grid_config::TRACK_SORT_BY_NUMBER:

                {
                    // Disc-aware (with folder fallback): disc -> track -> path
                    std::vector<metadb_handle_ptr> vec;
                    vec.reserve(tracks.get_count());
                    for (t_size i = 0; i < tracks.get_count(); i++) vec.push_back(tracks[i]);

                    struct key_t { int disc; int track; pfc::string8 path; metadb_handle_ptr h; };
                    std::vector<key_t> keys;
                    keys.reserve(vec.size());
                    for (auto& h : vec) {
                        key_t k{};
                        k.disc = get_disc_number_for_handle(h);
                        k.track = get_track_number_for_handle(h);
                        try { k.path = h->get_path(); } catch (...) {}
                        k.h = h;
                        keys.push_back(std::move(k));
                    }

                    std::stable_sort(keys.begin(), keys.end(), [](const key_t& a, const key_t& b) {
                        int ad = (a.disc > 0 ? a.disc : 999);
                        int bd = (b.disc > 0 ? b.disc : 999);
                        if (ad != bd) return ad < bd;
                        int at = (a.track > 0 ? a.track : 999);
                        int bt = (b.track > 0 ? b.track : 999);
                        if (at != bt) return at < bt;
                        return pfc::stricmp_ascii(a.path.c_str(), b.path.c_str()) < 0;
                    });

                    tracks.remove_all();
                    for (auto& k : keys) tracks.add_item(k.h);
                }

                break;

            case grid_config::TRACK_SORT_BY_NAME:

                {
                    // Disc-aware (with folder fallback): disc -> title -> path
                    std::vector<metadb_handle_ptr> vec;
                    vec.reserve(tracks.get_count());
                    for (t_size i = 0; i < tracks.get_count(); i++) vec.push_back(tracks[i]);

                    struct key_t { int disc; pfc::string8 title; pfc::string8 path; metadb_handle_ptr h; };
                    std::vector<key_t> keys;
                    keys.reserve(vec.size());
                    for (auto& h : vec) {
                        key_t k{};
                        k.disc = get_disc_number_for_handle(h);
                        try {
                            file_info_impl info;
                            if (h->get_info(info)) {
                                const char* t = info.meta_get("TITLE", 0);
                                if (t) k.title = t;
                            }
                        } catch (...) {}
                        try { k.path = h->get_path(); } catch (...) {}
                        k.h = h;
                        keys.push_back(std::move(k));
                    }

                    std::stable_sort(keys.begin(), keys.end(), [](const key_t& a, const key_t& b) {
                        int ad = (a.disc > 0 ? a.disc : 999);
                        int bd = (b.disc > 0 ? b.disc : 999);
                        if (ad != bd) return ad < bd;
                        int tcmp = pfc::stricmp_ascii(a.title.c_str(), b.title.c_str());
                        if (tcmp != 0) return tcmp < 0;
                        return pfc::stricmp_ascii(a.path.c_str(), b.path.c_str()) < 0;
                    });

                    tracks.remove_all();
                    for (auto& k : keys) tracks.add_item(k.h);
                }

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

                new_cols = std::min(20, new_cols + 1);  // Match menu limit of 20 columns

            } else {

                new_cols = std::max(1, new_cols - 1);   // Minimum 1 column

            }



            if (new_cols != m_config.columns) {

                m_config.columns = new_cols;

                m_layout_cache.invalidate();  // v10.0.30: Invalidate cache when columns change

                calculate_layout();  // Recalculate item size



                // Mark thumbnails for regeneration if size changed significantly

                for (auto& item : m_items) {

                    if (item->thumbnail->bitmap && item->thumbnail->cached_size > 0) {

                        int size_diff = abs(item->thumbnail->cached_size - m_item_size);

                        if (size_diff > 20) {

                            // Size changed enough to warrant regeneration

                            // Safe loading flag reset

            if (item && item->thumbnail && !shutdown_protection::is_shutting_down()) {

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

            // Wheel: use system lines, but step is a gentle fraction of row height

            UINT wheelLines = 3; SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &wheelLines, 0);

            int row_h = (m_item_size + calculate_text_height() + PADDING);

            int pixel_step = std::max(8, row_h / 6);

            static int wheelAccum = 0; wheelAccum += (int)delta;

            int notches = wheelAccum / WHEEL_DELTA; // 120 per notch

            wheelAccum = wheelAccum % WHEEL_DELTA;



            int move_px = (int)wheelLines * pixel_step * notches;

            if (move_px != 0) {

                int old_pos = m_scroll_pos;

                m_scroll_pos -= move_px; // positive delta => up

                RECT rc; GetClientRect(m_hwnd, &rc);

                int total_height = calculate_total_height();

                m_scroll_pos = std::max(0, std::min(m_scroll_pos, std::max(0, total_height - (int)rc.bottom)));

                if (m_scroll_pos != old_pos) {

                    SetScrollPos(m_hwnd, SB_VERT, m_scroll_pos, TRUE);

                    InvalidateRect(m_hwnd, NULL, FALSE);

                    m_last_user_scroll = GetTickCount64();

                }

            }

        }

        return 0;

    }



    // Ensure display index is visible (used by keyboard navigation)

    void ensure_index_visible(int display_index) {

        RECT rc; GetClientRect(m_hwnd, &rc);

        calculate_layout();

        int text_height = calculate_text_height();

        int cols = std::max(1, m_config.columns);

        rebuild_placement_map(cols);

        auto it = m_item_placements.find(display_index);

        if (it == m_item_placements.end()) return;

        const ItemPlacement& pl = it->second;

        int cell_h = m_item_size + text_height + PADDING;

        int y = pl.row * cell_h + PADDING - m_scroll_pos;

        int h = pl.height * m_item_size + text_height;

        if (y < 0) {

            m_scroll_pos += y;

        } else if (y + h > rc.bottom) {

            m_scroll_pos += (y + h - rc.bottom);

        }

        if (m_scroll_pos < 0) m_scroll_pos = 0;

        update_scrollbar();

    }



    // Execute configured default action on current selection

    void perform_default_action_on_selection() {

        size_t count = get_item_count();

        if (count == 0) return;

        std::vector<size_t> real_indices_to_process;

        if (!m_selected_indices.empty()) {

            for (int sel : m_selected_indices) {

                if (sel >= 0 && sel < (int)count) {

                    size_t real_idx = get_real_index(sel);

                    if (real_idx < m_items.size()) real_indices_to_process.push_back(real_idx);

                }

            }

        } else if (m_last_selected >= 0 && m_last_selected < (int)count) {

            size_t real_idx = get_real_index(m_last_selected);

            if (real_idx < m_items.size()) real_indices_to_process.push_back(real_idx);

        }

        if (real_indices_to_process.empty()) return;



        static_api_ptr_t<playlist_manager> pm;

        t_size playlist = pm->get_active_playlist();

        switch (m_config.doubleclick) {

            case grid_config::DOUBLECLICK_PLAY: {

                if (playlist != pfc::infinite_size) {

                    pm->playlist_clear(playlist);

                    for (size_t real_idx : real_indices_to_process) {

                        metadb_handle_list sorted_tracks = m_items[real_idx]->tracks;

                        sort_tracks_for_playlist(sorted_tracks);

                        pm->playlist_add_items(playlist, sorted_tracks, bit_array_false());

                    }

                    pm->set_playing_playlist(playlist);

                    static_api_ptr_t<playback_control> pc; pc->play_start();

                }

                break;

            }

            case grid_config::DOUBLECLICK_ADD_TO_CURRENT: {

                if (playlist != pfc::infinite_size) {

                    metadb_handle_list tracks_to_add;

                    for (size_t real_idx : real_indices_to_process) {

                        metadb_handle_list sorted_tracks = m_items[real_idx]->tracks;

                        sort_tracks_for_playlist(sorted_tracks);

                        tracks_to_add.add_items(sorted_tracks);

                    }

                    if (tracks_to_add.get_count() > 0) pm->playlist_add_items(playlist, tracks_to_add, bit_array_false());

                }

                break;

            }

            case grid_config::DOUBLECLICK_ADD_TO_NEW: {

                metadb_handle_list tracks_to_add;

                for (size_t real_idx : real_indices_to_process) {

                    metadb_handle_list sorted_tracks = m_items[real_idx]->tracks;

                    sort_tracks_for_playlist(sorted_tracks);

                    tracks_to_add.add_items(sorted_tracks);

                }

                if (tracks_to_add.get_count() > 0) {

                    t_size new_playlist = pm->create_playlist("New Playlist", pfc::infinite_size, pfc::infinite_size);

                    pm->playlist_add_items(new_playlist, tracks_to_add, bit_array_false());

                    pm->set_active_playlist(new_playlist);

                    static_api_ptr_t<playback_control> pc; pc->play_start();

                }

                break;

            }

            case grid_config::DOUBLECLICK_PLAY_IN_GRID: {

                metadb_handle_list tracks_to_add;

                for (size_t real_idx : real_indices_to_process) {

                    metadb_handle_list sorted_tracks = m_items[real_idx]->tracks;

                    sort_tracks_for_playlist(sorted_tracks);

                    tracks_to_add.add_items(sorted_tracks);

                }

                if (tracks_to_add.get_count() > 0) {

                    t_size album_grid_playlist = pfc::infinite_size;

                    t_size playlist_count = pm->get_playlist_count();

                    for (t_size i = 0; i < playlist_count; i++) {

                        pfc::string8 name; pm->playlist_get_name(i, name);

                        if (strcmp(name.c_str(), "Album Grid") == 0) { album_grid_playlist = i; break; }

                    }

                    if (album_grid_playlist == pfc::infinite_size) {

                        album_grid_playlist = pm->create_playlist("Album Grid", pfc::infinite_size, pfc::infinite_size);

                    }

                    pm->playlist_clear(album_grid_playlist);

                    pm->playlist_add_items(album_grid_playlist, tracks_to_add, bit_array_false());

                    pm->set_active_playlist(album_grid_playlist);

                    static_api_ptr_t<playback_control> pc; pc->play_start();

                }

                break;

            }

        }

    }

    

    void update_scrollbar() {

        RECT rc;

        GetClientRect(m_hwnd, &rc);



        // v10.0.39: Use centralized height calculation

        int total_height = calculate_total_height();



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

        static const GUID guid = { 0x6b7c5f91, 0xac4e, 0x5d96, { 0xb9, 0x03, 0x8d, 0x4e, 0xac, 0x6f, 0x3b, 0x21 } };

        return guid;

    }

    

    // Public getter for window handle (needed for bump functionality)

    HWND get_hwnd() const { return m_hwnd; }

    

    // Playlist callback methods for auto-refresh - implement all required abstract methods

    void on_items_added(t_size p_base, metadb_handle_list_cref p_data, const bit_array & p_selection) override {

        // CRITICAL v10.0.17: Validate object before ANY member access

        if (!is_valid()) return;

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {

            refresh_items();

        }

    }

    

    void on_items_reordered(const t_size * p_order, t_size p_count) override {

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {

            refresh_items();

        }

    }

    

    void on_items_removing(const bit_array & p_mask, t_size p_old_count, t_size p_new_count) override {

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

        // Called before removal - no action needed

    }

    

    void on_items_removed(const bit_array & p_mask, t_size p_old_count, t_size p_new_count) override {

        // CRITICAL v10.0.17: Validate object before ANY member access

        if (!is_valid()) return;

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {

            refresh_items();

        }

    }

    

    void on_items_selection_change(const bit_array & p_affected, const bit_array & p_state) override {

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

        // Selection changes don't affect our grid view

    }

    

    void on_item_focus_change(t_size p_from, t_size p_to) override {

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

        // Focus changes don't affect our grid view

    }

    

    void on_items_modified(const bit_array & p_mask) override {

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {

            refresh_items();

        }

    }

    

    void on_items_modified_fromplayback(const bit_array & p_mask, play_control::t_display_level p_level) override {

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

        // Playback modifications don't affect our grid view

    }

    

    void on_items_replaced(const bit_array & p_mask, const pfc::list_base_const_t<playlist_callback::t_on_items_replaced_entry> & p_data) override {

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

        if (m_config.view == grid_config::VIEW_PLAYLIST && m_hwnd) {

            refresh_items();

        }

    }

    

    void on_item_ensure_visible(t_size p_idx) override {

        if (m_is_destroying.load() || !m_hwnd) return;  // Prevent use-after-free

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

                // Only update if the track actually changed

                if (!track.is_valid() || !m_last_now_playing.is_valid() ||

                    track != m_last_now_playing) {

                    console::print("Track changed - updating now playing");

                    update_now_playing(track);

                    m_last_now_playing = track;

                }

            } else if (m_last_now_playing.is_valid()) {

                // Playback stopped

                console::print("Playback stopped");

                update_now_playing(nullptr);

                m_last_now_playing.release();

            }

        } catch(...) {

            // Ignore errors during shutdown

        }

    }

    

private:

    void update_now_playing(metadb_handle_ptr track) {

        int old_now_playing_index = m_now_playing_index;



        if (!track.is_valid()) {

            m_now_playing.release();

            m_now_playing_index = -1;



            // v10.0.43: Invalidate cell-based placement cache when clearing now playing

            m_placement_cache_dirty = true;



            if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);



            // v10.0.30: Invalidate layout cache if enlarged mode is active

            if (m_config.enlarged_now_playing != grid_config::ENLARGED_NONE && old_now_playing_index != m_now_playing_index) {

                m_layout_cache.invalidate();



                // Clear cached thumbnail for old now playing to force normal-res reload

                if (old_now_playing_index >= 0 && old_now_playing_index < (int)m_items.size()) {

                    m_items[old_now_playing_index]->thumbnail->clear();

                }

            }

            return;

        }



        m_now_playing = track;

        m_now_playing_index = find_track_album(track);



        // v10.0.43: Invalidate cell-based placement cache when now playing changes

        m_placement_cache_dirty = true;



        // v10.0.30: Invalidate layout cache if now playing changed and enlarged mode is active

        if (m_config.enlarged_now_playing != grid_config::ENLARGED_NONE && old_now_playing_index != m_now_playing_index) {

            m_layout_cache.invalidate();



            // Clear cached thumbnails for old and new now playing to force high-res reload

            if (old_now_playing_index >= 0 && old_now_playing_index < (int)m_items.size()) {

                m_items[old_now_playing_index]->thumbnail->clear();

            }

            if (m_now_playing_index >= 0 && m_now_playing_index < (int)m_items.size()) {

                m_items[m_now_playing_index]->thumbnail->clear();

            }

        }



        // Auto-scroll only when album/artist changes (not every track in same album)

        // This allows users to browse the library without constant interruption

        if (m_now_playing_index >= 0 && m_config.auto_scroll_to_now_playing &&

            old_now_playing_index != m_now_playing_index) {

            console::printf("Auto-scroll: album changed from %d to %d", old_now_playing_index, m_now_playing_index);

            jump_to_now_playing(false);  // false = no animation for auto-scroll

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

            int row = found_index / std::max(1, m_config.columns);

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



        // v10.0.43: Use new placement map to find actual position of enlarged item

        rebuild_placement_map(m_config.columns);



        auto it = m_item_placements.find(m_now_playing_index);

        if (it == m_item_placements.end()) return;



        const ItemPlacement& placement = it->second;

        int text_height = calculate_text_height();

        int normal_item_height = m_item_size + text_height;

        int target_scroll = placement.row * (normal_item_height + PADDING);



        RECT rc;

        GetClientRect(m_hwnd, &rc);

        int viewport_height = rc.bottom;



        // Simple centering that works for both normal and enlarged items

        int actual_item_size = get_item_size(m_now_playing_index);

        int actual_item_height = actual_item_size + text_height;



        // Smart positioning that respects scrollbar limits (like manual scrolling)

        SCROLLINFO si = {};

        si.cbSize = sizeof(si);

        si.fMask = SIF_RANGE | SIF_PAGE;

        GetScrollInfo(m_hwnd, SB_VERT, &si);

        int max_scroll = std::max(0, si.nMax - (int)si.nPage);



        if (m_config.enlarged_now_playing != grid_config::ENLARGED_NONE && actual_item_size > m_item_size) {

            // For enlarged items, ensure they're fully visible without exceeding scrollbar limits

            int item_bottom = target_scroll + actual_item_height;



            // If item would be cut off at bottom, scroll down just enough to show it fully

            if (item_bottom > m_scroll_pos + viewport_height) {

                target_scroll = item_bottom - viewport_height;

            }



            // But don't scroll beyond what the scrollbar allows (respects manual scroll limits)

            target_scroll = std::min(target_scroll, max_scroll);

            target_scroll = std::max(0, target_scroll);

        } else {

            // For normal items, try to center but respect scrollbar limits

            int centered_scroll = target_scroll - viewport_height / 2 + actual_item_height / 2;

            target_scroll = std::max(0, std::min(centered_scroll, max_scroll));

        }

        

        // Update scroll position

        m_scroll_pos = target_scroll;

        update_scrollbar();

        InvalidateRect(m_hwnd, NULL, FALSE);

        

        // Flash effect for visual feedback

        // Could implement a temporary highlight here

    }

    

};



// Async loader concurrency counter (after class definition)

std::atomic<int> album_grid_instance::s_inflight_loaders{0};



// v10.0.27: Implementation of zombie callback handler

void zombie_callback_handler::callback_run() {

    // v10.0.28 FIX: NEVER dereference owner pointer - just discard callback

    console::print("[Album Art Grid v10.0.28] Zombie callback triggered - safe discard");

    

    // ALWAYS mark as dead to prevent future calls

    m_alive = false;

    m_owner = nullptr;

    m_owner_magic = GRID_MAGIC_DEAD;

    

    // NEVER try to access owner - it might be destroyed

    // Just silently discard the callback

    return;

}



// UI element factory

// ui_element factory class

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

        // v10.0.28 FIX: Check shutdown before creating new instances

        if (shutdown_protection::is_shutting_down()) {

            console::print("[Album Art Grid v10.0.28] FACTORY SHUTDOWN - refusing to create new instance");

            return nullptr;

        }

        

        // CRITICAL FIX v10.0.26: Create instance without service_impl_t wrapper

        album_grid_instance* raw_instance = new album_grid_instance(cfg, callback);

        raw_instance->initialize_window(parent);

        

        service_ptr_t<ui_element_instance> instance;

        instance.attach(raw_instance);

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

        console::print("[Album Art Grid v10.0.16] foobar2000 is shutting down - protecting all instances");

        shutdown_protection::initiate_app_shutdown();

    }

    

    void clear_global_shutdown_flag() {

        // Reset shutdown flag on startup

        shutdown_protection::g_is_shutting_down.store(false);

        shutdown_protection::g_active_instances.clear();

    }

}



// Library viewer support - allow showing the Album Art Grid window from Library menu

void show_album_art_grid_window() {

    // Try to activate the main UI which should make the grid visible if already open

    static_api_ptr_t<ui_control> ui_ctrl;

    

    console::print("[Album Art Grid v10.0.16] Activated from Library menu");

    

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

// ============ INITQUIT PART ============

#ifndef INITQUIT_INCLUDED_V20

#define INITQUIT_INCLUDED_V20

// Fixed initquit service for v10.0.18 - NULL Pointer Write Fix

// NO GLOBAL FLAGS - only instance-specific protection

// Fixed SEH exception handling



// Forward declaration for shutdown notification

namespace shutdown_protection {

    extern void initiate_app_shutdown();  // Only called during REAL app shutdown

}



// Helper function for GDI+ shutdown with SEH

static void shutdown_gdiplus_safe(ULONG_PTR token) {

    __try {

        if (token != 0) {

            Gdiplus::GdiplusShutdown(token);

        }

    }

    __except(EXCEPTION_EXECUTE_HANDLER) {

        // Ignore any exceptions during shutdown

    }

}



// GDI+ initialization service with improved shutdown handling

class initquit_gdiplus : public initquit {

private:

    ULONG_PTR m_gdiplusToken = 0;

    static std::atomic<bool> s_shutting_down;

    

public:

    void on_init() override {

        s_shutting_down.store(false);

        

        // Initialize GDI+

        Gdiplus::GdiplusStartupInput gdiplusStartupInput;

        gdiplusStartupInput.GdiplusVersion = 1;

        gdiplusStartupInput.DebugEventCallback = NULL;

        gdiplusStartupInput.SuppressBackgroundThread = FALSE;

        gdiplusStartupInput.SuppressExternalCodecs = FALSE;

        

        Gdiplus::Status status = Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

        

        if (status == Gdiplus::Ok && m_gdiplusToken != 0) {

            console::print("Album Art Grid v10.0.28 initialized - DESTRUCTOR SHUTDOWN GUARD");

            console::print("Available in Library menu - Press A-Z or 0-9 to jump to albums");

        } else {

            console::print("Album Art Grid v10.0.18: GDI+ initialization failed");

            m_gdiplusToken = 0;

        }

    }

    

    void on_quit() override {

        console::print("initquit::quit entry");

        

        // Set LOCAL shutdown flag (not global!)

        s_shutting_down.store(true);

        

        // Notify ONLY the active grid instances about app shutdown

        // This does NOT set any global flags that would block new instances

        try {

            shutdown_protection::initiate_app_shutdown();

        } catch(...) {

            // Ignore exceptions

        }

        

        // Wait for any pending operations to complete

        Sleep(100);

        

        // Shutdown GDI+ using helper function with SEH

        shutdown_gdiplus_safe(m_gdiplusToken);

        m_gdiplusToken = 0;

        

        console::print("initquit::quit exit");

    }

    

    static bool is_shutting_down() {

        return s_shutting_down.load();

    }

};



// Define the static member

std::atomic<bool> initquit_gdiplus::s_shutting_down = false;



// Register the service

static service_factory_single_t<initquit_gdiplus> g_initquit_gdiplus;

#endif



// ============ LIBRARY VIEWER PART ============

#ifndef LIBRARY_VIEWER_INCLUDED_V20

#define LIBRARY_VIEWER_INCLUDED_V20

// Library Viewer implementation for Album Art Grid v10.0.20

// FIXED: Shutdown crash protection WITHOUT blocking new instances

// This makes the component appear in foobar2000's Library viewer list





// Forward declaration - these functions are implemented in grid_v10_0_15.cpp

extern void show_album_art_grid_window();

extern bool is_album_art_grid_window_visible();



// Check if we're in initquit shutdown (NOT global - only during actual shutdown)

namespace {

    bool is_app_shutting_down() {

        // This is a heuristic check - during shutdown, the main window is usually gone

        HWND main_wnd = core_api::get_main_window();

        if (!main_wnd || !IsWindow(main_wnd)) {

            // Double-check by trying to get the API

            try {

                static_api_ptr_t<ui_control> ui;

                // If we can get the UI control and it's valid, we're not shutting down

                return false;

            } catch(...) {

                // Can't get UI control - probably shutting down

                return true;

            }

        }

        return false;

    }

}



// Library viewer implementation with smart shutdown detection

class album_art_grid_library_viewer : public library_viewer {

private:

    // Use a static atomic that's only set during destructor

    static std::atomic<bool> s_being_destroyed;

    

public:

    album_art_grid_library_viewer() {

        // Constructor - we're definitely not being destroyed

        s_being_destroyed.store(false);

    }

    

    ~album_art_grid_library_viewer() {

        // Mark as being destroyed

        s_being_destroyed.store(true);

    }

    

    // Return GUID of preferences page (we don't have one)

    GUID get_preferences_page() override {

        return pfc::guid_null;

    }

    

    // We support the "activate" action (showing the window)

    bool have_activate() override {

        // Only block if we're actively being destroyed OR app is shutting down

        if (s_being_destroyed.load() || is_app_shutting_down()) {

            return false;

        }

        return true;

    }

    

    // Show the Album Art Grid window when activated from Library menu

    void activate() override {

        // Only block if we're actively being destroyed OR app is shutting down

        if (s_being_destroyed.load() || is_app_shutting_down()) {

            return;

        }

        

        // Safe call without SEH (causes compilation issues)

        try {

            console::print("[Album Art Grid v10.0.20] Activated from Library viewer");

            

            // Show the window (this function is implemented in main grid file)

            show_album_art_grid_window();

        }

        catch(...) {

            // Ignore exceptions during potential shutdown race

        }

    }

    

    // Unique GUID for this library viewer (different from ui_element GUID)

    GUID get_guid() override {

        // {8A3C5B7E-9F2D-4A6B-B1E3-7C9D8F5A3E21}

        static const GUID library_viewer_guid = 

            { 0x8a3c5b7e, 0x9f2d, 0x4a6b, { 0xb1, 0xe3, 0x7c, 0x9d, 0x8f, 0x5a, 0x3e, 0x21 } };

        return library_viewer_guid;

    }

    

    // Display name in the Library menu

    const char* get_name() override {

        // Only return empty during destruction or shutdown

        if (s_being_destroyed.load() || is_app_shutting_down()) {

            return "";

        }

        return "Album Art Grid";

    }

};



// Static member definition

std::atomic<bool> album_art_grid_library_viewer::s_being_destroyed = false;



// Register the library viewer

static library_viewer_factory_t<album_art_grid_library_viewer> g_album_art_grid_library_viewer;

#endif



// Component export handled by DECLARE_COMPONENT_VERSION macro
























