// Minimal helpers stub v10.0.5 - with shutdown protection
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <atomic>

// Global shutdown flag
static std::atomic<bool> g_shutting_down{false};

// Core shared functions - only the ones not defined in component_client.cpp
extern "C" {
    static HANDLE g_infiniteEvent = NULL;
    static CRITICAL_SECTION g_cs;
    static bool g_cs_initialized = false;
    
    __declspec(dllexport) HANDLE GetInfiniteWaitEvent() {
        if (g_shutting_down) return NULL;
        
        if (!g_infiniteEvent) {
            g_infiniteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        }
        return g_infiniteEvent;
    }
    
    __declspec(dllexport) void uPrintCrashInfo_OnEvent() {
        // v10.0.5: Add shutdown protection
        if (g_shutting_down) return;
        
        // Empty implementation - but safe during shutdown
    }
    
    __declspec(dllexport) void uBugCheck() {
        // v10.0.5: Don't crash during shutdown
        if (g_shutting_down) return;
        
        // In production, this would terminate the process
        OutputDebugStringA("uBugCheck called\n");
    }
    
    __declspec(dllexport) void uOutputDebugString(const char* msg) {
        if (g_shutting_down) return;
        if (msg) OutputDebugStringA(msg);
    }
    
    __declspec(dllexport) int stricmp_utf8_ex(const char* a, const char* b, unsigned len) {
        if (!a) a = "";
        if (!b) b = "";
        return _strnicmp(a, b, len);
    }
    
    __declspec(dllexport) int stricmp_utf8(const char* a, const char* b) {
        if (!a) a = "";
        if (!b) b = "";
        return _stricmp(a, b);
    }
    
    __declspec(dllexport) void uPrintfV(const char* fmt, va_list args) {
        if (g_shutting_down) return;
        
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        OutputDebugStringA(buffer);
    }
    
    __declspec(dllexport) void uPrintf(const char* fmt, ...) {
        if (g_shutting_down) return;
        
        va_list args;
        va_start(args, fmt);
        uPrintfV(fmt, args);
        va_end(args);
    }
    
    __declspec(dllexport) DWORD uGetFileAttributes(const wchar_t* fn) {
        if (g_shutting_down) return INVALID_FILE_ATTRIBUTES;
        return GetFileAttributesW(fn);
    }
    
    __declspec(dllexport) void uFormatSystemErrorMessage(void* buffer, DWORD code) {
        if (g_shutting_down) return;
        
        char* buf = (char*)buffer;
        sprintf_s(buf, 256, "System error: 0x%08X", code);
    }
    
    // v10.0.5: Add shutdown cleanup function
    __declspec(dllexport) void helpers_shutdown() {
        g_shutting_down = true;
        
        // Clean up event handle
        if (g_infiniteEvent) {
            CloseHandle(g_infiniteEvent);
            g_infiniteEvent = NULL;
        }
        
        // Clean up critical section if initialized
        if (g_cs_initialized) {
            DeleteCriticalSection(&g_cs);
            g_cs_initialized = false;
        }
    }
    
    // v10.0.5: Add initialization function
    __declspec(dllexport) void helpers_init() {
        g_shutting_down = false;
        
        if (!g_cs_initialized) {
            InitializeCriticalSection(&g_cs);
            g_cs_initialized = true;
        }
    }
}

// CallStackTracker class with shutdown protection
class __declspec(dllexport) uCallStackTracker {
public:
    uCallStackTracker(const char* name) {
        // v10.0.5: Check shutdown state
        if (g_shutting_down) return;
        // Empty implementation
    }
    
    ~uCallStackTracker() {
        // v10.0.5: Safe during shutdown
        // Empty implementation  
    }
};

// FB2K namespace functions with shutdown protection
namespace fb2k {
    __declspec(dllexport) void setWindowDarkMode(HWND hwnd, bool dark) {
        if (g_shutting_down || !hwnd) return;
        
        typedef HRESULT (WINAPI *pDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
        HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
        if (dwmapi) {
            pDwmSetWindowAttribute func = (pDwmSetWindowAttribute)GetProcAddress(dwmapi, "DwmSetWindowAttribute");
            if (func) {
                BOOL value = dark ? TRUE : FALSE;
                func(hwnd, 20, &value, sizeof(value));
            }
            FreeLibrary(dwmapi);
        }
    }
    
    __declspec(dllexport) bool isDarkMode() {
        if (g_shutting_down) return false;
        
        HKEY hKey;
        DWORD value = 1;
        DWORD size = sizeof(DWORD);
        if (RegOpenKeyExW(HKEY_CURRENT_USER, 
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size);
            RegCloseKey(hKey);
        }
        return value == 0;
    }
}