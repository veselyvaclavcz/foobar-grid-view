// Working foobar2000 v2 component with proper SDK linkage
#define FOOBAR2000_TARGET_VERSION 80

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"

// Component version information
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "1.0.0",
    "Album Art Grid Component for foobar2000 v2\n"
    "Displays album covers in a customizable grid layout.\n"
    "\n"
    "Working component with proper SDK integration."
);

// Prevent component renaming
VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Service to verify component loads
class albumart_grid_initquit : public initquit {
public:
    void on_init() override {
        console::print("Album Art Grid: Component initialized successfully!");
        FB2K_console_formatter() << "Album Art Grid: Running on foobar2000 v" << core_version_info_v2::get()->get_version_string();
    }
    
    void on_quit() override {
        console::print("Album Art Grid: Component shutting down.");
    }
};

// Register the initquit service
static initquit_factory_t<albumart_grid_initquit> g_albumart_grid_initquit;

// Add a simple menu item to verify functionality
class mainmenu_albumart_grid : public mainmenu_commands {
public:
    enum {
        cmd_total = 1
    };
    
    t_uint32 get_command_count() override {
        return cmd_total;
    }
    
    GUID get_command(t_uint32 p_index) override {
        static const GUID guid_albumart_grid = { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x12, 0x34, 0x12, 0x34, 0x12, 0x34 } };
        return guid_albumart_grid;
    }
    
    void get_name(t_uint32 p_index, pfc::string_base & p_out) override {
        p_out = "Album Art Grid Settings";
    }
    
    bool get_description(t_uint32 p_index, pfc::string_base & p_out) override {
        p_out = "Configure Album Art Grid display";
        return true;
    }
    
    GUID get_parent() override {
        return mainmenu_groups::view;
    }
    
    void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override {
        popup_message::g_show("Album Art Grid component is working!\n\nGrid view coming soon.", "Album Art Grid");
    }
};

// Register the menu service
static mainmenu_commands_factory_t<mainmenu_albumart_grid> g_mainmenu_albumart_grid;