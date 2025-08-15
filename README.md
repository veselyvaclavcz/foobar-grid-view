# Album Art Grid Component for foobar2000

A high-performance album art grid view component for foobar2000 music player with extensive customization options.

![Version](https://img.shields.io/badge/version-9.5.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey)
![foobar2000](https://img.shields.io/badge/foobar2000-v2.0+-green)

## Features

### 🎤 NEW in v9.5: Artist Image Support
- **Smart image selection** - displays artist images when grouping by artist
- **Automatic fallback** - shows album art if no artist image available
- **Works with** embedded images and folder-based artist images

### 🎨 Visual Display
- **High-quality album art rendering** with bicubic interpolation
- **Auto-fill mode** - automatically adjusts grid to container width
- **Dynamic column adjustment** (3-10 columns) via Ctrl+Mouse Wheel
- **Dark mode support** with theme-aware UI elements
- **Smart tooltips** - only shown when text labels are hidden

### 📚 Organization
- **13 Grouping Modes**: Folder, Directory, Album, Artist, Album Artist, Artist/Album, Performer, Composer, Genre, Year, Label, Rating, Comment
- **11 Sorting Options**: Name, Artist, Album, Year, Genre, Date Modified, Total Size, Track Count, Rating, Path, Random/Shuffle
- **Flexible combination** - group by one criterion, sort by another

### 🎯 Interaction
- **Fixed selection alignment** - click accuracy guaranteed
- **Multi-selection support** with Shift+Click and Ctrl+Click
- **Double-click to play**
- **Context menu** with play, add to playlist, and configuration options
- **Layout edit mode integration** - proper cut/copy/paste support

### ⚙️ Customization
- **Text display options** - 1 to 3 lines of text
- **Show/hide track counts**
- **Font integration** - uses foobar2000's font preferences
- **Persistent settings** - remembers your preferences

## Installation

1. Download `foo_albumart_grid_v95.dll` from the [Releases](https://github.com/veselyvaclavcz/foobar-grid-view/releases) page
2. Install the component using one of these methods:
   - **Recommended**: Use foobar2000 > File > Preferences > Components > Install... (you can also drag-n-drop the file here)
   - Or manually copy the DLL file to your foobar2000 components folder
3. Restart foobar2000
4. Add the component to your layout:
   - View > Layout > Edit Layout
   - Right-click in layout editor > Replace UI Element... > Album Art Grid

## Usage

### Basic Controls
- **Ctrl + Mouse Wheel**: Adjust number of columns (3-10)
- **Double-click**: Play album/folder
- **Right-click**: Open context menu for options
- **Shift + Click**: Select range
- **Ctrl + Click**: Toggle individual selection

### Context Menu Options

#### Grouping
Choose how to organize your music:
- By Folder (full path)
- By Directory (parent folder name)
- By Album, Artist, Album Artist, etc.
- By metadata (Genre, Year, Label, Rating)

#### Sorting
Order your groups by:
- Alphabetical (Name, Artist, Album)
- Chronological (Year, Date Modified)
- Size metrics (Total Size, Track Count)
- Random shuffle

#### Display Options
- Columns: 3 to 10
- Text Lines: 1 to 3
- Show/Hide text labels
- Show/Hide track counts

## Technical Details

### Requirements
- foobar2000 v2.0 or newer
- Windows x64
- Visual C++ 2022 Redistributables

### Build Information
- SDK: foobar2000 SDK v2 (2025-03-07)
- Compiler: Microsoft Visual C++ 2022
- C++ Standard: C++17
- Dependencies: GDI+, Windows Common Controls

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/veselyvaclavcz/foobar-grid-view.git
```

2. Download foobar2000 SDK and place in `SDK-2025-03-07` folder

3. Open Visual Studio 2022 x64 Native Tools Command Prompt

4. Run the build script:
```bash
BUILD_V95_FROM_94.bat
```

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for detailed version history and improvements.

## Known Issues

- Custom pattern grouping temporarily removed (implementation complexity)
- Playlist-based grouping not available (architectural limitation)

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

This project is based on the foobar2000 SDK and follows its licensing terms.

## Credits

- Created with assistance from Anthropic's Claude AI
- Based on original concepts by marc2003
- Built with foobar2000 SDK by Peter Pawlowski

## Support

For issues, questions, or suggestions, please:
- Open an issue on [GitHub](https://github.com/yourusername/foo_albumart_grid/issues)
- Visit the [foobar2000 forum thread](#) (if applicable)

---

**Note**: This component is a third-party extension for foobar2000 and is not officially affiliated with or endorsed by foobar2000 or Resolute Ltd.