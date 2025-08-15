// Simple working component with menu item
#define FOOBAR2000_TARGET_VERSION 80

#include "SDK-2025-03-07/foobar2000/SDK/foobar2000.h"

// Component version
DECLARE_COMPONENT_VERSION(
    "Album Art Grid",
    "1.5.0",
    "Album Art Grid Component for foobar2000 v2\n"
    "Working version with menu item\n"
    "\n"
    "Select View -> Album Art Grid to test"
);

VALIDATE_COMPONENT_FILENAME("foo_albumart_grid.dll");

// Simple initialization
class albumart_grid_initquit : public initquit {
public:
    void on_init() override {
        console::print("Album Art Grid v1.5.0 loaded successfully!");
    }
    
    void on_quit() override {
        console::print("Album Art Grid shutting down.");
    }
};

static initquit_factory_t<albumart_grid_initquit> g_albumart_grid_initquit;

// Menu command
class mainmenu_albumart : public mainmenu_commands {
    enum {
        cmd_show_grid = 0,
        cmd_total
    };

public:
    t_uint32 get_command_count() override { 
        return cmd_total; 
    }
    
    GUID get_command(t_uint32 p_index) override {
        static const GUID guid_show_grid = { 0x87654321, 0x4321, 0x8765, { 0x43, 0x21, 0x87, 0x65, 0x43, 0x21, 0x87, 0x65 } };
        if (p_index == cmd_show_grid) return guid_show_grid;
        return pfc::guid_null;
    }
    
    void get_name(t_uint32 p_index, pfc::string_base & p_out) override {
        if (p_index == cmd_show_grid) p_out = "Album Art Grid";
    }
    
    bool get_description(t_uint32 p_index, pfc::string_base & p_out) override {
        if (p_index == cmd_show_grid) {
            p_out = "Show Album Art Grid view";
            return true;
        }
        return false;
    }
    
    GUID get_parent() override {
        return mainmenu_groups::view;
    }
    
    void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override {
        if (p_index == cmd_show_grid) {
            // Get library info
            auto lib = library_manager::get();
            metadb_handle_list all_items;
            lib->get_all_items(all_items);
            
            pfc::string8 message;
            message << "Album Art Grid Component v1.5.0\n\n";
            message << "Library contains " << all_items.get_count() << " tracks\n\n";
            
            // Count unique albums
            pfc::map_t<pfc::string8, bool> albums;
            for (size_t i = 0; i < all_items.get_count(); i++) {
                const file_info* info = NULL;
                if (all_items[i]->get_info(info)) {
                    const char* album = info->meta_get("ALBUM", 0);
                    if (album) albums[album] = true;
                }
            }
            
            message << "Found " << albums.get_count() << " unique albums\n\n";
            message << "Full grid UI coming soon!";
            
            popup_message::g_show(message, "Album Art Grid");
        }
    }
};

static mainmenu_commands_factory_t<mainmenu_albumart> g_mainmenu_albumart;