# Module: LifecycleManager

## Overview
**Layer**: Foundation  
**Purpose**: Manages component initialization, shutdown, and object lifetime  
**Status**: Not Started  
**Version**: 0.0.1  
**Last Updated**: 2025-01-09  

## Responsibilities
- Component initialization sequence
- Shutdown coordination across all modules
- Reference counting management
- Object validation (magic numbers)
- Global shutdown flag management
- Resource cleanup orchestration

## Dependencies
### Internal Dependencies
- None (foundation layer)

### External Dependencies
- foobar2000 SDK: service_base, initquit
- Windows API: InterlockedIncrement/Decrement

## Interface

### Public Methods
```cpp
class LifecycleManager {
public:
    static LifecycleManager& instance();
    
    // Initialize the lifecycle manager
    // Returns: true if successful
    bool initialize();
    
    // Register a module for lifecycle management
    // module: Module to track
    // priority: Shutdown priority (higher = shutdown first)
    void register_module(ILifecycleAware* module, int priority);
    
    // Begin shutdown sequence
    void begin_shutdown();
    
    // Check if shutting down
    bool is_shutting_down() const;
    
    // Validate object is alive and safe to use
    bool validate_object(const validated_object* obj) const;
};

// Interface for modules that need lifecycle management
class ILifecycleAware {
public:
    virtual void on_initialize() = 0;
    virtual void on_shutdown() = 0;
    virtual bool is_valid() const = 0;
};

// Base class for validated objects
class validated_object {
protected:
    std::atomic<uint64_t> m_magic{GRID_MAGIC_VALID};
    std::atomic<bool> m_destroyed{false};
    
public:
    bool is_valid() const;
    void invalidate();
};
```

## Implementation Notes

### Critical Sections
- **Shutdown sequence**: Must unregister callbacks before destroying objects
- **Reference counting**: Never delete if refcount > 0
- **Object validation**: Always check before pointer dereference

### Error Handling
- **Invalid object access**: Return safe default, log error
- **Shutdown during operation**: Abort operation gracefully
- **Memory corruption**: SEH exception handling

### Performance Considerations
- Magic number checks are atomic operations (minimal overhead)
- Shutdown flag check is inlined for speed

## Testing

### Test Coverage
- [ ] Normal initialization
- [ ] Shutdown during initialization
- [ ] Multiple shutdown calls
- [ ] Object validation with corrupted memory
- [ ] Module registration order

### Known Issues
- Previous versions crashed due to improper shutdown sequence

## Change Log
| Date | Version | Change | Author |
|------|---------|--------|--------|
| 2025-01-09 | 0.0.1 | Initial documentation | System |