# Release flow (local)

Goal: always start from the latest built package in `dist/`, locate the matching source, bump to a new version, and ensure the new `.fb2k-component` ends up in `dist/` for local verification.

## 1) Find the latest version in `dist/`

Packages are named like:

`dist/foo_albumart_grid_v10_0_50_SMART_GRID_FLOW_HYBRID.fb2k-component`

PowerShell helper (picks the highest `v10_0_XX`):

```powershell
$latest = Get-ChildItem -File dist -Filter "foo_albumart_grid_v10_0_*_SMART_GRID_FLOW_HYBRID.fb2k-component" |
  ForEach-Object {
    if ($_.Name -match 'v10_0_(\d+)_') {
      [pscustomobject]@{ File = $_; Patch = [int]$Matches[1] }
    }
  } |
  Sort-Object Patch -Descending |
  Select-Object -First 1

$latest.File.FullName
```

## 2) Locate the matching source (and build script)

Expected files (same patch number as the package):

- `foo_albumart_grid_v10_0_XX_SMART_GRID_FLOW_HYBRID.cpp`
- `build_v10_0_XX_SMART_GRID_FLOW_HYBRID.ps1` (optional but recommended)

If the source isn’t in the current checkout but exists in git history, restore it:

```powershell
git log --all --oneline -- foo_albumart_grid_v10_0_XX_SMART_GRID_FLOW_HYBRID.cpp
git checkout <commit> -- foo_albumart_grid_v10_0_XX_SMART_GRID_FLOW_HYBRID.cpp
```

## 3) Create the new version from the latest source

1. Copy the latest source to a new file:
   - `foo_albumart_grid_v10_0_XX_...cpp` → `foo_albumart_grid_v10_0_YY_...cpp`
2. Bump the component version in the new file:
   - `DECLARE_COMPONENT_VERSION(..., "10.0.YY", ...)`
   - Update the header comment string(s) mentioning `10.0.XX`
3. Apply your code changes on top of the new file.

## 4) Update build/packaging to point to the new source

Update `component/foo_albumart_grid.vcxproj`:

- `<ClCompile Include="..\foo_albumart_grid_v10_0_YY_SMART_GRID_FLOW_HYBRID.cpp" />`
- Post-build packaging name should write:
  - `dist/foo_albumart_grid_v10_0_YY_SMART_GRID_FLOW_HYBRID.fb2k-component`

Note: PowerShell `Compress-Archive` supports only `.zip`, so package as `.zip` then rename to `.fb2k-component` (already handled in the current post-build command).

## 5) Create/update the build script for the new version

Copy the last one and bump the expected output path:

- `build_v10_0_XX_SMART_GRID_FLOW_HYBRID.ps1` → `build_v10_0_YY_SMART_GRID_FLOW_HYBRID.ps1`
- Ensure it checks for:
  - `dist/foo_albumart_grid_v10_0_YY_SMART_GRID_FLOW_HYBRID.fb2k-component`

SDK prereqs: the build script should build these before the component:

- `SDK-2025-03-07/pfc/pfc.vcxproj`
- `SDK-2025-03-07/foobar2000/shared/shared.vcxproj`
- `SDK-2025-03-07/foobar2000/SDK/foobar2000_SDK.vcxproj`
- `SDK-2025-03-07/foobar2000/foobar2000_component_client/foobar2000_component_client.vcxproj`

## 6) Build and verify output in `dist/`

Run:

```powershell
.\build_v10_0_YY_SMART_GRID_FLOW_HYBRID.ps1
```

Verify:

- `dist/foo_albumart_grid_v10_0_YY_SMART_GRID_FLOW_HYBRID.fb2k-component` exists
- (Optional) the build script prints SHA256 for quick sanity checks

