# Developer Notes - v10.0.47 (Stability, Performance, Footer Info)

This document captures the technical context behind v10.0.47 to aid future maintenance.

## Source & Artifacts
- New source: `foo_albumart_grid_v10_0_47_SMART_GRID_FLOW_HYBRID.cpp`
- Built component: `dist/foo_albumart_grid_v10_0_47_SMART_GRID_FLOW_HYBRID.fb2k-component`
- README updated to reflect v10.0.47 changes and usage.

## Status/Footer Integration
- `%albumart_grid_info%` extended to include Group and Sort,
  e.g. `Library: 2,134 albums — Group: Album — Sort: Release Date`.
- Implementation: `albumart_grid_field_provider::process_field()` now emits group/sort labels.
- Shared state updated during `refresh_items()`:
  - `g_album_count`, `g_is_library_view`, `g_last_grouping`, `g_last_sorting`.
- Removed the temporary in‑grid footer overlay; status info lives only in the foobar2000 footer.

## Config Safety
- Rewrote `grid_config` persistence to a versioned binary format (magic `AAGC`, ver `1`).
- Field‑by‑field load with bounds checks; legacy fallback for older blobs.
- Avoids UB from struct `memcpy` across versions/platforms.

## Rendering & Performance
- Added reusable back buffer (HDC/HBITMAP) and only reallocate on size changes.
- Coalesced invalidation via `WM_APP+101` to reduce redundant repaints.
- Skip expensive interpolation when drawing 1:1 thumbnails.

## Async Artwork Loading
- Restored proven detached `std::thread` workers with proper `.detach()` to avoid terminate.
- `WM_APP_THUMBNAIL_READY` result path validates instance/generation and updates the cache safely.

## Thumbnail Cache
- Converted to shared ownership (`std::shared_ptr<thumbnail_data>`) with an LRU list + map index.
- Eliminates dangling pointers on item rebuild; proper eviction and memory tracking.
- Adaptive memory sampling throttled (~1.5s) to reduce `GlobalMemoryStatusEx` frequency.

## Shutdown & Safety
- `thumbnail_data::clear()` uses component shutdown state (no GDI+ heuristics).
- Type‑safe `shutdown_protection` registry (`std::set<album_grid_instance*>`).

## Project / Build
- Project updated to compile 10.0.47 and to package the `.fb2k-component`.
- Linked against SDK prebuilt libs (`shared`, `pfc`, `foobar2000_SDK`, `foobar2000_component_client`).

## Follow‑ups / Ideas
- Optional thread pool (re‑introduce carefully once stable in the field).
- Bucketized thumbnail sizes to reduce churn on column changes.
- Optional setting: show/hide `%albumart_grid_info%` extensions for compact status bars.
