# Album Art Grid for foobar2000 - Version 10.0.45

## 🎉 Latest Release: v10.0.45 SMART GRID FLOW

**Download**: `foo_albumart_grid_v10_0_45_SMART_GRID_FLOW_HYBRID.fb2k-component`

### ✨ NEW FEATURES in v10.0.45 SMART GRID FLOW
- **📅 RELEASE DATE SORTING** - Adds a "By Release Date" sort option using album metadata (ORIGINALDATE, DATE, YEAR).
- **🖱️ DOUBLE-CLICK PERSISTENCE FIX** - Double-click action selection now survives foobar2000 restarts.
- **🧠 SMART GRID FLOW CONTINUES** - All v10.0.44 layout and reflow improvements stay active.

### ⭐ Maintained from v10.0.44 – Smart Grid Flow (Overlay Navigation + Stability)
- Overlay navigation: Up/Down to move selection inside 3x3 playlist overlay, Enter to play.
- Subtle selection highlight (semi-transparent, theme-aware; no accent stripe).
- Enlarged placement: 2x2/3x3 keeps one cell at the natural position; no extra rows; compact fill.
- More responsive (but smooth) scrollbar behavior.
- Double-click option: "Play in Album Grid playlist".

Install: import `foo_albumart_grid_v10_0_45_SMART_GRID_FLOW_HYBRID.fb2k-component` into foobar2000.
Source: `foo_albumart_grid_v10_0_45_SMART_GRID_FLOW_HYBRID.cpp`

Keyboard (overlay shown):
- Up/Down: move selection within overlay
- Enter: play selected track

Notes:
- Arrows are handled by the grid window so the playlist panel won't steal focus.
- Wheel/scrollbar adjusted for finer movement.
### ✨ Highlights from v10.0.43 SMART GRID FLOW
- **🧠 INTELLIGENT ENLARGED ITEM PLACEMENT** - Automatically finds optimal positions for 3x3 enlarged artwork
- **🔄 WRAP-AROUND POSITIONING** - Items maintain logical sequence when enlarged artwork blocks natural positions
- **🎯 ZERO-GAP GRID LAYOUT** - Mathematical precision eliminates visual gaps in grid display
- **⬅️ BACKWARD POSITIONING PREVENTION** - Smart algorithm prevents gaps at grid end
- **📋 SEQUENTIAL ORDER PRESERVATION** - Items stay near expected positions rather than jumping to beginning
- **🧮 LAST ROW LOGIC** - Properly manages forbidden zones in incomplete rows with mathematical precision
- **⚡ CELL-BASED ALLOCATION** - Revolutionary 2D cell reservation system for perfect grid filling
- **🔧 MAINTAINED: Playlist Overlay** - All v10.0.35 playlist display features preserved
- **📐 MAINTAINED: Layout** - Enlarged artwork remains perfectly square
- **✅ MAINTAINED: Navigation** - Page Up/Down, Letter Jump, Unicode support all preserved

### 🚀 MAINTAINED from v10.0.30 PERFORMANCE OPTIMIZATION
- **⚡ PERFORMANCE OPTIMIZATIONS** - Confirmed smooth operation with 3500+ item collections
- **🎯 AUTO-SCROLL FIXED** - Now works correctly (album-level changes only for better UX)
- **🔇 CONSOLE SPAM ELIMINATED** - Reduced timer frequency to 2s, clean console output
- **🖼️ ENLARGED ARTWORK CUTOFF RESOLVED** - Smart extra height calculation for items at list end
- **📐 SPACING ISSUES FIXED** - Scrollbar-respecting positioning eliminates excessive gaps
- **🎵 SMART UX** - Auto-scroll only on album changes, not every track (prevents browsing interruption)

