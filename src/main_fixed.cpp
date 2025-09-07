// Album Art Grid Component for foobar2000
// Version 10.0.50 - Complete Rewrite with proper entry points

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../SDK-2025-03-07/foobar2000/SDK/foobar2000.h"

// Component information
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "10.0.50",
    "Album Art Grid Component\n"
    "Display your music library as a customizable grid of album covers\n\n"
    "Features:\n"
    "- Multiple grouping modes (Album, Artist, Genre, etc.)\n"
    "- Flexible sorting options\n"  
    "- Resizable thumbnails with no limits\n"
    "- Automatic memory management\n"
    "- Library and playlist views\n"
    "- Smart caching for performance\n\n"
    "Version 10.0.50 - Complete stable rewrite\n"
    "Built with modular architecture for reliability"
);

// Validate SDK version
VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Simple initquit service for now
class album_grid_initquit : public initquit {
public:
    void on_init() override {
        console::print("Album Art Grid v10.0.50 initialized");
    }
    
    void on_quit() override {
        console::print("Album Art Grid v10.0.50 shutting down");
    }
};

static initquit_factory_t<album_grid_initquit> g_initquit;