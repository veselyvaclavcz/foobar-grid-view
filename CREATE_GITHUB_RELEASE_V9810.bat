@echo off
echo Creating GitHub Release for v9.8.10...

REM Check if GitHub CLI is installed
gh --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: GitHub CLI is not installed!
    echo Please install it from: https://cli.github.com/
    pause
    exit /b 1
)

REM Create the release with the component file
gh release create v9.8.10 ^
    --title "Album Art Grid v9.8.10 - Critical Crash Fixes" ^
    --notes "## 🛠️ Critical Bug Fixes Release

This release fixes critical crashes that occurred when using playlist functionality multiple times.

## 🚨 CRITICAL FIXES

### Crash Fixes
- **Fixed**: Crash when using \"Add to Current Playlist\" multiple times
  - Race condition during playlist refresh operations has been resolved
  - Component now handles playlist modifications safely

- **Fixed**: Menu Play action now preserves playlist in playlist view mode
  - Previously would clear playlist when using \"Play\" from context menu
  - Now matches double-click behavior for consistency

### Stability Improvements
- **Enhanced**: Batch track operations to prevent multiple refresh triggers
- **Enhanced**: Null checking and bounds validation throughout
- **Fixed**: Access violations caused by refresh_items() clearing m_items during operations

## ✨ IMPROVEMENTS

### Playlist Functionality
- Menu Play action now searches for corresponding tracks in playlist
- Consistent behavior between double-click and menu actions
- Single batch playlist_add_items() calls instead of multiple operations

### Technical Details
- Copy track data before playlist operations to avoid invalidation
- Prevents race conditions during automatic playlist refresh
- Safer memory management for grid items

## 📦 Installation

1. Download `foo_albumart_grid_v98.fb2k-component` below
2. Double-click the file to install (or drag into foobar2000)
3. Restart foobar2000 if prompted

## 🔄 Upgrade Notes

- **IMPORTANT**: This fixes critical crashes - upgrade strongly recommended
- Fully compatible with existing configurations
- No settings will be lost during upgrade

## 🎯 What's Fixed

- No more crashes when adding albums to playlists repeatedly
- Menu \"Play\" works same as double-click in playlist mode
- Stable operation when switching between library and playlist views
- Reliable playlist modifications without memory issues

---

**Tested with foobar2000 v2.1.6**

*For bug reports or feature requests, please visit the [foobar2000 forum thread](https://www.foobar2000.org/forum) or create an issue on GitHub.*" ^
    foo_albumart_grid_v98.fb2k-component

if errorlevel 0 (
    echo.
    echo ========================================
    echo RELEASE CREATED SUCCESSFULLY!
    echo ========================================
    echo.
    echo View it at: https://github.com/veselyvaclavcz/foobar-grid-view/releases/tag/v9.8.10
    echo.
) else (
    echo.
    echo RELEASE CREATION FAILED!
    echo Make sure you're authenticated with GitHub CLI.
    echo Run: gh auth login
)

pause