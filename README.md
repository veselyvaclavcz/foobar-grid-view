# Album Art Grid for foobar2000

A component that displays album artwork in a customizable grid layout for foobar2000.

## Version 10.0.15 (2025-08-29) - SMART LETTER JUMP NAVIGATION

### Download
- **[Release v10.0.15](https://github.com/veselyvaclavcz/foobar-grid-view/releases/latest)** - Download from GitHub Releases
- **[foo_albumart_grid_v10_0_15.fb2k-component](foo_albumart_grid_v10_0_15.fb2k-component)** - Component package

### Latest Changes (v10.0.15) - SMART LETTER JUMP NAVIGATION
- **🎯 INTELLIGENT LETTER JUMP NAVIGATION**: Press A-Z or 0-9 to quickly navigate your library
  - **Smart Jump Logic**: Jumps based on what's actually displayed
    - When showing "Artist - Album" → jumps by Artist name
    - When showing "Album" only → jumps by Album name
    - When showing "Folder Name" → jumps by Folder name
  - **Cycle Through Matches**: Press the same letter again to jump to next item with same starting letter
  - **Visual Feedback**: Automatically selects jumped-to item for clear visibility
  - **Non-intrusive**: Only activates with plain key press (no Ctrl/Alt/Shift modifiers)
  - **Wrap-around**: After last match, cycles back to first match
- **✅ Maintains ALL previous features and fixes from v10.0.14**

### Previous Changes (v10.0.14) - CRASH-FREE SHUTDOWN
- **🛡️ SMART SHUTDOWN PROTECTION**: Prevents crashes without blocking multi-instance support
  - Instance-specific shutdown detection (no global flags)
  - SEH exception handling for GDI+ shutdown
  - Fixed function naming issues (initiate_app_shutdown)
  - Proper component packaging (.fb2k-component format)

### Previous Changes (v10.0.13) - LIBRARY VIEWER INTEGRATION
- **🔧 LIBRARY MENU INTEGRATION**: Component now appears in foobar2000's Library menu system
  - Added `library_viewer` service registration for proper Library menu integration
  - Component appears in "Installed media library viewers" list in preferences
  - Enhanced ui_element with `KFlagSupportsBump` and `KFlagHavePopupCommand` flags
  - Added `bump()` method for window activation from library menu
  - Dual-approach implementation ensures compatibility with foobar2000's library system
- **✅ Maintains ALL v10.0.12 Features**: All multi-instance fixes and stability improvements preserved

### Previous Changes (v10.0.12) - MULTI-INSTANCE FIX
- **🔧 CRITICAL FIX: Multiple grid instances can now be opened**
  - Fixed regression from v10.0.11 that prevented opening more than one grid instance
  - Shutdown protection now only activates on foobar2000 exit, not when closing individual grids
  - Users can now open/close multiple grid panels without needing to restart
- **🐛 Additional Crash Fixes**:
  - Fixed crash at offset 1196Ch in callback_run during shutdown
  - Fixed version string incorrectly reporting as "10.0.2" in console
  - Added null pointer checks in main_thread_callback
  - Safe thumbnail creation with proper error handling
- **✅ Maintains all fixes from v10.0.11**

### Previous v10.0.11 Changes - GLOBAL SHUTDOWN PROTECTION
- **🛡️ GLOBAL SHUTDOWN PROTECTION SYSTEM**: Revolutionary approach to prevent ALL window closing crashes
  - Centralized shutdown manager with atomic flags
  - Safe virtual call wrappers with SEH exception handling
  - Protected all critical paths including offsets 1181Ch and 11824h
  - Thread-safe instance tracking and cleanup
- **⚡ AUTO-ADJUSTING PERFORMANCE**: Dynamic configuration based on available RAM
  - 8GB+ RAM: 512MB cache, 20 item prefetch, 50 item buffer
  - 16GB+ RAM: 1024MB cache, 35 item prefetch, 75 item buffer
  - 32GB+ RAM: 2048MB cache, 50 item prefetch, 100 item buffer
- **🔧 COMPREHENSIVE FIXES FROM v10.0.10**:
  - Fixed infinite recursion crashes at foo_albumart_grid+207C8h
  - Fixed use-after-free crashes in playlist callbacks at 0x1162C
  - Protected album art loading operations
- **✅ ALL PREVIOUS FEATURES MAINTAINED**:
  - F9FCh memory corruption fix during shutdown
  - "Open in Folder" context menu feature
  - All callback validity fixes from v10.0.8

### Previous v10.0.7 Changes
- Fixed line 800 crash - added m_callback validity check before query_font_ex()
- Resolved persistent crash at offset 11CF4h reported by multiple users

### Previous v10.0.6 Changes
- Fixed ALL m_callback validity checks to prevent access violation crashes
- Fixed crash at address 0x0000000100000003 during shutdown
- Added validity checks at 7 critical locations
- Prevents uCallStackTracker destructor crashes

### Previous v10.0.5 Changes
- Added atomic shutdown flags and critical section protection
- Enhanced thread-safe resource cleanup
- Protected helper functions against shutdown access

### Installation (v10.0.13)
**⚠️ IMPORTANT: Use the safe installation method to prevent crashes**

#### Recommended Installation
1. **CLOSE foobar2000 completely** (very important!)
2. Double-click `foo_albumart_grid_v10_0_13_LIBRARY.fb2k-component` 
3. Click "Yes" or "Apply" when prompted to install
4. **Restart foobar2000** (mandatory for proper initialization)
5. Add "Album Art Grid" to your layout (View → Layout → Edit Layout)
6. Verify "Album Art Grid v10.0.13 initialized - Library Viewer Integration" appears in console
7. **NEW**: Check Library → Preferences → Media Library → Installed media library viewers to see "Album Art Grid"

#### Version Switching Warning
**Never replace component files while foobar2000 is running** - this causes memory corruption and crashes. Always use the safe installation script or manually close foobar2000 first.

### Features
- **Album Art Display**: Shows album covers in a responsive grid
- **Dual View Modes**: Switch between Media Library and Current Playlist views
- **Grouping Options** (13 modes):
  - Album, Artist, Artist-Album, Genre, Year, Label
  - Composer, Performer, Album Artist, Directory
  - Comment, Rating, Folder
- **Sorting Options** (11 modes):
  - Name, Date, Track Count, Artist, Album
  - Year, Genre, Path, Size, Rating, Random
- **Interactive Features**:
  - Search/filter with real-time results (Ctrl+Shift+S)
  - Configurable columns (1-20, adjust with Ctrl+Scroll)
  - Double-click actions (Play, Add to playlist)
  - Right-click context menu with "Open in Folder" (v10.0.9)
  - Track count badges
  - Now Playing indicator with blue border
  - Jump to Now Playing (Ctrl+Q)
- **Performance**:
  - Optimized memory usage (128MB cache limit)
  - Lazy loading for smooth scrolling
  - Prefetching for seamless navigation
- **Status Bar Integration**:
  - `%albumart_grid_count%` - Number of albums
  - `%albumart_grid_view%` - Current view mode
  - `%albumart_grid_info%` - Combined info

### Keyboard Shortcuts
- `Ctrl+Shift+S` - Focus search box
- `Ctrl+Mouse Wheel` - Adjust column count
- `Ctrl+Q` - Jump to now playing
- `Enter` - Play selected album
- `Delete` - Remove from playlist (playlist view only)

### What's New in v10.0.1
- **Fixed**: Component loading errors ("No version information")
- **Fixed**: Crashes during shutdown/sleep
- **Fixed**: Memory leaks and excessive RAM usage (reduced from 512MB to 128MB)
- **Fixed**: Access violations during component shutdown
- **Improved**: Thread safety and stability
- **Added**: Proper version resource information

### Requirements
- foobar2000 v2.0 or later (64-bit)
- Windows 10/11

### Source Code (v10.0.13)
The component includes the following source files:
- `grid_v10_0_13_library.cpp` - Main grid implementation with library viewer integration
- `initquit_v10_0_13.cpp` - Initialization and shutdown handling with v10.0.13 updates
- `library_viewer_impl.cpp` - Library viewer service implementation for Library menu integration
- `stdafx.cpp` - Precompiled header support
- `foo_albumart_grid_v10_0_13_LIBRARY.fb2k-component` - Ready-to-install component package

### Building from Source
Requires:
- Visual Studio 2022 Build Tools
- foobar2000 SDK (2025-03-07)
- Windows SDK 10.0.26100.0

### Known Issues  
- None reported in v10.0.13
- v10.0.13 maintains all stability fixes from v10.0.12 and earlier versions
- Library viewer integration working as designed (appears in preferences list)
- All window closing crashes fixed with comprehensive protection system
- Multi-instance support fully functional

### Support
Report issues at: https://github.com/veselyvaclavcz/foobar-grid-view/issues

### Credits
Created with assistance from Anthropic's Claude AI

### License
Public Domain

---
*Tested with foobar2000 v2.1.6 x64*