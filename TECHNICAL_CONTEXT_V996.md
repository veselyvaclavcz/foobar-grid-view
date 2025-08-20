# Technical Context Document - Album Art Grid v9.9.6

## Component Architecture Deep Dive

### Class Hierarchy
```cpp
album_grid_instance : public ui_element_instance, public playlist_callback_single
├── Window Management
├── Grid Item Management  
├── Thumbnail Caching
├── Event Handling
└── Now Playing Tracking
```

### Critical Data Structures

#### grid_item Structure
```cpp
struct grid_item {
    metadb_handle_list tracks;        // All tracks in this album
    pfc::string8 display_name;        // Album or folder name
    pfc::string8 sort_key;            // For sorting
    pfc::string8 artist;              // Artist name
    pfc::string8 album;               // Album name
    pfc::string8 genre;               // Genre
    pfc::string8 year;                // Year
    std::shared_ptr<thumbnail_data> thumbnail;  // Artwork
    t_filetimestamp newest_date;      // For date sorting
    int rating;                        // Average rating
    t_filesize total_size;            // Total size of tracks
}
```

#### thumbnail_data Structure
```cpp
struct thumbnail_data {
    Gdiplus::Bitmap* bitmap;
    DWORD last_access;     // For LRU tracking
    bool loading;          // Loading state
    int cached_size;       // Size when cached
    size_t memory_size;    // Actual memory usage
}
```

### Memory Management Implementation

#### Cache Singleton
```cpp
class thumbnail_cache {
    static std::map<pfc::string8, std::shared_ptr<thumbnail_data>> cache;
    static size_t total_memory;
    static size_t max_memory;
    
    // Adaptive memory calculation
    static size_t get_available_memory() {
        MEMORYSTATUSEX memInfo;
        GlobalMemoryStatusEx(&memInfo);
        size_t quarter_available = memInfo.ullAvailPhys / 4;
        return max(MIN_CACHE_SIZE, min(MAX_CACHE_SIZE, quarter_available));
    }
}
```

### Now Playing Implementation Details

#### Key Member Variables
```cpp
// Now Playing tracking
metadb_handle_ptr m_now_playing;     // Current playing track
int m_now_playing_index;              // Index in grid (-1 if not found)
bool m_highlight_now_playing;         // Show visual indicator
bool m_auto_scroll_to_now_playing;    // Auto-scroll enabled
DWORD m_last_user_scroll;             // Track manual scrolling
```

#### Detection Flow
1. **Timer Tick** (every 1 second) → `on_timer(TIMER_NOW_PLAYING)`
2. **Check Now Playing** → `check_now_playing()`
   - Validates window and items exist
   - Queries playback_control for current track
3. **Update State** → `update_now_playing(track)`
   - Finds album containing track
   - Updates index
   - Triggers auto-scroll if enabled
4. **Visual Update** → `draw_item()`
   - Checks `is_now_playing` flag
   - Draws blue border and play icon

### Critical Functions

#### find_track_album Algorithm
```cpp
int find_track_album(metadb_handle_ptr track) {
    // 1. Direct handle comparison (exact match)
    for each item:
        for each track in item:
            if (track == item_track) return index;
    
    // 2. Metadata comparison (fallback)
    get album/artist from track
    for each item:
        if (item.album == album && item.artist == artist)
            return index;
    
    return -1;  // Not found
}
```

#### jump_to_now_playing Logic
```cpp
void jump_to_now_playing() {
    // Safety checks
    if (index < 0 || !window || items.empty()) return;
    
    // Calculate position
    row = index / columns;
    target_scroll = row * (item_height + padding);
    
    // Center if possible
    target_scroll -= viewport_height / 2;
    
    // Update and refresh
    m_scroll_pos = target_scroll;
    update_scrollbar();
    InvalidateRect();
}
```

### Event Handling Flow

#### Window Message Processing
```
WM_CREATE → initialize_window()
WM_PAINT → on_paint()
WM_SIZE → on_size()
WM_MOUSEWHEEL → on_mousewheel()
WM_LBUTTONDOWN → on_lbuttondown()
WM_RBUTTONDOWN → on_rbuttondown()
WM_KEYDOWN → on_keydown()
WM_TIMER → on_timer()
WM_CONTEXTMENU → on_contextmenu()
```

