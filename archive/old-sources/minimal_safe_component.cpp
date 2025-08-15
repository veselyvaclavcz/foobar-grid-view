#define FOOBAR2000_TARGET_VERSION_COMPATIBLE 82
#define FOOBAR2000_TARGET_VERSION 82

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"

// This is a safer minimal component that properly initializes all required SDK components
// and follows the exact initialization pattern from the reference SDK component

#ifdef _WIN32
static HINSTANCE g_hIns;
#endif

// Core API implementation for this component
namespace core_api {
#ifdef _WIN32
    HINSTANCE get_my_instance() {
        return g_hIns;
    }
#endif

    fb2k::hwnd_t get_main_window() {
        PFC_ASSERT(g_foobar2000_api != NULL);
        return g_foobar2000_api->get_main_window();
    }

    const char* get_my_file_name() {
        return "foo_albumart_grid_minimal.dll";
    }

    const char* get_my_full_path() {
        static pfc::string_simple full_path;
        return full_path;
    }

    bool are_services_available() {
        return g_foobar2000_api != NULL;
    }

    bool assert_main_thread() {
        return g_foobar2000_api ? g_foobar2000_api->assert_main_thread() : true;
    }

    void ensure_main_thread() {
        if (!is_main_thread()) FB2K_BugCheck();
    }

    bool is_main_thread() {
        return g_foobar2000_api ? g_foobar2000_api->is_main_thread() : true;
    }

    const char* get_profile_path() {
        PFC_ASSERT(g_foobar2000_api != NULL);
        return g_foobar2000_api->get_profile_path();
    }

    bool is_shutting_down() {
        return g_foobar2000_api ? g_foobar2000_api->is_shutting_down() : false;
    }

    bool is_initializing() {
        return g_foobar2000_api ? g_foobar2000_api->is_initializing() : true;
    }

    bool is_portable_mode_enabled() {
        PFC_ASSERT(g_foobar2000_api != NULL);
        return g_foobar2000_api->is_portable_mode_enabled();
    }

    bool is_quiet_mode_enabled() {
        PFC_ASSERT(g_foobar2000_api != NULL);
        return g_foobar2000_api->is_quiet_mode_enabled();
    }
}

namespace {
    // Safe minimal client implementation
    class safe_foobar2000_client : public foobar2000_client, private foobar2000_component_globals {
        static pfc::string_simple g_name, g_full_path;
        static bool g_services_available, g_initialized;

    public:
        t_uint32 get_version() override {
            return FOOBAR2000_CLIENT_VERSION;
        }

        pservice_factory_base get_service_list() override {
            return service_factory_base::__internal__list;
        }

        void get_config(stream_writer * p_stream, abort_callback & p_abort) override {
            // Safe empty implementation
            if (p_stream == nullptr) return;
        }

        void set_config(stream_reader * p_stream, abort_callback & p_abort) override {
            // Safe empty implementation
            if (p_stream == nullptr) return;
        }

        void set_library_path(const char * path, const char * name) override {
            if (path) g_full_path = path;
            if (name) g_name = name;
        }

        void services_init(bool val) override {
            if (val) g_initialized = true;
            g_services_available = val;
        }

        bool is_debug() override {
            return PFC_DEBUG != 0;
        }
    };

    // Static member definitions
    pfc::string_simple safe_foobar2000_client::g_name, safe_foobar2000_client::g_full_path;
    bool safe_foobar2000_client::g_services_available = false, safe_foobar2000_client::g_initialized = false;
}

// Global client instance
static safe_foobar2000_client g_safe_client;

#ifdef _WIN32
extern "C" {
    // Component entry point - this is called by foobar2000 v2
    __declspec(dllexport) foobar2000_client * _cdecl foobar2000_get_interface(foobar2000_api * p_api, HINSTANCE hIns) {
        // Validate parameters
        if (!p_api || !hIns) {
            return nullptr;
        }

        // Store globals
        g_hIns = hIns;
        g_foobar2000_api = p_api;

        // Return our client implementation
        return &g_safe_client;
    }
}
#endif

// Minimal initquit service to ensure proper initialization
namespace {
    class minimal_initquit : public initquit {
    public:
        void on_init() override {
            // Component initialized successfully
        }

        void on_quit() override {
            // Component shutdown
        }
    };

    FB2K_SERVICE_FACTORY(minimal_initquit);
}