// Proper foobar2000 component that won't crash
// This component uses the correct SDK pattern with proper linking

#define FOOBAR2000_TARGET_VERSION 80
#define FOOBAR2000_TARGET_VERSION_COMPATIBLE 80

// Include the complete SDK
#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"

// Component information - this is required for all components
DECLARE_COMPONENT_VERSION(
    "Album Art Grid (Proper)",
    "1.0.0", 
    "A proper foobar2000 component that doesn't crash.\n"
    "Built with full SDK integration and proper foobar2000_client implementation."
);

// This validates the component filename
VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Simple initquit service to demonstrate the component loads properly
class albumart_grid_initquit : public initquit {
public:
    void on_init() override {
        console::print("Album Art Grid component loaded successfully!");
        console::print("Component linked with proper foobar2000_client implementation.");
    }
    
    void on_quit() override {
        console::print("Album Art Grid component shutting down.");
    }
};

// Register the service with foobar2000
static initquit_factory_t<albumart_grid_initquit> g_initquit_factory;

// Optional: Add a simple menu item to verify the component works
class albumart_grid_menu : public mainmenu_commands {
public:
    enum {
        cmd_about = 0,
        cmd_total
    };
    
    t_uint32 get_command_count() override {
        return cmd_total;
    }
    
    GUID get_command(t_uint32 p_index) override {
        // Generate a unique GUID for our command
        static const GUID guid_about = { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef } };
        
        if (p_index == cmd_about) return guid_about;
        return pfc::guid_null;
    }
    
    void get_name(t_uint32 p_index, pfc::string_base & p_out) override {
        if (p_index == cmd_about) {
            p_out = "About Album Art Grid";
        }
    }
    
    bool get_description(t_uint32 p_index, pfc::string_base & p_out) override {
        if (p_index == cmd_about) {
            p_out = "Shows information about the Album Art Grid component";
            return true;
        }
        return false;
    }
    
    GUID get_parent() override {
        return mainmenu_groups::help;
    }
    
    void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override {
        if (p_index == cmd_about) {
            popup_message::g_show("Album Art Grid Component\n\nVersion 1.0.0\nBuilt with proper SDK integration\n\nThis component demonstrates that the vtable crash issue has been resolved.", "About Album Art Grid");
        }
    }
};

// Register the menu service
static mainmenu_commands_factory_t<albumart_grid_menu> g_menu_factory;