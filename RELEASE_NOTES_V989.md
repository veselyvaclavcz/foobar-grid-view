# Album Art Grid v9.8.9 - Release Notes

## 🔧 Critical Bug Fixes Release

This release addresses critical issues reported by users in v9.8.8, particularly with playlist view functionality and search filter behavior.

## 🐛 Bug Fixes

### Playlist View Issues
- **Fixed**: Double-click in Playlist view no longer clears the entire playlist
  - Previously, double-clicking an album while viewing a playlist would erase the playlist
  - Now correctly plays the selected album without modifying the playlist

### Search Filter Issues  
- **Fixed**: Double-click on filtered items now plays the correct album
  - Previously, clicking on filtered results would play a different album due to index mismatch
  - Now properly maps display indices to actual item indices

- **Fixed**: Hover tooltips now show correct items when search filter is active
  - Tooltips were displaying information for wrong albums when filtered
  - Now correctly shows information for the hovered item

### UI Improvements
- **Added**: "Playlist is empty" message when viewing empty playlists
  - Provides clear feedback instead of showing a blank grid

## ✨ Features (from v9.8.8)

### Search & Filter
- Press `Ctrl+Shift+S` to toggle search box
- Real-time filtering as you type
- Searches in album, artist, and genre fields
- Press `Ctrl+Shift+S` again or Escape to close

### Context Menu Enhancements
- Direct Play/Add commands in context menu
- Configurable double-click behavior submenu
- Search option in right-click menu

## 📦 Installation

1. Close foobar2000 if running
2. Double-click `foo_albumart_grid_v989.fb2k-component`
3. Restart foobar2000

## 🔄 Upgrade Notes

- Fully compatible with existing configurations
- No settings will be lost during upgrade
- Component will automatically update from any previous version

## 🧪 Testing Checklist

- [x] Double-click in Library view clears playlist and plays selected album
- [x] Double-click in Playlist view plays without clearing playlist  
- [x] Search filter correctly maps clicked items to actual albums
- [x] Hover tooltips match displayed albums when filtered
- [x] Empty playlist shows appropriate message

## 📝 Technical Details

- Built with foobar2000 SDK 2025-03-07
- Compiled with Visual Studio 2022
- Target: foobar2000 v2.0+

## 🙏 Acknowledgments

Thanks to all users who reported issues and helped with testing, especially those who identified the playlist view and filtering bugs.

---
*For bug reports or feature requests, please visit the [foobar2000 forum thread](https://www.foobar2000.org/forum) or create an issue on GitHub.*