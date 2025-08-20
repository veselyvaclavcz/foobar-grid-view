# Album Art Grid Component for foobar2000

**Version 9.9.6** - A visual grid component that displays album artwork from your music library in foobar2000 with Now Playing tracking.

## Features

### Core Features
- **Album Art Grid Display**: Shows album covers in a customizable grid layout
- **Multiple View Modes**: Switch between Media Library and Active Playlist views
- **Smart Grouping**: Automatically groups tracks by album
- **Dark Mode Support**: Follows foobar2000's theme settings

### Now Playing Features (v9.9.6)
- **Visual Now Playing Indicator**:
  - Blue border highlights the currently playing album
  - Small play icon in bottom-left corner of the album art
- **Jump to Now Playing**:
  - Press `Ctrl+Q` to instantly scroll to the currently playing album
  - Available via context menu
- **Auto-scroll to Now Playing**:
  - Optional automatic scrolling when track changes
  - Toggle via context menu
  - Smart detection - won't scroll if you've manually scrolled recently

### Interactive Features
- **Flexible Column Control** (v9.9.0):
  - Support for 1-20 columns (previously limited to 3-10)
  - Adjust columns via mouse wheel (Ctrl+Scroll) or context menu
  - Synchronized limits between all control methods
- **Track Sorting for Playlists** (v9.9.0):
  - Sort tracks by Track Number (default)
  - Sort tracks by Title (alphabetical)
  - No Sorting option (preserve original order)
  - Applies to all playlist operations (double-click, menu actions)
- **Configurable Double-Click Actions**:
  - Play album
  - Add to current playlist
  - Add to new playlist
- **Real-time Search/Filter** (v9.8.8):
  - Press `Ctrl+Shift+S` to toggle search box
  - Filter albums by artist, album name, or genre
  - Real-time filtering as you type
  - Search box automatically clears when closed
- **Context Menu**: Right-click for quick actions and configuration

### Visual Features
- **Optimized Rendering**: Efficient GDI+ image handling
- **Automatic Refresh**: Updates when library or playlist changes
- **Responsive Layout**: Adjusts grid based on window size

## Installation

1. Download `foo_albumart_grid_v996.fb2k-component`
2. Double-click the file to install in foobar2000
3. Or manually copy to your foobar2000 components folder

## Usage

### Adding to Layout
1. In foobar2000, go to View → Layout → Configure
2. Right-click in your layout
3. Choose "Add New UI Element" → "Album Art Grid"

### Keyboard Shortcuts
- `Ctrl+Shift+S`: Toggle search/filter box
- `Ctrl+Mouse Wheel`: Adjust number of columns (1-20)
- `Double-Click`: Perform configured action (Play/Add)

### Configuration
Right-click in the grid to access:
- **Columns**: Choose from 1-20 columns for the grid display
- **Track Sorting**: Configure how tracks are sorted when added to playlists
- **Double-Click Action**: Configure what happens when you double-click an album
- **View Mode**: Toggle between Media Library and Playlist views
- **Search**: Open the search/filter box

## Version History

### v9.9.6 (Latest) - Now Playing Feature
- **New**: Visual Now Playing indicator with blue border
- **New**: Small play icon in bottom-left corner of playing album
- **New**: Jump to Now Playing with Ctrl+Q keyboard shortcut
- **New**: Context menu option for Jump to Now Playing
- **New**: Auto-scroll to Now Playing toggle option
- **Fixed**: Crash prevention with comprehensive safety checks
- **Improved**: Timer-based now playing detection

### v9.9.3 - Memory Optimization
- **New**: Smart LRU cache management without timers
- **New**: Adaptive memory limits (25% of available RAM)
- **Fixed**: Eliminated unwanted grid refreshes
- **Improved**: Better memory management and performance

### v9.9.0 - Flexible Columns & Track Sorting
- **New**: Unlimited column flexibility (1-20 columns instead of 3-10)
- **New**: Track sorting options for playlist operations
  - By Track Number (default)
  - By Title (alphabetical)
  - No Sorting (preserve original order)
- **New**: Track Sorting submenu in context menu
- **Enhanced**: Mouse wheel and menu column controls now synchronized
- **Enhanced**: All playlist operations now respect track sorting preference

### v9.8.10 - Critical Crash Fixes
- Fixed: Crash when using "Add to Current Playlist" multiple times
- Fixed: Race condition during playlist refresh operations
- Fixed: Menu Play now preserves playlist in playlist view mode
- Fixed: Menu Play searches for corresponding tracks in playlist (matches double-click)

### v9.8.9 - Bug Fixes
- Fixed: Double-click in Playlist view no longer clears the playlist
- Fixed: Double-click on filtered items now plays the correct album
- Fixed: Hover tooltips now show correct items when search filter is active
- Added: "Playlist is empty" message for empty playlists

### v9.8.8
- Search box now clears when closed
- Improved focus handling for reliable keyboard shortcuts

### v9.8.7
- Fixed focus issues with search shortcuts
- Improved toggle behavior

### v9.8.6
- Changed search shortcut to `Ctrl+Shift+S` (avoids conflicts)
- Made search a proper toggle (open/close with same shortcut)

### v9.8.5
- Initial search/filter functionality
- Real-time filtering of albums

### v9.8.2
- Added configurable double-click actions
- Added Double-Click Action submenu

### v9.8.1
- Enhanced playlist view support
- Auto-refresh on playlist changes

### v9.8.0
- Initial release with core grid functionality

## Building from Source

### Prerequisites
- Visual Studio 2022 Build Tools or Community Edition
- foobar2000 SDK (included in SDK-2025-03-07 folder)
- Windows SDK

### Build Instructions
1. Clone this repository
2. Ensure Visual Studio Build Tools are installed
3. Run `BUILD_V99_FINAL_FIXED.bat`
4. The compiled component will be created as `foo_albumart_grid_v996.fb2k-component`

### Source Files
- `grid_v99_minimal.cpp` - Main component source (v9.9.6 with Now Playing feature)
- `BUILD_V99_FINAL_FIXED.bat` - Build script for v9.9.6
- `SDK-2025-03-07/` - Required foobar2000 SDK
- `component_client.cpp` - Component entry point from SDK
- `helpers_minimal.cpp` - Helper functions for SDK compatibility
- `initquit_service.cpp` - GDI+ initialization service

## Technical Details

- **Language**: C++17
- **SDK Version**: foobar2000 SDK 2025-03-07
- **Target Version**: foobar2000 v2.0+
- **Dependencies**: Windows GDI+, foobar2000 SDK

## Known Issues

- None currently reported in v9.9.6

## Contributing

Feel free to submit issues or pull requests for bug fixes and improvements.

## License

This component is provided as-is for use with foobar2000.

## Author

Developed for the foobar2000 community.

---

For support or questions, please open an issue on GitHub.