#### Playlist Callbacks
```
on_items_added() → refresh if playlist view
on_items_removed() → refresh if playlist view
on_items_reordered() → refresh if playlist view
on_playlist_switch() → refresh if playlist view
```

### Build System Details

#### Compilation Flags
```
/O2          - Optimize for speed
/MD          - Multithreaded DLL runtime
/EHsc        - C++ exceptions
/std:c++17   - C++17 standard
/DNDEBUG     - Release build
/DUNICODE    - Unicode support
```

#### Link Dependencies
- foobar2000_SDK.lib
- pfc.lib
- shared.lib
- Windows System Libraries:
  - kernel32, user32, gdi32
  - comctl32, uxtheme
  - gdiplus, shlwapi
  - shell32, ole32

### Performance Optimizations

1. **Lazy Loading**
   - Only load thumbnails for visible items
   - Progressive loading with TIMER_PROGRESSIVE

2. **Prefetching**
   - Load 20 items ahead in scroll direction
   - Buffer zone of 50 items around viewport

3. **Image Scaling**
   - Thumbnails scaled once and cached
   - High-quality bilinear interpolation
   - PixelFormat32bppPARGB for speed

4. **Drawing Optimizations**
   - Double buffering to prevent flicker
   - Clip rectangle to draw only visible items
   - GDI+ for high-quality rendering

### Known Technical Constraints

1. **foobar2000 SDK Limitations**
   - Must use specific service patterns
   - UI element lifecycle management
   - Thread safety requirements

2. **Windows GDI+ Quirks**
   - Memory management for Bitmap objects
   - Stream handling for image loading
   - Dark mode compatibility

3. **Component Boundaries**
   - Cannot directly access playback events (using timer instead)
   - Limited to UI element sandbox
   - Must respect foobar2000's threading model

### Debug & Troubleshooting

#### Common Issues & Solutions

1. **High Memory Usage**
   - Check: Cache size limits
   - Solution: Reduce MAX_CACHE_SIZE_MB

2. **Slow Scrolling**
   - Check: Prefetch settings
   - Solution: Increase PREFETCH_AHEAD

3. **Missing Now Playing**
   - Check: Timer running
   - Solution: Verify TIMER_NOW_PLAYING active

4. **Crash on Exit**
   - Check: Cleanup in destructor
   - Solution: Ensure all GDI+ objects deleted

#### Debugging Helpers
```cpp
// Console output for debugging
console::printf("Album Grid: %s", message);

// Crash info collection
FB2K_ADD_CRASH_INFO("Album Grid", "v9.9.6");
```

### Future Development Notes

#### Potential Improvements
1. **Event-Driven Now Playing**
   - Research: play_callback_static alternatives
   - Consider: Service factory pattern

2. **Animated Transitions**
   - Smooth scrolling to now playing
   - Fade effects for artwork loading

3. **Custom Rendering**
   - Direct2D for better performance
   - Hardware acceleration support

4. **Configuration UI**
   - Preferences page for all settings
   - Color customization
   - Layout templates

#### Code Areas Needing Refactor
1. `draw_item()` - Too long, should be split
2. `refresh_items()` - Could be optimized for incremental updates
3. Error handling - Should use RAII consistently
4. Magic numbers - Should be configurable constants

### Testing Procedures

#### Unit Test Areas
- Memory limit enforcement
- LRU eviction logic
- Track to album matching
- Scroll position calculations

#### Integration Tests
- Large library handling (10,000+ albums)
- Rapid scrolling performance
- Memory leak detection
- Multi-instance support

#### User Acceptance Tests
- Visual indicator visibility
- Keyboard shortcut responsiveness
- Auto-scroll behavior
- Context menu functionality

---

**Document Version:** 1.0  
**Component Version:** 9.9.6  
**Last Updated:** August 20, 2025

This document serves as a technical reference for maintaining and extending the Album Art Grid component.