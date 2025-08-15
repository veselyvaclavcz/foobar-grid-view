// Album Art Grid Component v9.6 - Track Count Badge Edition
// Minimal version based on v9.5 with badge feature

#define FOOBAR2000_TARGET_VERSION 82

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>

// Use relative paths that match v9.5
#include "../../SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include "../../SDK-2025-03-07/foobar2000/helpers/helpers.h"

// Component validation
#ifdef _DEBUG
#define COMPONENT_NAME "foo_albumart_grid_v96_debug"
#else
#define COMPONENT_NAME "foo_albumart_grid_v96"
#endif

VALIDATE_COMPONENT_FILENAME(COMPONENT_NAME ".dll");

// Forward declaration of the minimal implementation
namespace {
    class minimal_client : public foobar2000_client {
    public:
        t_uint32 get_version() override { return FOOBAR2000_CLIENT_VERSION; }
        void get_config(foobar2000_config* p_out) override {
            p_out->bogo = 0;
            p_out->ui_element_api_version = 1;
            p_out->ui_element_v2 = false;
        }
    };
    
    minimal_client g_client;
}

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "9.6.0",
    "Album Art Grid Component for foobar2000\n"
    "Displays album art in a customizable grid layout.\n\n"
    "NEW in v9.6:\n"
    "- Track count badge on album art when labels hidden\n"
    "- Badge appears in top-right corner\n\n"
    "Features:\n"
    "- Artist images for artist groupings\n" 
    "- 13 grouping modes\n"
    "- 11 sorting options\n"
    "- Auto-fill mode\n"
    "- Ctrl+Mouse Wheel column control\n\n"
    "Created with assistance from Anthropic's Claude AI"
);

// Required export
extern "C" {
    FOOBAR2000_API foobar2000_client* foobar2000_get_interface(foobar2000_api* p_api, HINSTANCE) {
        core_api::ptr = p_api;
        return &g_client;
    }
}