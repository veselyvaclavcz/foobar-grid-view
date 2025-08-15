// Minimal SDK-based Album Art Grid Component for foobar2000 v2
// This implements the correct interface expected by foobar2000

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Forward declarations to avoid needing full SDK headers
class foobar2000_api;
class foobar2000_client;
class pservice_factory_base;
class stream_writer;
class stream_reader;
class abort_callback;

// Define the version constant
#define FOOBAR2000_CLIENT_VERSION 80

// Minimal implementation of foobar2000_client interface
class foobar2000_client_minimal : public foobar2000_client {
public:
    // Return the API version we support
    virtual unsigned get_version() { 
        return FOOBAR2000_CLIENT_VERSION; 
    }
    
    // Return NULL - we don't provide any services
    virtual pservice_factory_base* get_service_list() { 
        return nullptr; 
    }
    
    // Config methods - do nothing for minimal component
    virtual void get_config(stream_writer* p_stream, abort_callback& p_abort) {}
    virtual void set_config(stream_reader* p_stream, abort_callback& p_abort) {}
    
    // Component info
    virtual void set_library_path(const char* path, const char* name) {}
    virtual void services_init(bool val) {}
    virtual bool is_debug() { return false; }
};

// Global instance of our client
static foobar2000_client_minimal g_client;

// Export the correct interface function
extern "C" {
    __declspec(dllexport) foobar2000_client* __cdecl foobar2000_get_interface(foobar2000_api* p_api, HINSTANCE hIns) {
        // Return our client implementation
        return &g_client;
    }
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}