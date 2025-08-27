// Safe runtime stub functions for foobar2000 symbols
// These provide minimal implementations that don't call back into foobar2000

#include <windows.h>
#include <string.h>
#include <new>

// Prevent any potential recursion by using simple implementations
extern "C" {
    // Return a valid but minimal event handle
    __declspec(dllexport) HANDLE GetInfiniteWaitEvent() {
        static HANDLE s_event = nullptr;
        if (!s_event) {
            s_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        }
        return s_event;
    }
    
    // Safe debug functions that output to debugger without recursion
    __declspec(dllexport) void uPrintCrashInfo_OnEvent() {
        OutputDebugStringA("uPrintCrashInfo_OnEvent called\n");
    }
    
    __declspec(dllexport) void uBugCheck() {
        OutputDebugStringA("uBugCheck called\n");
    }
    
    __declspec(dllexport) void uPrintfV() {
        // Safe no-op implementation
    }
    
    __declspec(dllexport) void uOutputDebugString() {
        // Safe no-op implementation  
    }
    
    // String comparison functions with safe implementations
    __declspec(dllexport) int stricmp_utf8_ex() {
        return 0; // Safe default
    }
    
    __declspec(dllexport) int stricmp_utf8() {
        return 0; // Safe default
    }
    
    // File system functions with safe defaults
    __declspec(dllexport) DWORD uGetFileAttributes() {
        return INVALID_FILE_ATTRIBUTES; // Safe failure return
    }
    
    __declspec(dllexport) void uFormatSystemErrorMessage() {
        // Safe no-op implementation
    }
}

// Minimal call stack tracker that doesn't interact with foobar2000 internals
class uCallStackTracker {
private:
    static thread_local int s_depth;
    
public:
    __declspec(dllexport) uCallStackTracker(const char* msg) {
        s_depth++;
        if (s_depth < 10) { // Prevent deep recursion
            OutputDebugStringA("CallStack: ");
            if (msg) OutputDebugStringA(msg);
            OutputDebugStringA("\n");
        }
    }
    
    __declspec(dllexport) ~uCallStackTracker() {
        s_depth--;
    }
};

thread_local int uCallStackTracker::s_depth = 0;

// C-style exports for compatibility
extern "C" {
    __declspec(dllexport) void* __cdecl uCallStackTracker_constructor(const char* msg) {
        return new(std::nothrow) uCallStackTracker(msg);
    }
    
    __declspec(dllexport) void __cdecl uCallStackTracker_destructor(void* obj) {
        if (obj) {
            delete static_cast<uCallStackTracker*>(obj);
        }
    }
}