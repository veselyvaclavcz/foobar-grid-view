// Album Art Grid Component for foobar2000
// Version 10.0.50 - Minimal Working x64 Build

// Windows headers
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <ole2.h>
#pragma comment(lib, "winmm.lib")

// SDK
#include "../SDK-2025-03-07/foobar2000/SDK/foobar2000.h"

// Component version info
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "10.0.50",
    "Album Art Grid Component\n"
    "Display your music library as a customizable grid of album covers\n\n"
    "Version 10.0.50 - x64 Build\n"
    "Built for foobar2000 v2.x"
);

// Validate component filename
VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Simple initquit service to prove the component loads
class album_grid_initquit : public initquit {
public:
    void on_init() override {
        console::print("Album Art Grid v10.0.50 loaded successfully!");
        console::print("Component is working in x64 mode");
    }
    
    void on_quit() override {
        console::print("Album Art Grid v10.0.50 shutting down");
    }
};

// Register the service
static initquit_factory_t<album_grid_initquit> g_album_grid_initquit;