# Album Art Grid for foobar2000

A component that displays album artwork in a customizable grid layout for foobar2000.

## Version 10.0.9 (2025-08-26)

### Download
- **[Release v10.0.9](https://github.com/veselyvaclavcz/foobar-grid-view/releases/latest)** - Download from GitHub Releases
- **[foo_albumart_grid_v10_0_9_REAL.fb2k-component](foo_albumart_grid_v10_0_9_REAL.fb2k-component)** - Component package

### Latest Changes (v10.0.9) - MEMORY CORRUPTION FIX + NEW FEATURE
- **🔥 CRITICAL F9FCh MEMORY FIX**: Fixed memory corruption crash during shutdown
  - Added atomic `shutdown_in_progress` flag to prevent race conditions
  - Safe cleanup sequence in `thumbnail_cache::clear_all()` method
  - Protected `add_thumbnail` operations during shutdown
  - Exception handling during shutdown to prevent access violations
- **🆕 NEW FEATURE**: "Open in Folder" context menu option
  - Right-click any album → "Open in Folder" to open its directory in Windows Explorer
  - Automatically extracts folder path from album tracks
  - Fallback handling for maximum compatibility
- **✅ STABILITY**: All previous v10.0.8 callback validity fixes included

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

### Installation (v10.0.9)
**⚠️ IMPORTANT: Use the safe installation method to prevent crashes**

#### Recommended Installation
1. **CLOSE foobar2000 completely** (very important!)
2. Double-click `foo_albumart_grid_v10_0_9_REAL.fb2k-component` 
3. Click "Yes" or "Apply" when prompted to install
4. **Restart foobar2000** (mandatory for proper initialization)
5. Add "Album Art Grid" to your layout (View → Layout → Edit Layout)
6. Verify "Album Art Grid v10.0.9 initialized - F9FCh memory fix applied" appears in console

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

### Source Code (v10.0.9)
The component includes the following source files:
- `grid_v10_0_9_working.cpp` - Main grid implementation with F9FCh memory fix and Open in Folder feature
- `initquit_v10_0_9.cpp` - Initialization and shutdown handling with memory corruption protection
- `foo_albumart_grid_v10_0_9_REAL.fb2k-component` - Ready-to-install component package

### Building from Source
Requires:
- Visual Studio 2022 Build Tools
- foobar2000 SDK (2025-03-07)
- Windows SDK 10.0.26100.0

### Known Issues  
- None reported in v10.0.9
- v10.0.9 FIXES the critical F9FCh memory corruption crash during shutdown
- v10.0.9 includes all previous callback validity fixes from v10.0.8
- v10.0.9 adds new "Open in Folder" feature for enhanced usability

### Support
Report issues at: https://github.com/veselyvaclavcz/foobar-grid-view/issues

### Credits
Created with assistance from Anthropic's Claude AI

### License
Public Domain

---
*Tested with foobar2000 v2.1.6 x64*