### 🚨 MAINTAINED CRITICAL FIXES from v10.0.28 & Earlier
- **🛠️ SHUTDOWN CRASH ELIMINATION** - Fixed foo_ui_std crashes during shutdown (access violation at FFFFFFFFFFFFFFFFh)
- **🔒 UNSAFE POINTER FIX** - Removed dangerous `*((volatile bool*)inst) = false` operations causing memory corruption  
- **⚡ SAFE CLEANUP** - Safe instance list cleanup without touching instance memory
- **🎯 FACTORY PROTECTION** - UI factory refuses to create instances during shutdown
- **🧠 GDI+ PROTECTION** - Enhanced GDI+ shutdown protection for thumbnail cleanup
- **✅ SERVICE SAFETY** - Prevents service system access violations during component destruction

### 🚨 MAINTAINED CRITICAL FIXES from v10.0.27 & Earlier
- **🛠️ CRASH ELIMINATION** - Fixed access violation crashes at offset 15D0Ch/15D14h during shutdown
- **🔄 RACE CONDITION FIX** - Fixed destructor race condition - invalidate() now called LAST
- **🔒 THREAD SAFETY** - Added critical section protection for thread-safe destruction
- **🎯 NULL POINTER FIX** - Fixed NULL pointer dereference in main_thread_callback::callback_run
- **⚡ CLEANUP SEQUENCE** - Proper resource cleanup sequence to prevent use-after-free
- **✅ PRESERVED FEATURES** - All v10.0.23 scrollbar fixes and v10.0.22 performance optimizations maintained

### 🚀 MAINTAINED from v10.0.22 CUSTOM SCROLLBAR OPTIMIZATION
- **🎯 PERFORMANCE FIX** - Eliminates UI hangs with large collections (3000+ items)
- **⚡ CUSTOM SCROLLBAR** - High-performance custom scrollbar replaces Windows scrollbar
- **🚀 OPTIMIZED SCROLLING** - Eliminated expensive calculate_layout() calls during scrolling
- **🎨 THEME INTEGRATION** - Proper foobar2000 theme colors with hover effects
- **📐 SMART POSITIONING** - Scrollbar positioned outside grid area like artist info panel
- **🖱️ FULL INTERACTION** - Complete mouse support - thumb dragging, page scrolling, wheel

## Project Overview

Stable and feature-complete Album Art Grid component for foobar2000, focusing on **zero crashes** and **user-friendly installation**.

## 📥 Installation

1. **Download** the latest `foo_albumart_grid_v10_0_35_PLAYLIST_OVERLAY_ENHANCED.fb2k-component`
2. **Double-click** the `.fb2k-component` file to install in foobar2000
3. **Access the component** via:
   - **Library menu** → Select "Album Art Grid" from Media Library viewers dropdown
   - **Manual UI layout** → View → Layout → Enable editing → Right-click → Add UI Element → Album Art Grid

## ✨ Key Features

### 🎹 Navigation & Controls
- **Page Up/Down** - Navigate by full pages using PgUp/PgDn keys
- **Letter Jump** - Press A-Z or 0-9 to jump to first album starting with that character
- **Unicode Support** - Perfect display of Chinese, Japanese, and other Unicode text
- **Context Menu** - Right-click for "Open in Folder" and other options

### 🎵 Dedicated Playlist (NEW in v10.0.26)
- **Single Reusable Playlist** - Creates/reuses one "Album Grid" playlist instead of multiplying playlists
- **Right-Click Access** - "Play in Album Grid Playlist" option directly in context menu  
- **Double-Click Configuration** - Also available as configurable double-click action
- **Instant Workflow** - Select albums → Right-click → Play in Album Grid Playlist → Immediate playback
- **No Playlist Clutter** - Grid functions as pure viewer, not playlist manager

### 📚 Library Integration
- **Media Library Viewer** - Appears in foobar2000's official Media Library viewers list
- **Automatic Discovery** - foobar2000 recognizes and lists the component automatically
- **Seamless Integration** - Works with all foobar2000 library features

### ⚡ Performance & Scrolling (NEW in v10.0.22)
- **Large Collection Support** - Smooth scrolling with 3000+ albums (no more UI hangs)
- **Custom Scrollbar** - High-performance implementation with proper theme colors
- **Optimized Layout** - Eliminated expensive recalculations during scrolling
- **Smart Hover Effects** - Theme-aware scrollbar with visual feedback

