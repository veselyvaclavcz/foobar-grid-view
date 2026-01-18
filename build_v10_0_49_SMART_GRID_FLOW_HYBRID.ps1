# Build script for Album Art Grid v10.0.49 (SMART GRID FLOW HYBRID)
# Produces:
# - dist\foo_albumart_grid_v10_0_49_SMART_GRID_FLOW_HYBRID.fb2k-component
# - foo_albumart_grid_v10_0_49_SMART_GRID_FLOW_HYBRID.fb2k-component (copy in repo root)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $repoRoot

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (!(Test-Path $vswhere)) {
    throw "vswhere.exe not found at: $vswhere"
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
if (-not $vsPath) {
    throw "Visual Studio with MSBuild not found (vswhere returned empty)."
}

$msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
if (!(Test-Path $msbuild)) {
    throw "MSBuild.exe not found at: $msbuild"
}

$config = "Release"
$platform = "x64"

Write-Host "Using MSBuild: $msbuild" -ForegroundColor Cyan

Write-Host "`nBuilding SDK libs (pfc, shared, foobar2000_SDK)..." -ForegroundColor Cyan
& $msbuild "SDK-2025-03-07/pfc/pfc.vcxproj" /p:Configuration=$config /p:Platform=$platform /m
& $msbuild "SDK-2025-03-07/foobar2000/shared/shared.vcxproj" /p:Configuration=$config /p:Platform=$platform /m
& $msbuild "SDK-2025-03-07/foobar2000/SDK/foobar2000_SDK.vcxproj" /p:Configuration=$config /p:Platform=$platform /m

Write-Host "`nBuilding foobar2000_component_client.lib..." -ForegroundColor Cyan
& $msbuild "SDK-2025-03-07/foobar2000/foobar2000_component_client/foobar2000_component_client.vcxproj" /p:Configuration=$config /p:Platform=$platform /m

Write-Host "`nBuilding component..." -ForegroundColor Cyan
& $msbuild "component/foo_albumart_grid.vcxproj" /p:Configuration=$config /p:Platform=$platform /m

$distComponent = "dist/foo_albumart_grid_v10_0_49_SMART_GRID_FLOW_HYBRID.fb2k-component"
if (!(Test-Path $distComponent)) {
    throw "Expected component not found: $distComponent"
}

Copy-Item -Force $distComponent "foo_albumart_grid_v10_0_49_SMART_GRID_FLOW_HYBRID.fb2k-component"

# Optional: mirror output to your "real" project dist folder for local inspection
# - Set env var FOO_ALBUMART_GRID_DIST_MIRROR to a directory path.
$mirrorDir = $env:FOO_ALBUMART_GRID_DIST_MIRROR
if (-not $mirrorDir) {
    $defaultMirror = "C:\Users\mail\Desktop\Claude Expert Projects\Projects\foo_albumart_grid\dist"
    if (Test-Path $defaultMirror) { $mirrorDir = $defaultMirror }
}

if ($mirrorDir) {
    New-Item -ItemType Directory -Force $mirrorDir | Out-Null
    Copy-Item -Force $distComponent (Join-Path $mirrorDir (Split-Path $distComponent -Leaf))
}

$hash = (Get-FileHash $distComponent -Algorithm SHA256).Hash
$sizeKb = [math]::Round((Get-Item $distComponent).Length / 1KB)

Write-Host "`nBuild complete." -ForegroundColor Green
Write-Host "Output: $distComponent (${sizeKb} KB)" -ForegroundColor Yellow
Write-Host "SHA256: $hash" -ForegroundColor Yellow

