# Album Art Grid Component for foobar2000

**Version 9.8.9** - A visual grid component that displays album artwork from your music library in foobar2000.

## Features

### Core Features
- **Album Art Grid Display**: Shows album covers in a customizable grid layout
- **Multiple View Modes**: Switch between Media Library and Active Playlist views
- **Smart Grouping**: Automatically groups tracks by album
- **Dark Mode Support**: Follows foobar2000's theme settings

### Interactive Features
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

1. Download `foo_albumart_grid_v989.fb2k-component`
2. Double-click the file to install in foobar2000
3. Or manually copy to your foobar2000 components folder

## Usage

### Adding to Layout
1. In foobar2000, go to View → Layout → Configure
2. Right-click in your layout
3. Choose "Add New UI Element" → "Album Art Grid"

### Keyboard Shortcuts
- `Ctrl+Shift+S`: Toggle search/filter box
- `Double-Click`: Perform configured action (Play/Add)

### Configuration
Right-click in the grid to access:
- **Double-Click Action**: Configure what happens when you double-click an album
- **Switch View**: Toggle between Media Library and Playlist views
- **Search**: Open the search/filter box

## Version History

### v9.8.9 (Latest) - Critical Bug Fixes
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
3. Run `BUILD_V98_SEARCH.bat`
4. The compiled component will be created as `foo_albumart_grid_v98.fb2k-component`

### Source Files
- `grid_v98_playlist_fixed.cpp` - Main component source
- `BUILD_V98_SEARCH.bat` - Build script
- `SDK-2025-03-07/` - Required foobar2000 SDK

## Technical Details

- **Language**: C++17
- **SDK Version**: foobar2000 SDK 2025-03-07
- **Target Version**: foobar2000 v2.0+
- **Dependencies**: Windows GDI+, foobar2000 SDK

## Known Issues

- Search currently filters by partial text match (case-insensitive)
- Some keyboard shortcuts may conflict with foobar2000's built-in shortcuts

## Contributing

Feel free to submit issues or pull requests for bug fixes and improvements.

## License

This component is provided as-is for use with foobar2000.

## Author

Developed for the foobar2000 community.

---

For support or questions, please open an issue on GitHub.