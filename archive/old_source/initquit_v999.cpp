// initquit service for Album Art Grid v9.9.9
#define FOOBAR2000_TARGET_VERSION 80
#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <gdiplus.h>
#include <atomic>

// Global shutdown flag for safety
std::atomic<bool> g_component_shutting_down(false);

// GDI+ initialization service
class initquit_gdiplus : public initquit {
private:
    ULONG_PTR m_gdiplusToken;
    
public:
    void on_init() override {
        // Initialize GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        
        console::print("Album Art Grid v9.9.9 initialized");
        console::print("Critical fix: Shutdown/sleep crash protection enabled");
        console::print("Features: Status bar fields, Now Playing indicator, Enhanced search");
    }
    
    void on_quit() override {
        // Set shutdown flag if it exists
        if (&g_component_shutting_down) {
            g_component_shutting_down = true;
        }
        
        // Shutdown GDI+
        if (m_gdiplusToken) {
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
            m_gdiplusToken = 0;
        }
        
        console::print("Album Art Grid v9.9.9 shutdown complete");
    }
};

// Register the service
static initquit_factory_t<initquit_gdiplus> g_initquit_gdiplus;