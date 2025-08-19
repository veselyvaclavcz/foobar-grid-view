// Minimal helpers stub - only functions NOT in component_client.cpp
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <cstdarg>

// Core shared functions - only the ones not defined in component_client.cpp
extern "C" {
    static HANDLE g_infiniteEvent = NULL;
    
    __declspec(dllexport) HANDLE GetInfiniteWaitEvent() {
        if (!g_infiniteEvent) {
            g_infiniteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        }
        return g_infiniteEvent;
    }
    
    __declspec(dllexport) void uPrintCrashInfo_OnEvent() {
        // Empty implementation
    }
    
    __declspec(dllexport) void uBugCheck() {
        // In production, this would terminate the process
        OutputDebugStringA("uBugCheck called\n");
    }
    
    __declspec(dllexport) void uOutputDebugString(const char* msg) {
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
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        OutputDebugStringA(buffer);
    }
    
    __declspec(dllexport) void uPrintf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        uPrintfV(fmt, args);
        va_end(args);
    }
    
    __declspec(dllexport) DWORD uGetFileAttributes(const wchar_t* fn) {
        return GetFileAttributesW(fn);
    }
    
    __declspec(dllexport) void uFormatSystemErrorMessage(void* buffer, DWORD code) {
        char* buf = (char*)buffer;
        sprintf_s(buf, 256, "System error: 0x%08X", code);
    }
}

// CallStackTracker class
class __declspec(dllexport) uCallStackTracker {
public:
    uCallStackTracker(const char* name) {
        // Empty implementation
    }
    
    ~uCallStackTracker() {
        // Empty implementation  
    }
};

// FB2K namespace functions
namespace fb2k {
    __declspec(dllexport) void setWindowDarkMode(HWND hwnd, bool dark) {
        if (!hwnd) return;
        
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