// Fixed initquit service for v10.0.16 - Enhanced Callback Protection
// NO GLOBAL FLAGS - only instance-specific protection
// Fixed SEH exception handling
#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <gdiplus.h>
#include <atomic>

// Forward declaration for shutdown notification
namespace shutdown_protection {
    extern void initiate_app_shutdown();  // Only called during REAL app shutdown
}

// Helper function for GDI+ shutdown with SEH
static void shutdown_gdiplus_safe(ULONG_PTR token) {
    __try {
        if (token != 0) {
            Gdiplus::GdiplusShutdown(token);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        // Ignore any exceptions during shutdown
    }
}

// GDI+ initialization service with improved shutdown handling
class initquit_gdiplus : public initquit {
private:
    ULONG_PTR m_gdiplusToken = 0;
    static std::atomic<bool> s_shutting_down;
    
public:
    void on_init() override {
        s_shutting_down = false;
        
        // Initialize GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        gdiplusStartupInput.GdiplusVersion = 1;
        gdiplusStartupInput.DebugEventCallback = NULL;
        gdiplusStartupInput.SuppressBackgroundThread = FALSE;
        gdiplusStartupInput.SuppressExternalCodecs = FALSE;
        
        Gdiplus::Status status = Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        
        if (status == Gdiplus::Ok && m_gdiplusToken != 0) {
            console::print("Album Art Grid v10.0.16 initialized - Unicode & Crash Fix");
            console::print("Available in Library menu - Press A-Z or 0-9 to jump to albums");
        } else {
            console::print("Album Art Grid v10.0.16: GDI+ initialization failed");
            m_gdiplusToken = 0;
        }
    }
    
    void on_quit() override {
        console::print("initquit::quit entry");
        
        // Set LOCAL shutdown flag (not global!)
        s_shutting_down = true;
        
        // Notify ONLY the active grid instances about app shutdown
        // This does NOT set any global flags that would block new instances
        try {
            shutdown_protection::initiate_app_shutdown();
        } catch(...) {
            // Ignore exceptions
        }
        
        // Wait for any pending operations to complete
        Sleep(100);
        
        // Shutdown GDI+ using helper function with SEH
        shutdown_gdiplus_safe(m_gdiplusToken);
        m_gdiplusToken = 0;
        
        console::print("initquit::quit exit");
    }
    
    static bool is_shutting_down() {
        return s_shutting_down;
    }
};

// Define the static member
std::atomic<bool> initquit_gdiplus::s_shutting_down{false};

// Register the service
static service_factory_single_t<initquit_gdiplus> g_initquit_gdiplus;