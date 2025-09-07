# Module: CallbackManager

## Overview
**Layer**: Foundation  
**Purpose**: Safe management of all SDK callbacks with weak references  
**Status**: Not Started  
**Version**: 0.0.1  
**Last Updated**: 2025-01-09  

## Responsibilities
- Register/unregister SDK callbacks safely
- Weak pointer management for callbacks
- Prevent use-after-free crashes
- Handle callbacks during shutdown
- Exception protection for all callback invocations

## Dependencies
### Internal Dependencies
- LifecycleManager: For shutdown detection

### External Dependencies
- foobar2000 SDK: playlist_callback, library_callback, play_callback
- Windows API: SEH exception handling

## Interface

### Public Methods
```cpp
class CallbackManager {
public:
    static CallbackManager& instance();
    
    // Register a playlist callback
    // callback: Weak reference to callback handler
    // Returns: Registration token for unregistering
    callback_token register_playlist_callback(
        std::weak_ptr<playlist_callback_single> callback
    );
    
    // Register a library callback  
    callback_token register_library_callback(
        std::weak_ptr<library_callback> callback
    );
    
    // Unregister a callback
    void unregister(callback_token token);
    
    // Unregister all callbacks for an object
    void unregister_all_for_object(void* object);
    
    // Execute callback safely with exception protection
    template<typename Func>
    void safe_execute(Func&& func);
    
private:
    // Validate callback is still alive
    bool validate_callback(void* ptr) const;
};

// Token for callback registration
class callback_token {
    friend class CallbackManager;
    uint64_t m_id;
    callback_type m_type;
};
```

## Implementation Notes

### Critical Sections
- **Callback execution**: Must validate object before calling
- **Unregistration**: Must happen before object destruction
- **Exception handling**: All callbacks wrapped in SEH

### Error Handling
- **Dead callback**: Skip execution, remove from registry
- **Exception in callback**: Catch, log, continue
- **Shutdown race**: Check shutdown flag before execution

### Performance Considerations
- Weak pointer checks add minimal overhead
- Exception handlers only on stack during callback

## Testing

### Test Coverage
- [ ] Register/unregister callbacks
- [ ] Callback execution with valid object
- [ ] Callback execution with destroyed object
- [ ] Exception in callback handler
- [ ] Mass unregistration during shutdown
- [ ] Concurrent callback execution

### Known Issues
- Previous versions crashed from callbacks on destroyed objects

## Change Log
| Date | Version | Change | Author |
|------|---------|--------|--------|
| 2025-01-09 | 0.0.1 | Initial documentation | System |