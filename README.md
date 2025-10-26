Album Art Grid (foo_albumart_grid)

Version: 10.0.46

- Smart grid flow with enlarged Now Playing (2x2 / 3x3)
- Text overlay rendered directly on artwork
- Playlist overlay for enlarged Now Playing
- Full Unicode label rendering
- Smooth scrolling and optimized layout

What’s fixed in 10.0.46
- Playlist overlay respects the selected UI font height. Measuring, drawing, and hit-testing now use the same foobar2000 UI font, eliminating overlap with the artwork text overlay.
- Transparent PNG alpha blending fixes in overlays and grid draw.
- Stability fixes around grouping change and cache invalidation.

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
  - `foo_albumart_grid_v10_0_45_SMART_GRID_FLOW_HYBRID.cpp`
- Define:
  - `_WIN32_WINNT=0x0600`, `FOOBAR2000_TARGET_VERSION=80`
- Output name:
  - `foo_albumart_grid.dll`

Install/Run
- Build Release x64.
- Copy `foo_albumart_grid.dll` to foobar2000’s `components` folder.
- Restart foobar2000, add the element via Layout Editing Mode: Library > Album Art Grid.
- Features:
  - Right-click > Enlarged Now Playing to enable 3x3 and “Show Playlist Overlay”.
  - Change foobar2000’s UI font to see the overlay resize correctly.

Notes
- Source also contains previous reference versions for history.
- If you maintain packaging as `.fb2k-component`, build and bundle the DLL and required metadata as usual.
