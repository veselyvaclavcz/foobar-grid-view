# Album Art Grid for foobar2000

A component that displays album artwork in a customizable grid layout for foobar2000.

## Version 10.0.3 (2025-08-23)

### Download
- **[Release v10.0.3](https://github.com/veselyvaclavcz/foobar-grid-view/releases/latest)** - Download from GitHub Releases
- **[foo_albumart_grid.dll](foo_albumart_grid.dll)** - DLL only (for manual installation)

### Latest Changes (v10.0.3)
- **CRITICAL FIX**: Fixed metadata extraction errors that prevented album art from loading
- **Fixed**: Problematic library paths no longer break the entire album view
- **Improved**: Better error handling when processing library items
- **Enhanced**: Graceful fallback to folder names when metadata extraction fails
- **Maintained**: All features from v10.0.2 including GDI+ fixes and v10.0.1 stability improvements

### Installation
1. Close foobar2000 if running
2. Double-click the `.fb2k-component` file
3. Click "Yes" or "Apply" when prompted to install
4. Restart foobar2000
5. Add "Album Art Grid" to your layout (View → Layout → Edit Layout)

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

### Source Code
The component includes the following source files:
- `grid_v999_final.cpp` - Main grid implementation
- `initquit_v999_fixed.cpp` - Initialization and shutdown handling
- `component_version.rc` - Version resource information

### Building from Source
Requires:
- Visual Studio 2022 Build Tools
- foobar2000 SDK (2025-03-07)
- Windows SDK 10.0.26100.0

### Known Issues
- None reported in v10.0.1

### Support
Report issues at: https://github.com/veselyvaclavcz/foobar-grid-view/issues

### Credits
Created with assistance from Anthropic's Claude AI

### License
Public Domain

---
*Tested with foobar2000 v2.1.6 x64*