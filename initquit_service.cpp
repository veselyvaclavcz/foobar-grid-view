// initquit service for Album Art Grid component
#define FOOBAR2000_TARGET_VERSION 80
#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <windows.h>
#include <gdiplus.h>

// GDI+ initialization service
class initquit_gdiplus : public initquit {
private:
    ULONG_PTR m_gdiplusToken;
    
public:
    void on_init() override {
        // Initialize GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        
        console::print("Album Art Grid v9.9.0 initialized");
    }
    
    void on_quit() override {
        // Shutdown GDI+
        if (m_gdiplusToken) {
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
            m_gdiplusToken = 0;
        }
        
        console::print("Album Art Grid v9.9.0 shutdown");
    }
};

// Register the service
static initquit_factory_t<initquit_gdiplus> g_initquit_gdiplus;