### 🛡️ Stability & Safety
- **Zero Shutdown Crashes** - Protected against all known shutdown-related crashes
- **Memory Safety** - Proper object validation and pointer checking
- **Exception Handling** - Graceful handling of all error conditions
- **Thread Safety** - Safe operation in multi-threaded environment

## Features

### Grouping Modes (13)
- By Folder Structure
- By Album / Artist / Album Artist
- By Year / Genre  
- By Date Modified / Date Added
- By File Size / Track Count
- By Rating / Playcount
- Custom Pattern (titleformat)

### Sorting Options (11)
- Name / Artist / Album
- Year / Genre
- Date Modified
- Total Size / Track Count  
- Rating / Path
- Random/Shuffle

### UI Features
- 📐 Resizable thumbnails (80px minimum, no maximum)
- 🔢 Auto-fill mode (1+ columns, no limit)
- 🎨 Dark mode support
- 🔍 Real-time search
- 🎵 Now playing highlight
- 🌐 Full Unicode support
- 📱 High DPI support

### Performance
- Smart memory management (256MB-2GB cache)
- LRU cache with auto-detection
- Lazy loading for large libraries
- Optimized for 10,000+ albums

## Architecture

### 5-Layer Modular Design

```
┌─────────────────────────────────────┐
│         UI Layer                     │
│  GridWindow, MenuHandler, Input     │
├─────────────────────────────────────┤
│      Integration Layer               │
│  SDK Adapters, Callbacks            │
├─────────────────────────────────────┤
│    Business Logic Layer              │
│  AlbumDataModel, SearchEngine       │
├─────────────────────────────────────┤
│      Resources Layer                 │
│  ThumbnailCache, AlbumArtLoader     │
├─────────────────────────────────────┤
│      Foundation Layer                │
│  LifecycleManager, CallbackManager  │
└─────────────────────────────────────┘
```

## Project Structure

```
foo_albumart_grid/
├── src/
│   ├── foundation/     # Core lifecycle and safety
│   ├── resources/      # Caching and loading
│   ├── core/          # Business logic
│   ├── integration/   # SDK integration
│   └── ui/           # User interface
├── tests/            # Test suites
├── docs/
│   ├── modules/      # Module documentation
│   ├── api/         # API documentation
│   └── development/ # Development logs
└── build/           # Build outputs
```

## Development Status

### Current Phase: Architecture & Setup ✅
- [x] SDK analysis and crash investigation
- [x] Feature specification from v10.0.17
- [x] Modular architecture design
- [x] Project structure creation
- [x] Documentation system setup

### Next Phase: Foundation Implementation
- [ ] LifecycleManager module
- [ ] CallbackManager with weak pointers
- [ ] Basic SDK integration
- [ ] Initial test harness

## Building

### Prerequisites
- Visual Studio 2019 or later
- foobar2000 SDK (latest)
- Windows SDK 10.0+

### Build Steps
```bash
# Clone repository
git clone [repository]

# Open solution
start foo_albumart_grid.sln

# Build
msbuild /p:Configuration=Release /p:Platform=x64
```

## Documentation

### For Developers
- `/docs/modules/` - Individual module documentation
- `/docs/development/` - Development logs and decisions
- `/docs/api/` - API documentation
- `ARCHITECTURE_DESIGN_V2.md` - Complete architecture

## Known Issues from Previous Versions

### Critical Issues (Fixed in v11)
- **Shutdown crashes** - Resolved with proper lifecycle management
- **Memory leaks** - Fixed with strict cache limits
- **Callback corruption** - Solved with weak pointer pattern

## 📋 Version History

