# Developer Notes – v10.0.44 (Smart Grid Flow, Overlay Navigation)

This document summarizes the main technical changes introduced in v10.0.44 to help future maintenance.

## Source & Artifacts
- New source: `foo_albumart_grid_v10_0_44_SMART_GRID_FLOW_HYBRID.cpp`
- Published component: `foo_albumart_grid_v10_0_44_SMART_GRID_FLOW_HYBRID.fb2k-component`
- README updated: v10.0.44 section promoted to top (“Latest Release”).
- .gitignore updated to allow tracking the 10.0.44 component file.

## Overlay Navigation (3×3 Enlarged)
- Selection state: `int m_overlay_selected_index`.
- Key handling:
  - Grid window claims arrows via `WM_GETDLGCODE` (returns `DLGC_WANTARROWS | DLGC_WANTALLKEYS | DLGC_WANTCHARS`).
  - `on_keydown`: when overlay active, Up/Down move `m_overlay_selected_index`, Enter calls `handle_playlist_overlay_click()`.
  - Focus is forced to the grid window at keydown to prevent playlist view stealing navigation.
- Visuals:
  - Subtle selection highlight using alpha-blended fill (theme-aware). Blue accent stripe removed per UX feedback.
  - Overlay background height now computed from header + visible rows + footer (no clipping).

## Enlarged Placement Engine (2×2 / 3×3)
- `rebuild_placement_map(int cols)` rewritten to:
  - Compute exact `total_rows = ceil((N − 1 + s*s) / cols)` (no row growth allowed).
  - For enlarged index: place an s×s block such that one of its cells covers the natural position (`index/cols, index%cols`).
  - Enforce forbidden last-row cells (`cells_in_last_row`) so the block never spills past usable width.
  - Fill remaining 1×1 items sequentially into free non-forbidden cells. Cache updated with `m_placement_cache_dirty` semantics.
- Cache invalidation on: item refresh, filter change, layout change, now-playing index change.

## Scrolling & Performance
- Scrollbar reactivity:
  - Throttle reduced to ~12 ms; thumb drag unthrottled.
  - Line scroll step is a small fraction of row height (`row_h / 6`) for gentle per-line movement.
  - Mouse wheel uses system lines (`SPI_GETWHEELSCROLLLINES`), translates to pixel steps based on row height; accumulates partial deltas for smoothness.
- Painting still double-buffered; future improvement: `ScrollWindowEx` for incremental scroll blits.

## Double-Click Action
- Added `DOUBLECLICK_PLAY_IN_GRID` to `grid_config::doubleclick_action`.
- Context menu: submenu item id `113` (“Play in Album Grid playlist”).
- `on_lbuttondblclk` (and keyboard Enter via helper) collects selected albums’ tracks, populates/activates the "Album Grid" playlist, and starts playback.

## Async Artwork (prior 10.0.44 base)
- Off-UI-thread artwork creation with `WM_APP_THUMBNAIL_READY` to marshal results to UI.
- Concurrency cap `s_inflight_loaders` and generation guard `m_items_generation` avoid stale updates.

## README Changes (Docs)
- Promoted v10.0.44 section to the top as the current release.
- Added overlay navigation usage and install steps.

## Follow‑ups / Ideas
- Use `ScrollWindowEx` to avoid full invalidates during small scrolls.
- Bucketized thumbnail sizes to reduce churn on column changes.
- Consider WIC-based decode path for large downscales.
- Optional PageUp/PageDown within overlay to page rows.
- Tag GitHub release `v10.0.44` and attach the component.
