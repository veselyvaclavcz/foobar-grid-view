// Minimal foobar2000 v2 component implementation
// This creates a properly structured component that won't crash on load

#include "minimal_component_proper.h"

// Global API pointer (required by SDK)
foobar2000_api* g_foobar2000_api = nullptr;

// Component client implementation
namespace {
    class foobar2000_client_impl : public foobar2000_client, private foobar2000_component_globals
    {
    public:
        // Return the API version we support
        t_uint32 get_version() override {
            return FOOBAR2000_CLIENT_VERSION;
        }

        // Return the service factory list (empty for minimal component)
        pservice_factory_base get_service_list() override {
            return service_factory_base::__internal__list;
        }

        // Config operations (minimal implementation)
        void get_config(stream_writer* p_stream, abort_callback& p_abort) override {
            // No configuration to save for minimal component
        }

        void set_config(stream_reader* p_stream, abort_callback& p_abort) override {
            // No configuration to load for minimal component
        }

        // Component path information
        void set_library_path(const char* path, const char* name) override {
            // Store component path if needed (not required for minimal component)
        }

        // Service initialization callback
        void services_init(bool val) override {
            // Services are available when val is true
        }

        // Debug mode check
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

// Component version declaration (required by SDK)
DECLARE_COMPONENT_VERSION(
    COMPONENT_NAME,
    COMPONENT_VERSION, 
    COMPONENT_ABOUT
);

// DLL entry point function that foobar2000 calls to get our client interface
extern "C" {
    __declspec(dllexport) foobar2000_client* __cdecl foobar2000_get_interface(foobar2000_api* p_api, HINSTANCE hIns) {
        // Store the API pointer globally
        g_foobar2000_api = p_api;
        
        // Return our client implementation
        return &g_client;
    }
}

// Standard Windows DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Disable thread library calls for performance
            DisableThreadLibraryCalls(hinstDLL);
            break;
            
        case DLL_PROCESS_DETACH:
            // Cleanup if needed
            g_foobar2000_api = nullptr;
            break;
            
        default:
            break;
    }
    return TRUE;
}