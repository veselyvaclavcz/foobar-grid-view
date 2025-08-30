// Library Viewer implementation for Album Art Grid v10.0.17
// FIXED: Shutdown crash protection WITHOUT blocking new instances
// This makes the component appear in foobar2000's Library viewer list

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <atomic>

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
        s_being_destroyed = false;
    }
    
    ~album_art_grid_library_viewer() {
        // Mark as being destroyed
        s_being_destroyed = true;
    }
    
    // Return GUID of preferences page (we don't have one)
    GUID get_preferences_page() override {
        return pfc::guid_null;
    }
    
    // We support the "activate" action (showing the window)
    bool have_activate() override {
        // Only block if we're actively being destroyed OR app is shutting down
        if (s_being_destroyed || is_app_shutting_down()) {
            return false;
        }
        return true;
    }
    
    // Show the Album Art Grid window when activated from Library menu
    void activate() override {
        // Only block if we're actively being destroyed OR app is shutting down
        if (s_being_destroyed || is_app_shutting_down()) {
            return;
        }
        
        // Safe call without SEH (causes compilation issues)
        try {
            console::print("[Album Art Grid v10.0.16] Activated from Library viewer");
            
            // Show the window (this function is implemented in main grid file)
            show_album_art_grid_window();
        }
        catch(...) {
            // Ignore exceptions during potential shutdown race
        }
    }
    
    // Unique GUID for this library viewer (different from ui_element GUID)
    GUID get_guid() override {
        // {9B4D6C8F-AF5E-4B7C-C2F4-8E5F0C6A4F32} - Unique GUID for v10.0.16
        static const GUID library_viewer_guid = 
            { 0x9b4d6c8f, 0xaf5e, 0x4b7c, { 0xc2, 0xf4, 0x8e, 0x5f, 0x0c, 0x6a, 0x4f, 0x32 } };
        return library_viewer_guid;
    }
    
    // Display name in the Library menu
    const char* get_name() override {
        // Only return empty during destruction or shutdown
        if (s_being_destroyed || is_app_shutting_down()) {
            return "";
        }
        return "Album Art Grid";
    }
};

// Static member definition
std::atomic<bool> album_art_grid_library_viewer::s_being_destroyed{false};

// Register the library viewer
static library_viewer_factory_t<album_art_grid_library_viewer> g_album_art_grid_library_viewer;