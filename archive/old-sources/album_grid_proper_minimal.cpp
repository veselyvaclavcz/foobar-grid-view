// Properly structured minimal component for foobar2000 v2
// This implements the exact interface expected with correct types

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdint.h>

// Define necessary types from the SDK
typedef uint32_t t_uint32;
typedef size_t t_size;

// Forward declarations
class service_factory_base;
class stream_writer;
class stream_reader; 
class abort_callback;
class foobar2000_api;

// NOVTABLE macro - prevents vtable generation in base class
#ifdef _MSC_VER
#define NOVTABLE __declspec(novtable)
#else
#define NOVTABLE
#endif

// The foobar2000_client interface that components must implement
class NOVTABLE foobar2000_client {
public:
    virtual t_uint32 get_version() = 0;
    virtual service_factory_base* get_service_list() = 0;
    virtual void get_config(stream_writer* p_stream, abort_callback& p_abort) = 0;
    virtual void set_config(stream_reader* p_stream, abort_callback& p_abort) = 0;
    virtual void set_library_path(const char* path, const char* name) = 0;
    virtual void services_init(bool val) = 0;
    virtual bool is_debug() = 0;
};

// Our minimal implementation
class minimal_client : public foobar2000_client {
private:
    // Store component info
    char m_path[MAX_PATH];
    char m_name[MAX_PATH];
    bool m_services_available;
    
public:
    minimal_client() : m_services_available(false) {
        m_path[0] = 0;
        m_name[0] = 0;
    }
    
    // Return API version 80 for foobar2000 v2.0
    virtual t_uint32 get_version() override {
        return 80;
    }
    
    // No services provided
    virtual service_factory_base* get_service_list() override {
        return nullptr;
    }
    
    // Config methods - minimal implementation
    virtual void get_config(stream_writer* p_stream, abort_callback& p_abort) override {
        // Nothing to save
    }
    
    virtual void set_config(stream_reader* p_stream, abort_callback& p_abort) override {
        // Nothing to load
    }
    
    virtual void set_library_path(const char* path, const char* name) override {
        if (path) {
            strncpy_s(m_path, path, _TRUNCATE);
        }
        if (name) {
            strncpy_s(m_name, name, _TRUNCATE);
        }
    }
    
    virtual void services_init(bool val) override {
        m_services_available = val;
    }
    
    virtual bool is_debug() override {
#ifdef _DEBUG
        return true;
#else
        return false;
#endif
    }
};

// Global instance
static minimal_client g_client;
static foobar2000_api* g_api = nullptr;

// Export the entry point with correct signature
extern "C" {
    __declspec(dllexport) foobar2000_client* __cdecl foobar2000_get_interface(foobar2000_api* p_api, HINSTANCE hIns) {
        g_api = p_api;
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