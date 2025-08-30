// Fixed initquit service for v10.0.7 - line 800 crash fix
#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <gdiplus.h>
#include <atomic>

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
            console::print("Album Art Grid v10.0.7 initialized - Line 800 crash fix applied");
            console::print("Callback validity checks and crash protection enabled");
        } else {
            console::print("Album Art Grid v10.0.7: GDI+ initialization failed");
            m_gdiplusToken = 0;
        }
    }
    
    void on_quit() override {
        // Signal shutdown to all instances
        s_shutting_down = true;
        
        // Wait a bit for any pending operations to complete
        Sleep(50);
        
        // Shutdown GDI+ if it was initialized
        if (m_gdiplusToken != 0) {
            try {
                Gdiplus::GdiplusShutdown(m_gdiplusToken);
            } catch(...) {
                // Ignore any exceptions during shutdown
            }
            m_gdiplusToken = 0;
        }
    }
    
    static bool is_shutting_down() {
        return s_shutting_down;
    }
};

// Define the static member
std::atomic<bool> initquit_gdiplus::s_shutting_down{false};

// Register the service
static service_factory_single_t<initquit_gdiplus> g_initquit_gdiplus;