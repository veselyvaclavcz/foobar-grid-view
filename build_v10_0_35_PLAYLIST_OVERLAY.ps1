# Build script for Album Art Grid v10.0.35 PLAYLIST OVERLAY
# This version adds playlist overlay functionality for 3x3 enlarged now playing artwork

$ErrorActionPreference = "Stop"

# Configuration
$sourceFile = "foo_albumart_grid_v10_0_35_PLAYLIST_OVERLAY_ENHANCED.cpp"
$outputName = "foo_albumart_grid"
$version = "v10_0_35_PLAYLIST_OVERLAY_ENHANCED"
$defFile = "foo_albumart_grid.def"

# Visual Studio paths
$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\Community"
$vcvarsall = "$vsPath\VC\Auxiliary\Build\vcvarsall.bat"

# SDK paths
$sdkPath = "C:\Users\mail\Desktop\Claude Expert Projects\Projects\foo_albumart_grid\SDK-2025-03-07"
$fbInclude = "$sdkPath\foobar2000\SDK"
$fbLib = "$sdkPath\foobar2000\SDK\x64\Release\foobar2000_SDK.lib"
$pfcLib = "$sdkPath\pfc\x64\Release\pfc.lib"
$sharedLib = "$sdkPath\foobar2000\shared\shared-x64.lib"
$componentClientLib = "$sdkPath\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib"

# Clear previous outputs
Write-Host "Cleaning previous build..." -ForegroundColor Yellow
Remove-Item -Path "build\*.obj", "build\*.exp", "build\*.lib", "build\*.dll" -ErrorAction SilentlyContinue

# Create build directory if it doesn't exist
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Compile
Write-Host "`nCompiling $sourceFile..." -ForegroundColor Cyan
$compileCmd = @"
"$vcvarsall" x64 && cl.exe /c /O2 /GL /std:c++17 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_UNICODE" /D "UNICODE" /GF /Gm- /EHsc /MD /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /permissive- /external:W0 /Gd /TP /FC /I"$fbInclude" /I"$sdkPath" "$sourceFile" /Fo"build\$outputName.obj" 2>&1
"@

$output = cmd /c $compileCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed!" -ForegroundColor Red
    Write-Host $output
    exit 1
}
Write-Host "Compilation successful!" -ForegroundColor Green

# Link
Write-Host "`nLinking..." -ForegroundColor Cyan
$linkCmd = @"
"$vcvarsall" x64 && link.exe /OUT:"build\${outputName}_${version}.dll" /DLL /LTCG /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /MACHINE:X64 "build\$outputName.obj" "$fbLib" "$pfcLib" "$sharedLib" "$componentClientLib" kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib gdiplus.lib shlwapi.lib uxtheme.lib msimg32.lib 2>&1
"@

$output = cmd /c $linkCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "Linking failed!" -ForegroundColor Red
    Write-Host $output
    exit 1
}
Write-Host "Linking successful!" -ForegroundColor Green

# Create fb2k-component
Write-Host "`nCreating fb2k-component..." -ForegroundColor Cyan
$componentPath = "${outputName}_${version}.fb2k-component"

# Create temporary directory for correct DLL name
$tempDir = "temp_component"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
Copy-Item "build\${outputName}_${version}.dll" "$tempDir\${outputName}.dll"

# Create ZIP with correct DLL name
Compress-Archive -Path "$tempDir\${outputName}.dll" -DestinationPath "$componentPath.zip" -Force
Move-Item "$componentPath.zip" $componentPath
Remove-Item -Recurse -Force $tempDir

if (Test-Path $componentPath) {
    $size = (Get-Item $componentPath).Length / 1KB
    Write-Host "`nBuild complete!" -ForegroundColor Green
    Write-Host "Output: $componentPath (${size:N0} KB)" -ForegroundColor Yellow
    Write-Host "`nVersion 10.0.35 - PLAYLIST OVERLAY" -ForegroundColor Magenta
    Write-Host "- NEW: Playlist Overlay for 3x3 Enlarged Now Playing" -ForegroundColor Green
    Write-Host "- NEW: Track list display with current track highlighting" -ForegroundColor Green
    Write-Host "- NEW: Play triangle indicator for current track" -ForegroundColor Green
    Write-Host "- NEW: Smart scrolling for long playlists with scroll indicators" -ForegroundColor Green
    Write-Host "- NEW: Theme-aware dark overlay background (85% opacity)" -ForegroundColor Green
    Write-Host "- NEW: Configuration option in 'Enlarged Now Playing' submenu" -ForegroundColor Green
    Write-Host "- MAINTAINED: Text overlay with perfect theme color adaptation" -ForegroundColor White
    Write-Host "- MAINTAINED: Grid layout preserved - enlarged artwork remains square" -ForegroundColor White
    Write-Host "- MAINTAINED: Page Up/Down navigation, Letter Jump, Unicode support" -ForegroundColor White
    Write-Host "- MAINTAINED: All performance optimizations and crash protection" -ForegroundColor White
} else {
    Write-Host "Failed to create component!" -ForegroundColor Red
    exit 1
}