// Minimal safe foobar2000 component
// This avoids functions that require helpers library

#define FOOBAR2000_TARGET_VERSION 80
#define FOOBAR2000_TARGET_VERSION_COMPATIBLE 80

// Include minimal headers to avoid pulling in problematic dependencies
#include "SDK-2025-03-07/foobar2000/SDK/foobar2000-lite.h"
#include "SDK-2025-03-07/foobar2000/SDK/componentversion.h"
#include "SDK-2025-03-07/foobar2000/SDK/initquit.h"

// Component information
DECLARE_COMPONENT_VERSION(
    "Minimal Safe Grid",
    "1.0.0", 
    "Minimal foobar2000 component without problematic dependencies."
);

// This validates the component filename
VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Simple initquit service that doesn't use console or other problematic functions
class minimal_safe_initquit : public initquit {
public:
    void on_init() override {
        // Component initialization - no console output to avoid dependencies
    }
    
    void on_quit() override {
        // Component cleanup
    }
};

// Register the service with foobar2000
static initquit_factory_t<minimal_safe_initquit> g_initquit_factory;