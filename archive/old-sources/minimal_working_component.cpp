// Minimal working foobar2000 v2 component
// Based on the working component_client.cpp pattern

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>  // For timeGetTime

// Forward declarations from SDK
typedef unsigned int t_uint32;
typedef void* pservice_factory_base;

class stream_writer;
class stream_reader;
class abort_callback;

// Component client interface (minimal version)
class foobar2000_client {
public:
    virtual ~foobar2000_client() {}
    virtual t_uint32 get_version() = 0;
    virtual pservice_factory_base get_service_list() = 0;
    virtual void get_config(stream_writer* p_stream, abort_callback& p_abort) = 0;
    virtual void set_config(stream_reader* p_stream, abort_callback& p_abort) = 0;
    virtual void set_library_path(const char* path, const char* name) = 0;
    virtual void services_init(bool val) = 0;
    virtual bool is_debug() = 0;
};

// API interface
class foobar2000_api {
public:
    virtual ~foobar2000_api() {}
    // We don't need to implement anything for minimal component
};

// Version constants
#define FOOBAR2000_CLIENT_VERSION 80

// Global API pointer
foobar2000_api* g_foobar2000_api = nullptr;

// Component globals helper
class foobar2000_component_globals {
public:
    foobar2000_component_globals() {
        // Initialize if needed
    }
};

// Service factory base (empty for minimal component)
class service_factory_base {
public:
    static service_factory_base* __internal__list;
};

// Initialize the service list as null
service_factory_base* service_factory_base::__internal__list = nullptr;

// Our client implementation
namespace {
    class foobar2000_client_impl : public foobar2000_client, private foobar2000_component_globals
    {
    public:
        t_uint32 get_version() override {
            return FOOBAR2000_CLIENT_VERSION;
        }

        pservice_factory_base get_service_list() override {
            return service_factory_base::__internal__list;
        }

        void get_config(stream_writer* /*p_stream*/, abort_callback& /*p_abort*/) override {
            // No config for minimal component
        }

        void set_config(stream_reader* /*p_stream*/, abort_callback& /*p_abort*/) override {
            // No config for minimal component
        }

        void set_library_path(const char* /*path*/, const char* /*name*/) override {
            // Store path if needed (not required for minimal)
        }

        void services_init(bool /*val*/) override {
            // Services initialization callback
        }

        bool is_debug() override {
            #ifdef _DEBUG
            return true;
            #else
            return false;
            #endif
        }
    };
}

// Global client instance
static foobar2000_client_impl g_client;

// Export function that foobar2000 calls
extern "C" {
    __declspec(dllexport) foobar2000_client* __cdecl foobar2000_get_interface(foobar2000_api* p_api, HINSTANCE /*hIns*/) {
        g_foobar2000_api = p_api;
        return &g_client;
    }
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID /*lpvReserved*/) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            g_foobar2000_api = nullptr;
            break;
    }
    return TRUE;
}