# Module: GridWindow

## Overview
**Layer**: UI  
**Purpose**: Main window management, rendering, and user interaction  
**Status**: Not Started  
**Version**: 0.0.1  
**Last Updated**: 2025-01-09  

## Responsibilities
- Window creation and management
- Grid layout calculation
- Mouse/keyboard input handling
- Scroll management
- Selection management
- Rendering coordination
- Context menu display

## Dependencies
### Internal Dependencies
- AlbumDataModel: For data source
- ThumbnailRenderer: For image rendering
- GridLayoutEngine: For layout calculations
- CallbackManager: For UI callbacks

### External Dependencies
- Windows API: Window management, GDI+
- foobar2000 SDK: ui_element_instance, contextmenu_manager

## Interface

### Public Methods
```cpp
class GridWindow : public ILifecycleAware {
public:
    // Create window
    HWND create(HWND parent);
    
    // Destroy window
    void destroy();
    
    // Set data model
    void set_model(std::shared_ptr<AlbumDataModel> model);
    
    // Set column count (0 = auto)
    void set_columns(int count);
    
    // Get current selection
    metadb_handle_list get_selection() const;
    
    // Scroll to item
    void scroll_to_item(int index, bool center = false);
    
    // Scroll to now playing
    void scroll_to_now_playing();
    
protected:
    // Window procedure
    static LRESULT CALLBACK window_proc(HWND wnd, UINT msg, 
                                       WPARAM wp, LPARAM lp);
    
    // Message handlers
    void on_paint(HDC dc);
    void on_size(int width, int height);
    void on_mouse_move(int x, int y);
    void on_mouse_down(int button, int x, int y);
    void on_mouse_up(int button, int x, int y);
    void on_mouse_wheel(int delta);
    void on_key_down(UINT key);
    void on_context_menu(int x, int y);
    
private:
    // Hit testing
    int hit_test(int x, int y) const;
    
    // Selection management
    void select_item(int index, bool toggle, bool range);
    void select_all();
    void clear_selection();
};
```

## Implementation Notes

### Critical Sections
- **WM_DESTROY**: Must unregister callbacks before destruction
- **Painting**: Must validate all objects before rendering
- **Input handling**: Check shutdown flag before processing

### Error Handling
- **Invalid HDC**: Skip painting
- **Window creation failure**: Return nullptr, log error
- **Exception in paint**: Catch and skip frame

### Performance Considerations
- Only paint visible items (viewport culling)
- Cache layout calculations
- Batch invalidation regions
- Use double buffering for smooth scrolling

## Testing

### Test Coverage
- [ ] Window creation/destruction
- [ ] Resizing behavior
- [ ] Mouse selection (click, ctrl+click, shift+click)
- [ ] Keyboard navigation
- [ ] Scroll behavior
- [ ] Context menu
- [ ] High DPI support

### Known Issues
- None yet (new implementation)

## Change Log
| Date | Version | Change | Author |
|------|---------|--------|--------|
| 2025-01-09 | 0.0.1 | Initial documentation | System |