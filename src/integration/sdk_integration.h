#pragma once

#include "../foundation/lifecycle_manager.h"
#include "../foundation/callback_manager.h"
#include "../../SDK-2025-03-07/foobar2000/SDK/foobar2000.h"
#include <memory>
#include <atomic>

namespace albumart_grid {

// Forward declarations
class GridWindow;
class AlbumDataModel;

// GUID for the UI element
// {A3E5C2F1-4B7D-4E3A-9C8B-1F2D3E4A5B6C}
constexpr GUID album_grid_guid = 
{ 0xa3e5c2f1, 0x4b7d, 0x4e3a, { 0x9c, 0x8b, 0x1f, 0x2d, 0x3e, 0x4a, 0x5b, 0x6c } };

// UI Element instance - the actual component
class album_grid_instance : public ui_element_instance, 
                           public validated_object,
                           public ILifecycleAware {
public:
    album_grid_instance(HWND parent, 
                       ui_element_config::ptr config,
                       ui_element_instance_callback_ptr callback);
    
    // ui_element_instance methods
    HWND get_wnd() override;
    void set_configuration(ui_element_config::ptr config) override;
    ui_element_config::ptr get_configuration() override;
    
    // CRITICAL: Manual reference counting - NO service_impl_t wrapper!
    virtual int service_add_ref() noexcept override {
        LONG val = m_refcount.fetch_add(1, std::memory_order_acq_rel) + 1;
        return val;
    }
    
    virtual int service_release() noexcept override {
        LONG count = m_refcount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (count == 0) {
            delete this;
        }
        return count;
    }
    
    // ILifecycleAware implementation
    void on_initialize() override;
    void on_shutdown() override;
    bool is_valid() const override;
    
protected:
    ~album_grid_instance();
    
private:
    // Reference count - start at 1
    mutable std::atomic<LONG> m_refcount{1};
    
    // UI callback from host
    ui_element_instance_callback_ptr m_callback;
    
    // Configuration
    ui_element_config::ptr m_config;
    
    // Child components
    std::unique_ptr<GridWindow> m_window;
    std::shared_ptr<AlbumDataModel> m_model;
    
    // Parent window
    HWND m_parent;
    
    // State flags
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_shutting_down{false};
};

// UI Element factory - registered with SDK
class album_grid_factory : public ui_element {
    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(album_grid_factory);
    
public:
    // ui_element methods
    void get_name(pfc::string_base& out) override {
        out = "Album Art Grid";
    }
    
    const char* get_description() override {
        return "Displays album artwork in a customizable grid layout";
    }
    
    GUID get_guid() override {
        return album_grid_guid;
    }
    
    GUID get_subclass() override {
        return ui_element_subclass_media_library_viewers;
    }
    
    void get_category(pfc::string_base& out) override {
        out = "Media Library Viewers";
    }
    
    // Create instance - CRITICAL: Use 'new' directly, not service_impl_t!
    ui_element_instance_ptr instantiate(
        HWND parent,
        ui_element_config::ptr cfg,
        ui_element_instance_callback_ptr callback
    ) override;
    
    // Default configuration
    ui_element_config::ptr get_default_configuration() override;
    
    // Configuration dialog
    bool have_config_popup() const override { return true; }
    bool show_config_popup(ui_element_config::ptr cfg, HWND parent) override;
    
    // Versioning
    t_uint32 get_flags() override {
        return ui_element::flag_dont_show_in_layout_editing_mode;
    }
};

// initquit service for global initialization/shutdown
class album_grid_initquit : public initquit {
    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(album_grid_initquit);
    
public:
    void on_init() override;
    void on_quit() override;
};

// Library viewer registration
class album_grid_library_viewer : public library_viewer {
    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(album_grid_library_viewer);
    
public:
    const char* get_name() override {
        return "Album Art Grid";
    }
    
    GUID get_guid() override {
        return album_grid_guid;
    }
    
    bool get_is_media_library_viewer() override {
        return true;
    }
    
    void activate() override;
};

// Service factory declarations
static service_factory_single_t<album_grid_factory> g_album_grid_factory;
static service_factory_single_t<album_grid_initquit> g_album_grid_initquit;
static service_factory_single_t<album_grid_library_viewer> g_album_grid_library_viewer;

} // namespace albumart_grid