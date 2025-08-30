// Fixed initquit service for v10.0.5 - robust shutdown handling
#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <gdiplus.h>
#include <atomic>

// External shutdown function from helpers
extern "C" void helpers_shutdown();
extern "C" void helpers_init();

// GDI+ initialization service with robust shutdown handling
class initquit_gdiplus : public initquit {
private:
    ULONG_PTR m_gdiplusToken = 0;
    static std::atomic<bool> s_initialized;
    static std::atomic<bool> s_shutting_down;
    static CRITICAL_SECTION s_cs;
    
public:
    void on_init() override {
        // Initialize critical section for thread safety
        InitializeCriticalSection(&s_cs);
        
        // Initialize helpers
        helpers_init();
        
        s_shutting_down = false;
        s_initialized = false;
        
        // Initialize GDI+ with proper error checking
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        gdiplusStartupInput.GdiplusVersion = 1;
        gdiplusStartupInput.DebugEventCallback = NULL;
        gdiplusStartupInput.SuppressBackgroundThread = FALSE;
        gdiplusStartupInput.SuppressExternalCodecs = FALSE;
        
        Gdiplus::Status status = Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        
        if (status == Gdiplus::Ok && m_gdiplusToken != 0) {
            s_initialized = true;
            console::print("Album Art Grid v10.0.5 initialized");
            console::print("Robust shutdown handling enabled");
        } else {
            console::print("Album Art Grid v10.0.5: GDI+ initialization failed");
            m_gdiplusToken = 0;
        }
    }
    
    void on_quit() override {
        // Enter critical section for thread-safe shutdown
        EnterCriticalSection(&s_cs);
        
        // Signal shutdown to all components
        s_shutting_down = true;
        
        // Wait a bit for any pending operations to complete
        Sleep(100);
        
        // Shutdown helpers first
        try {
            helpers_shutdown();
        } catch(...) {
            // Ignore exceptions during shutdown
        }
        
        // Shutdown GDI+ if it was initialized
        if (s_initialized && m_gdiplusToken != 0) {
            try {
                // v10.0.5: More robust GDI+ shutdown
                // Force cleanup of any remaining GDI+ objects
                Gdiplus::GdiplusShutdown(m_gdiplusToken);
            } catch(...) {
                // Ignore any exceptions during shutdown
            }
            m_gdiplusToken = 0;
            s_initialized = false;
        }
        
        LeaveCriticalSection(&s_cs);
        
        // Clean up critical section
        DeleteCriticalSection(&s_cs);
    }
    
    static bool is_shutting_down() {
        return s_shutting_down;
    }
    
    static bool is_initialized() {
        return s_initialized && !s_shutting_down;
    }
};

// Define the static members
std::atomic<bool> initquit_gdiplus::s_initialized{false};
std::atomic<bool> initquit_gdiplus::s_shutting_down{false};
CRITICAL_SECTION initquit_gdiplus::s_cs;

// Register the service
static service_factory_single_t<initquit_gdiplus> g_initquit_gdiplus;