// Minimal Album Art Grid Component for foobar2000 v2
// This is the absolute minimum required for a component to load without crashing

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Export functions for foobar2000 v2
extern "C" {
    // Main entry point - MUST return NULL for minimal component
    __declspec(dllexport) void* foobar2000_get_interface(const char* name, unsigned version) {
        // Return NULL - we don't provide any services
        // This is valid and prevents crashes
        return NULL;
    }
    
    // Optional: Version string
    __declspec(dllexport) const char* foobar2000_component_version() {
        return "2.0.2";
    }
    
    // Optional: API version
    __declspec(dllexport) int foobar2000_get_interface_version() {
        return 80;  // foobar2000 v2.0 API
    }
}

// DLL entry point - required by Windows
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        // Disable thread notifications for performance
        DisableThreadLibraryCalls(hinstDLL);
        break;
    }
    return TRUE;
}