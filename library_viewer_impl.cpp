// Library Viewer implementation for Album Art Grid
// This makes the component appear in foobar2000's Library viewer list

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>

// Forward declaration - these functions are implemented in grid_v10_0_13_library.cpp
extern void show_album_art_grid_window();
extern bool is_album_art_grid_window_visible();

// Library viewer implementation
class album_art_grid_library_viewer : public library_viewer {
public:
    // Return GUID of preferences page (we don't have one)
    GUID get_preferences_page() override {
        return pfc::guid_null;
    }
    
    // We support the "activate" action (showing the window)
    bool have_activate() override {
        return true;
    }
    
    // Show the Album Art Grid window when activated from Library menu
    void activate() override {
        console::print("[Album Art Grid v10.0.13] Activated from Library viewer");
        
        // Show the window (this function is implemented in main grid file)
        show_album_art_grid_window();
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
        return "Album Art Grid";
    }
};

// Register the library viewer
static library_viewer_factory_t<album_art_grid_library_viewer> g_album_art_grid_library_viewer;