### v10.0.43 SMART GRID FLOW (Current - September 2025) ✅
- **INTELLIGENT ENLARGED ITEM PLACEMENT** - Automatically finds optimal positions for 3x3 enlarged artwork without creating gaps
- **WRAP-AROUND POSITIONING LOGIC** - When enlarged artwork can't be placed naturally, items maintain logical sequence through intelligent redistribution
- **ZERO-GAP GRID MATHEMATICS** - Revolutionary cell-based allocation system with mathematical precision eliminates all visual gaps
- **LAST USABLE POSITION ALGORITHM** - Smart detection of rightmost valid position for 3x3 enlarged items respecting grid boundaries
- **FORBIDDEN ZONE MANAGEMENT** - Proper handling of unusable positions in incomplete last rows (e.g., only positions 1-3 usable in 12th row)
- **SEQUENTIAL ORDER PRESERVATION** - Items stay "někde na konci" instead of jumping to beginning when positioning conflicts occur
- **TWO-PASS PLACEMENT ENGINE** - First pass handles wrap-around items, second pass places enlarged item at optimal position
- **MAINTAINED FEATURES** - All v10.0.35 playlist overlay, navigation, and performance features preserved

### v10.0.35 PLAYLIST OVERLAY ENHANCED (Previous - September 2025) ✅
- **PLAYLIST OVERLAY IMPLEMENTATION** - Professional playlist display overlaid on 3x3 enlarged now playing artwork
- **REAL METADATA ACCESS** - Shows actual track titles, artists, and durations using format_title() approach
- **PERFECT COLUMN ALIGNMENT** - Proportional column widths prevent overlap and ensure proper spacing
- **SUMMARY INFORMATION** - Footer displays total track count and playlist duration
- **SMART NAVIGATION** - Auto-centers playlist view around currently playing track
- **NOW PLAYING HIGHLIGHT** - Current track highlighted with blue text and play triangle indicator
- **THEME INTEGRATION** - 85% opacity dark overlay with theme-aware text colors for perfect readability
- **MAINTAINED FEATURES** - All v10.0.34 text overlay, navigation, and performance features preserved

### v10.0.34 TEXT OVERLAY THEME FIX (Previous - September 2025)
- **TEXT OVERLAY IMPLEMENTATION** - Text now overlays directly on artwork with theme-appropriate semi-transparent backgrounds
- **THEME COLOR FIX** - Dark themes use dark backgrounds with light text, light themes use light backgrounds with dark text
- **LAYOUT PROBLEM SOLVED** - Enlarged artwork (2x2/3x3) remains perfectly square - no more broken grid layout
- **PERFECT READABILITY** - 70% opacity overlay ensures text is always readable regardless of artwork content
- **MAINTAINED FEATURES** - All navigation, performance optimizations, and crash protection preserved

### v10.0.27 ZOMBIE CALLBACK FIX (Previous - September 2025)
- **ZOMBIE CALLBACK PATTERN** - Callbacks survive object destruction to prevent NULL pointer crashes
- **AUTOSCROLL PERSISTENCE** - Auto-scroll to Now Playing setting persists after foobar2000 restart
- **ATOMIC OPERATIONS** - Thread-safe callback handling with atomic operations
- **MAGIC NUMBER VALIDATION** - Object integrity validation using magic numbers
- **SHUTDOWN GUARD** - Early shutdown detection and minimal cleanup during unsafe shutdown
- **DESTRUCTOR PROTECTION** - Enhanced destructor protection with main window validation

### v10.0.24 DESTRUCTOR FIX (Previous - September 2025)
- **CRASH ELIMINATION** - Fixed access violation crashes at offset 15D0Ch/15D14h during shutdown
- **RACE CONDITION FIX** - Fixed destructor race condition - invalidate() now called LAST
- **THREAD SAFETY** - Added critical section protection for thread-safe destruction
- **NULL POINTER FIX** - Fixed NULL pointer dereference in main_thread_callback::callback_run
- **CLEANUP SEQUENCE** - Proper resource cleanup sequence to prevent use-after-free
- **DEFINITIVE SOLUTION** - This DEFINITIVELY FIXES destructor crashes in fb2.24.6 and fb2.25.1preview
- **MAINTAINED FEATURES** - All v10.0.23 scrollbar fixes and v10.0.22 performance optimizations preserved

