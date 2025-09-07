// Album Art Grid Component Main File
// Version 10.0.50 - Using SDK properly

#include "../SDK-2025-03-07/foobar2000/SDK/foobar2000.h"

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "10.0.50",
    "Album Art Grid Component\n"
    "Display your music library as a customizable grid of album covers\n\n"
    "Version 10.0.50 - Final x64 Build\n"
    "Built for foobar2000 v2.x"
);

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Simple initquit service to show the component loads
class album_grid_initquit : public initquit {
public:
    void on_init() override {
        console::print("=====================================");
        console::print("Album Art Grid v10.0.50 x64");
        console::print("Component loaded successfully!");
        console::print("=====================================");
    }
    
    void on_quit() override {
        console::print("Album Art Grid v10.0.50 shutting down");
    }
};

static initquit_factory_t<album_grid_initquit> g_album_grid_initquit;