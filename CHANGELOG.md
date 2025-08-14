# Album Art Grid Component - Changelog

## Version 9.5.0 - Artist Image Support (2025-01-14)

### New Feature

#### Artist Image Display 🎤
- **Smart Image Selection**: Component now displays artist images when using artist-based grouping modes
- **Supported Grouping Modes**:
  - By Artist
  - By Album Artist  
  - By Artist/Album
  - By Performer
  - By Composer
- **Automatic Fallback**: If no artist image is found, seamlessly falls back to album cover art
- **Works With**: Both embedded artist images and folder-based artist images configured in foobar2000
- **Visual Indicators**: Menu items show 🎤 emoji to indicate which modes support artist images

### Technical Details
- Uses foobar2000's `album_art_ids::artist` for artist image retrieval
- Graceful error handling ensures no blank spaces when images are missing
- No performance impact - same efficient loading as before

### All Features from v9.4
- 13 grouping modes
- 11 sorting options  
- Auto-fill mode with dynamic sizing
- Ctrl+Mouse Wheel column control (3-10)
- High-quality image rendering
- Smart tooltips (only when text hidden)
- Dynamic text display (1-3 lines)
- Full dark mode support

---

## Version 9.4.0 - Enhanced Sorting (Final Release)

### Major Improvements

#### 1. Fixed Selection/Highlighting Alignment ✅
- **Issue**: Clicking on items would select the wrong album
- **Solution**: Unified hit-test and drawing position calculations using consistent centering logic
- **Technical**: Both `hit_test()` and `paint()` now use identical layout calculations

#### 2. Auto-Fill Mode Implementation ✅
- **Feature**: Grid automatically adjusts item size to fill container width
- **Behavior**: Items resize based on window width and column count
- **Range**: Item sizes constrained between 50-250 pixels for optimal viewing

#### 3. Column Control via Ctrl+Mouse Wheel ✅
- **Feature**: Dynamic column adjustment from 3 to 10 columns
- **Control**: Hold Ctrl and scroll mouse wheel to adjust
- **Persistence**: Column preference saved in configuration

#### 4. Image Quality Enhancement ✅
- **Before**: Blurry, compressed-looking album art
- **After**: Crystal-clear, high-quality images
- **Technical**: 
  - Changed from `InterpolationModeDefault` to `InterpolationModeHighQualityBicubic`
  - Added `SmoothingModeHighQuality`, `PixelOffsetModeHighQuality`, `CompositingQualityHighQuality`
  - Proper aspect ratio preservation in thumbnail creation

#### 5. Dynamic Text Display ✅
- **Feature**: Flexible text display with 1-3 lines option
- **Smart Sizing**: Text height calculated from actual font metrics
- **Font Integration**: Uses foobar2000's font preferences
- **Formatting**: Proper word wrap and ellipsis for long text

#### 6. Smart Tooltips ✅
- **Behavior**: Only shows tooltips when text labels are hidden
- **Position**: Fixed positioning - consistently below items
- **Content**: Shows full album/folder name with track count

#### 7. Edit Mode Integration ✅
- **Feature**: Proper integration with foobar2000's layout editing
- **Visual**: Orange border indicates edit mode
- **Behavior**: Context menu properly passes through to host in edit mode
- **Operations**: UI element can be cut/copied/pasted like standard components

#### 8. Extended Grouping Options (13 modes) ✅
- **File-based**: By Folder, By Directory (parent folder name)
- **Artist/Album**: By Album, By Artist, By Album Artist, By Artist/Album, By Performer, By Composer
- **Metadata**: By Genre, By Year, By Label, By Rating, By Comment
- **Improvements**: Better album recognition using titleformat, proper album artist handling

#### 9. Enhanced Sorting Options (11 modes) ✅
- **Content**: By Name, By Artist, By Album, By Year, By Genre, By Rating
- **File-based**: By Date Modified, By Total Size, By Path
- **Collection**: By Track Count, Random/Shuffle
- **Flexibility**: Can group by one criterion and sort by another

#### 10. Dark Mode Support ✅
- Proper integration with foobar2000's dark mode
- Theme-aware scrollbars and UI elements

### Technical Improvements

- **Memory Management**: Proper use of smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- **Performance**: Lazy loading of album art, progressive thumbnail generation
- **Code Quality**: Clean separation of concerns, proper error handling
- **SDK Compliance**: Built with foobar2000 SDK v2 (2025-03-07)
- **Component Validation**: Proper filename validation for each version

### Bug Fixes

- Fixed component installation error ("component has been damaged")
- Fixed text truncation issues
- Fixed memory leaks in image handling
- Fixed scrollbar positioning
- Fixed multi-selection behavior
- Fixed tooltip positioning inconsistencies

### Version History

- **v8.4**: Initial fixes - selection alignment, auto-fill mode
- **v8.5**: Smart tooltips (only when text hidden)
- **v8.6**: Added "Remove from layout" option
- **v8.7-8.8**: UI element menu implementation attempts
- **v8.9**: Proper edit mode support
- **v9.0**: Complete edit mode integration
- **v9.1**: Dynamic text sizing based on font metrics
- **v9.2**: Improved album grouping, fixed tooltip positioning
- **v9.3**: Extended grouping options (14 modes including custom)
- **v9.4**: Removed custom pattern, added 11 sorting options

### Build Information

- **Compiler**: Microsoft Visual C++ 2022
- **Target**: x64
- **SDK**: foobar2000 SDK-2025-03-07
- **C++ Standard**: C++17
- **Dependencies**: GDI+, Windows Common Controls

### Installation

1. Close foobar2000
2. Copy `foo_albumart_grid_v94.dll` to your foobar2000 components folder
3. Restart foobar2000
4. Add "Album Art Grid" to your layout via View > Layout > Edit Layout

### Usage Tips

- **Ctrl+Mouse Wheel**: Adjust number of columns (3-10)
- **Right-click**: Access grouping and sorting options
- **Shift+Click**: Range selection
- **Ctrl+Click**: Toggle individual selection
- **Double-click**: Play album/folder

### Known Limitations

- Custom pattern grouping removed (too complex for current implementation)
- Playlist-based grouping not available (tracks source playlist info not retained)

### Credits

Created with assistance from Anthropic's Claude AI
Based on original concepts by marc2003

---

## Future Enhancements (Potential)

- [ ] Customizable color schemes
- [ ] Export grid as image
- [ ] Playlist-aware grouping
- [ ] Custom titleformat pattern dialog
- [ ] Animation effects
- [ ] Zoom slider in UI
- [ ] Configurable image sources (folder.jpg, cover.jpg, etc.)