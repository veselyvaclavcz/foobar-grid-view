# Album Art Grid for foobar2000

A component that displays album artwork in a customizable grid layout for foobar2000.

## Version 10.0.5 (2025-08-24)

### Download
- **[Release v10.0.5](https://github.com/veselyvaclavcz/foobar-grid-view/releases/latest)** - Download from GitHub Releases
- **[foo_albumart_grid_v10_0_5_shutdown_fix.fb2k-component](foo_albumart_grid_v10_0_5_shutdown_fix.fb2k-component)** - Component package

### Latest Changes (v10.0.5) - CRITICAL SHUTDOWN FIX
- **🔥 CRITICAL FIX**: Fixed crash during foobar2000 shutdown in `uPrintCrashInfo_OnEvent` 
- **🔥 CRITICAL FIX**: Fixed memory corruption during version switching
- **🛡️ SECURITY**: Added atomic shutdown flags to prevent access during cleanup
- **🛡️ SECURITY**: Implemented critical section protection during shutdown sequence
- **🔧 IMPROVED**: Enhanced thread-safe resource cleanup (GDI+, helpers, events)  
- **🔧 IMPROVED**: Protected all helper functions against shutdown access
- **📦 NEW**: Safe installation script with mandatory foobar2000 restart
- **📖 NEW**: Complete technical documentation of the shutdown fix
- **✅ MAINTAINED**: All features from v10.0.4 including 4 display labels and crash protection

### Installation (v10.0.5)
**⚠️ IMPORTANT: Use the safe installation method to prevent crashes**

#### Option A: Safe Installation (Recommended)
1. Run `INSTALL_V10_0_5_SAFE.bat` 
2. Script will close foobar2000 automatically if running
3. Follow prompts to complete installation  
4. Verify "Album Art Grid v10.0.5 initialized" appears in foobar2000 console

#### Option B: Manual Installation
1. **CLOSE foobar2000 completely** (very important!)
2. Double-click `foo_albumart_grid_v10_0_5_shutdown_fix.fb2k-component` 
3. Click "Yes" or "Apply" when prompted to install
4. **Restart foobar2000** (mandatory for v10.0.5 to load correctly)
5. Add "Album Art Grid" to your layout (View → Layout → Edit Layout)

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
  - Right-click context menu
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

### Source Code (v10.0.5)
The component includes the following source files:
- `grid_v10_0_5_shutdown_fix.cpp` - Main grid implementation with v10.0.5 version
- `initquit_v10_0_5.cpp` - Enhanced initialization and shutdown handling with critical sections
- `helpers_minimal_v10_0_5.cpp` - Protected helper functions with atomic shutdown flags
- `BUILD_V10_0_5_SHUTDOWN_FIX.bat` - Build script for v10.0.5
- `INSTALL_V10_0_5_SAFE.bat` - Safe installation script
- `VERSION_SWITCHING_FIX.md` - Technical documentation of shutdown fixes

### Building from Source
Requires:
- Visual Studio 2022 Build Tools
- foobar2000 SDK (2025-03-07)
- Windows SDK 10.0.26100.0

### Known Issues  
- None reported in v10.0.5
- v10.0.5 FIXES the critical shutdown crash from previous versions
- v10.0.5 FIXES memory corruption during version switching

### Support
Report issues at: https://github.com/veselyvaclavcz/foobar-grid-view/issues

### Credits
Created with assistance from Anthropic's Claude AI

### License
Public Domain

---
*Tested with foobar2000 v2.1.6 x64*