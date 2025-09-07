# Module: SDKIntegration

## Overview
**Layer**: Integration  
**Purpose**: Adapter layer between foobar2000 SDK and our modular architecture  
**Status**: Not Started  
**Version**: 0.0.1  
**Last Updated**: 2025-01-09  

## Responsibilities
- ui_element factory implementation
- Service registration with SDK
- SDK callback adaptation
- Host communication (ui_element_instance_callback)
- Configuration persistence
- Layout Editor support

## Dependencies
### Internal Dependencies
- LifecycleManager: Component lifecycle
- CallbackManager: Callback registration
- GridWindow: UI element instance

### External Dependencies
- foobar2000 SDK: All SDK interfaces
- ATL: Service registration helpers

## Interface

### Public Classes
```cpp
// UI Element factory (registered with SDK)
class album_grid_factory : public ui_element {
    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(album_grid_factory);
public:
    // ui_element methods
    void get_name(pfc::string_base& out) override;
    const char* get_description() override;
    GUID get_guid() override;
    GUID get_subclass() override;
    void get_category(pfc::string_base& out) override;
    
    // Create instance
    ui_element_instance_ptr instantiate(
        HWND parent,
        ui_element_config::ptr cfg,
        ui_element_instance_callback_ptr callback
    ) override;
    
    // Default configuration
    ui_element_config::ptr get_default_configuration() override;
};

// UI Element instance
class album_grid_instance : public ui_element_instance {
public:
    album_grid_instance(HWND parent, 
                       ui_element_config::ptr config,
                       ui_element_instance_callback_ptr callback);
    
    // ui_element_instance methods
    HWND get_wnd() override;
    void set_configuration(ui_element_config::ptr config) override;
    ui_element_config::ptr get_configuration() override;
    
    // Reference counting (CRITICAL - no service_impl_t wrapper!)
    int service_add_ref() noexcept override;
    int service_release() noexcept override;
    
protected:
    ~album_grid_instance();  // Protected destructor
    
private:
    std::atomic<LONG> m_refcount{1};
    validated_object m_validation;  // Magic number validation
};

// initquit service for shutdown coordination
class album_grid_initquit : public initquit {
    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(album_grid_initquit);
public:
    void on_init() override;
    void on_quit() override;
};
```

## Implementation Notes

### Critical Sections
- **Reference counting**: Manual implementation, NO service_impl_t wrapper
- **Service registration**: Use FB2K_MAKE_SERVICE macros correctly
- **Configuration serialization**: Handle version changes gracefully

### Error Handling
- **Invalid configuration**: Use defaults
- **Window creation failure**: Return null window
- **Callback exceptions**: Catch and log, don't propagate

### SDK Pattern Requirements
```cpp
// CORRECT service creation:
new service_impl_t<my_service>();  // Single wrapper

// WRONG (double-wrapped):
new service_impl_t<service_impl_t<my_service>>();  // CRASH!

// CORRECT instance creation:
class my_instance : public ui_element_instance {
    LONG m_refcount = 1;
    int service_add_ref() { return InterlockedIncrement(&m_refcount); }
    int service_release() { 
        LONG count = InterlockedDecrement(&m_refcount);
        if (count == 0) delete this;
        return count;
    }
};
```

### Layout Editor Support
- Detect edit mode via `callback->is_edit_mode_enabled()`
- Show "Replace Element" in context menu when in edit mode
- Support copy/paste/cut operations
- Persist configuration correctly

## Testing

### Test Coverage
- [ ] Service registration
- [ ] Instance creation/destruction
- [ ] Reference counting correctness
- [ ] Configuration save/load
- [ ] Layout Editor operations
- [ ] Shutdown sequence
- [ ] Multiple instances

### Known Issues
- Previous versions crashed due to double-wrapping services
- Improper reference counting caused double-delete

## Change Log
| Date | Version | Change | Author |
|------|---------|--------|--------|
| 2025-01-09 | 0.0.1 | Initial documentation | System |