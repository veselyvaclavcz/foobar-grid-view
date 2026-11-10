# Album Art Grid for foobar2000

[![Build component](https://github.com/veselyvaclavcz/foobar-grid-view/actions/workflows/build-component.yml/badge.svg)](https://github.com/veselyvaclavcz/foobar-grid-view/actions/workflows/build-component.yml)

**Album Art Grid** (`foo_albumart_grid`) is a native C++ UI component for the
[foobar2000](https://www.foobar2000.org/) music player. It turns the music
library or the active playlist into a fast, scrollable grid of album covers,
so a large collection can be browsed visually instead of as a text list.

Current release: **10.0.51** (Windows, 64-bit foobar2000 2.x).

## Features

- **Album grid for library or playlist**: switch between the whole media
  library and the current playlist (`P` while the grid has focus).
- **Grouping and sorting**: group by album, artist, album artist, genre, year,
  folder and more; sort by name, release date, track count or randomly.
- **Smart grid flow**: the Now Playing album can be enlarged to 2x2 or 3x3
  tiles, and the remaining items flow around it without gaps.
- **Playlist overlay**: the enlarged Now Playing tile can show its track list
  directly on top of the artwork.
- **Text overlay and labels**: one to three label lines rendered on the
  artwork, with full Unicode support and theme-aware colours.
- **Artwork scaling modes**: Fit (keeps aspect ratio), Crop to Fill or Stretch.
- **Multi-disc albums**: `CD1`/`CD2` folders are shown as one album with a
  `2CD`/`3CD` badge; tracks are queued in disc then track order.
- **Playback actions**: play, add to the current or a new playlist, open the
  album folder, jump to Now Playing (`Ctrl+Q`), search (`Ctrl+Shift+S`).
- **Status bar integration**: the `%albumart_grid_info%` field shows the album
  count and the active grouping and sorting, for example
  `Library: 2,134 albums — Group: Album — Sort: Release Date`.
- **Performance**: asynchronous thumbnail loading on a small thread pool, an
  adaptive LRU thumbnail cache and a persistent back buffer for smooth
  scrolling in large libraries.

## Installation

1. Download [`dist/foo_albumart_grid_v10_0_51_SMART_GRID_FLOW_HYBRID.fb2k-component`](dist/foo_albumart_grid_v10_0_51_SMART_GRID_FLOW_HYBRID.fb2k-component)
   (or the `foo_albumart_grid-fb2k-component` artifact of the latest successful
   [CI run](https://github.com/veselyvaclavcz/foobar-grid-view/actions/workflows/build-component.yml)).
2. In foobar2000 open *File > Preferences > Components*, click *Install...*,
   select the downloaded file and restart foobar2000.
3. Enter *View > Layout > Enable layout editing mode*, then add the
   **Album Art Grid** element to your layout.
4. Optional: add `%albumart_grid_info%` to the status bar format to see the
   album count and the active grouping and sorting.

Right-click the grid to change grouping, sorting, labels, columns, artwork
scaling, the enlarged Now Playing size and the double-click action.

## Building from source

Requirements:

- Windows with Visual Studio 2022 or the VS 2022 Build Tools (MSVC v143 x64
  toolset, Windows 10/11 SDK).
- No other dependencies: the foobar2000 SDK is included in
  [`SDK-2025-03-07/`](SDK-2025-03-07/).

From a *Developer Command Prompt for VS 2022* (or any shell where `msbuild` is
on the `PATH`) in the repository root:

```bat
:: 1. Build the foobar2000 SDK libraries
::    (pfc, foobar2000_SDK, shared, foobar2000_component_client)
msbuild SDK-2025-03-07\BUILD_SDK_LIBS.sln -p:Configuration=Release -p:Platform=x64 -m

:: 2. Build the component
msbuild component\foo_albumart_grid.vcxproj -p:Configuration=Release -p:Platform=x64 -m
```

Output:

| File | Description |
| --- | --- |
| `build\x64\Release\foo_albumart_grid.dll` | The component DLL (version 10.0.51) |
| `build\foo_albumart_grid_v10_0_51.fb2k-component` | Installable package (zip containing the DLL) |

The same two steps run in [GitHub Actions](.github/workflows/build-component.yml)
on every push to `main` and every pull request; the workflow also checks the
DLL version and uploads the `.fb2k-component` as a build artifact.

### Project layout

| Path | Contents |
| --- | --- |
| [`foo_albumart_grid_v10_0_51_SMART_GRID_FLOW_HYBRID.cpp`](foo_albumart_grid_v10_0_51_SMART_GRID_FLOW_HYBRID.cpp) | Source of the released component (v10.0.51) |
| [`component/foo_albumart_grid.vcxproj`](component/foo_albumart_grid.vcxproj) | Visual Studio project; the version is defined once as `ComponentVersion` |
| [`resources/`](resources/) | Windows resources (version info) |
| [`dist/`](dist/) | Prebuilt release package |
| [`SDK-2025-03-07/`](SDK-2025-03-07/) | foobar2000 SDK (third party, see [THIRD_PARTY.md](THIRD_PARTY.md)) |
| [`src/`](src/), [`CMakeLists.txt`](CMakeLists.txt), [`docs/modules/`](docs/modules/) | v11 modular rewrite, in development |
| [`docs/`](docs/) | Development notes |

## v11 (in development, not released)

[`CMakeLists.txt`](CMakeLists.txt) and the [`src/`](src/) tree contain a
modular rewrite of the component (foundation, integration, core, resources and
UI layers, documented in [`docs/modules/`](docs/modules/)). It is **work in
progress and not released**; the shipped component is v10.0.51, built from the
Visual Studio project above.

## What's new

### 10.0.51
- Multi-disc folders: `CD1/CD2` layouts now show the album root (labels, art
  and open-folder are no longer biased to CD2).

### 10.0.50
- Multi-disc: the grid shows one album, with a `2CD`/`3CD` badge when discs
  are detected.
- Playlist: disc-aware track ordering (disc then track, or disc then title).

### 10.0.48
- Artwork scaling: new Fit/Crop/Stretch modes (default Fit preserves aspect
  ratio).
- Rendering: restored per-item interpolation state to avoid unintended
  pixelation.
- Performance: async thumbnail loads use a small thread pool instead of
  per-item threads.
- UI: cached placeholder font and label text conversion to reduce per-frame
  overhead.
- Status: footer information is provided via `%albumart_grid_info%` (no
  in-grid footer).

### 10.0.47
- Stability: safer, versioned config storage (no raw `memcpy`) with
  backward-compatible loading.
- Rendering: persistent back buffer to reduce GDI churn and smooth scrolling.
- Async loads: stabilized thumbnail loading and fixed thread lifetime (no
  stray workers).
- Cache: adaptive, LRU-based thumbnail cache with shared ownership to prevent
  stale pointers.
- Status: `%albumart_grid_info%` now includes Group and Sort in addition to
  the album count.
- UI: removed the in-grid footer overlay; footer information now belongs in
  the status bar.
- Misc: coalesced invalidations to reduce redundant repaints.

## License

The source code of Album Art Grid is released under the [MIT License](LICENSE).

The bundled foobar2000 SDK in `SDK-2025-03-07/` is third-party code under its
own license; see [THIRD_PARTY.md](THIRD_PARTY.md).
