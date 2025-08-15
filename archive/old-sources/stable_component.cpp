// Stable foobar2000 v2 component - no crash version
#include <windows.h>

// Component version info
extern "C" __declspec(dllexport) const char* foobar2000_component_version() {
    return "1.0.0";
}

extern "C" __declspec(dllexport) int foobar2000_get_interface_version() {
    return 80;
}

// Main interface - returns NULL (no services provided yet)
extern "C" __declspec(dllexport) void foobar2000_get_interface(void** ppv, const char* name) {
    if (ppv) *ppv = NULL;
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}