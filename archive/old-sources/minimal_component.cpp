// Minimal foobar2000 component using the actual SDK
#define FOOBAR2000_TARGET_VERSION 80

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"

// Component information
DECLARE_COMPONENT_VERSION(
    "Minimal Album Grid",
    "1.0.0",
    "Minimal component for album art grid\n"
    "Created as a foundation for grid view implementation"
);

// This macro validates the component
VALIDATE_COMPONENT_FILENAME("foo_minimal_grid.dll");

// Empty service to satisfy the SDK requirements
class minimal_initquit : public initquit {
public:
    void on_init() override {
        // Component initialization
        console::print("Minimal Album Grid component loaded");
    }
    
    void on_quit() override {
        // Component cleanup
    }
};

// Register the service
static initquit_factory_t<minimal_initquit> g_minimal_initquit_factory;