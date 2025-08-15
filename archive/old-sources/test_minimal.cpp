// Ultra-minimal test component for foobar2000 v2
// This version requires NO SDK headers - just Windows headers

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Export the simplest possible function that just returns NULL
// This tells foobar2000 we exist but provide no services
extern "C" {
    __declspec(dllexport) void* __cdecl foobar2000_get_interface(void* p_api, HINSTANCE hIns) {
        // Return NULL means we're a valid component but provide no services
        // This should NOT crash - if it does, the crash is in foobar2000's handling
        return NULL;
    }
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}