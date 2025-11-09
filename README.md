Album Art Grid (foo_albumart_grid)

Version: 10.0.47

- Smart grid flow with enlarged Now Playing (2x2 / 3x3)
- Text overlay rendered directly on artwork
- Playlist overlay for enlarged Now Playing
- Full Unicode label rendering
- Smooth scrolling and optimized layout

What's new/fixed in 10.0.47
- Stability: safer, versioned config storage (no raw memcpy) with backward-compatible loading.
- Rendering: persistent back buffer to reduce GDI churn and smooth scrolling.
- Async loads: stabilized thumbnail loading and fixed thread lifetime (no stray workers).
- Cache: adaptive, LRU-based thumbnail cache with shared ownership to prevent stale pointers.
- Status footer: %albumart_grid_info% now includes Group and Sort in addition to album count (works in foobar2000 status bar/footer).
- UI: removed the in-grid footer overlay; footer info now belongs only in the status bar.
- Misc: coalesced invalidations to reduce redundant repaints.

Build Requirements
- Windows, Visual Studio 2019/2022 (x64 toolchain)
- foobar2000 SDK (included under `SDK-2025-03-07/`)

Build Steps (recommended)
- Create a new Win32/x64 DLL project in Visual Studio (Empty Project).
- Add include directories:
  - `SDK-2025-03-07/foobar2000/SDK`
  - `SDK-2025-03-07/foobar2000/helpers`
  - `SDK-2025-03-07/pfc`
  - `SDK-2025-03-07/libPPUI`
- Link libraries (Release x64):
  - `pfc`, `libPPUI`, `foobar2000_SDK`, `foobar2000_component_client`, `shared`
  - System: `comctl32`, `gdi32`, `gdiplus`, `user32`, `shlwapi`, `uxtheme`, `Msimg32`
- Add source file:
  - `foo_albumart_grid_v10_0_47_SMART_GRID_FLOW_HYBRID.cpp`
- Define:
  - `_WIN32_WINNT=0x0600`, `FOOBAR2000_TARGET_VERSION=80`
- Output name:
  - `foo_albumart_grid.dll`

Install/Run
- Build Release x64.
- Copy `foo_albumart_grid.dll` to foobar2000’s `components` folder.
- Restart foobar2000, add the element via Layout Editing Mode: Library > Album Art Grid.
- Status bar footer
  - Ensure your status bar string contains `%albumart_grid_info%`.
  - Output example: `Library: 2,134 albums — Group: Album — Sort: Release Date`.
  - Toggle Library/Playlist view with `P` while the grid has focus.

Notes
- Source also contains previous reference versions for history.
- If you maintain packaging as `.fb2k-component`, build and bundle the DLL and required metadata as usual.