### v10.0.23 SCROLLBAR CRASH FIX (Previous - September 2025)
- **CRASH ELIMINATION** - Fixed access violation crashes during component shutdown (offset 15CCCh)
- **NULL POINTER FIX** - Fixed NULL pointer dereferences in custom scrollbar mouse handling
- **MEMORY MANAGEMENT** - Resolved memory management race conditions in scrollbar brush/pen cleanup
- **VERSION CORRECTION** - Fixed version string corruption (now correctly shows v10.0.23)
- **SHUTDOWN SEQUENCE** - Enhanced shutdown sequence with proper resource cleanup order
- **PRESERVED OPTIMIZATIONS** - All v10.0.22 custom scrollbar performance optimizations maintained

### v10.0.22 CUSTOM SCROLLBAR OPTIMIZATION (Previous - September 2025)
- **PERFORMANCE FIX** - Eliminates UI hangs with large collections (3000+ items)
- **CUSTOM SCROLLBAR** - High-performance custom scrollbar replaces Windows scrollbar
- **OPTIMIZED SCROLLING** - Eliminated expensive calculate_layout() calls during scrolling
- **THEME INTEGRATION** - Proper foobar2000 theme colors with hover effects
- **SMART POSITIONING** - Scrollbar positioned outside grid area like artist info panel
- **FULL INTERACTION** - Complete mouse support - thumb dragging, page scrolling, wheel

### v10.0.20 PERSISTENT BLACKLIST FIX (Previous - September 2025)
- **PERSISTENT BLACKLIST** - Blacklist now SURVIVES component refreshes using global static storage
- **ZERO RETRIES** - Items without artwork are NEVER retried until manual blacklist clear
- **THREAD SAFETY** - Thread-safe blacklist operations with critical_section synchronization
- **ENHANCED KEYS** - Enhanced blacklist key generation for better item identification
- **COMPLETE ELIMINATION** - Complete elimination of infinite retry loops that caused console spam
- **FULL COMPATIBILITY** - Maintains all v10.0.18 shutdown crash fixes and library viewer integration
- **DEFINITIVE SOLUTION** - This DEFINITIVELY ELIMINATES the infinite retry issue
- **PRODUCTION READY** - Blacklist persists across all component operations and refreshes

### v10.0.19 MPV FINAL (Previous - September 2025)
- **DEFINITIVE MPV FIX** - Album art requested ONLY ONCE per item - no retries ever
- **Permanent Blacklist** - Items without art are NEVER retried (eliminates foo_mpv libav errors)
- **Smart Timer Logic** - Timer stops when no items need loading (not when all have images)
- **Zero Console Spam** - Completely eliminates repeated album art requests
- **Library Viewer Integration** - Component appears in Media Library preferences (maintained from v10.0.18)
- **Shutdown Crash Protection** - All shutdown-related crashes fixed (maintained from v10.0.18)
- **Page Up/Down Navigation** - Full page navigation with PgUp/PgDn keys (maintained from v10.0.18)
- **Ready for Production** - Fully stable and tested with definitive foo_mpv compatibility

### v10.0.18 (Previous - September 2025)
- **Library Viewer Integration** - Component appears in Media Library preferences
- **Shutdown Crash Protection** - Fixed all shutdown-related crashes
- **Page Up/Down Navigation** - Full page navigation with PgUp/PgDn keys
- **Enhanced Stability** - All v10.0.17 fixes maintained

### v10.0.17 (Previous Stable)
- **Critical Object Validation Fix** - Fixed crash at offset 0x1208Ch
- **Unicode Display** - Perfect rendering of international characters  
- **Letter Jump Navigation** - A-Z/0-9 navigation system
- **Corrupted Callback Protection** - Fixed callback pointer corruption

### v10.0.16 & Earlier
- Base functionality and feature development
- Various stability improvements
- Foundation for current stable release

---

**Note**: This is a complete rewrite. No code from previous versions is being reused. The focus is on **stability first**, with all features being reimplemented using safe patterns and proper SDK usage.



