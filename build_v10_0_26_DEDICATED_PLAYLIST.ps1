# PowerShell build script for v10.0.26 DEDICATED PLAYLIST - Enhanced User Workflow
Write-Host "===========================================" -ForegroundColor Green
Write-Host "Building v10.0.26 DEDICATED PLAYLIST - Enhanced User Workflow" -ForegroundColor Green
Write-Host "FEATURE: Single dedicated playlist prevents playlist multiplication" -ForegroundColor Red
Write-Host "===========================================" -ForegroundColor Green
Write-Host ""

$VSDIR = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
$WINKIT = "C:\Program Files (x86)\Windows Kits\10"
$WINKITVER = "10.0.22621.0"
$SDK_PATH = "SDK-2025-03-07"

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Name "build"
}

Write-Host "Compiling v10.0.26 DEDICATED PLAYLIST with enhanced workflow..." -ForegroundColor Yellow
Write-Host "DEDICATED PLAYLIST FEATURES:" -ForegroundColor Yellow
Write-Host "- NEW: 'Play in Album Grid Playlist' option prevents playlist multiplication" -ForegroundColor Green
Write-Host "- NEW: Single reusable 'Album Grid' playlist for all operations" -ForegroundColor Green  
Write-Host "- ENHANCED: Streamlined workflow - viewer to playlist in one click" -ForegroundColor Green
Write-Host "- MAINTAINED: All COM crash fixes and RAII protections from v10.0.25a" -ForegroundColor Red
Write-Host "- MAINTAINED: All destructor and race condition fixes" -ForegroundColor Red
Write-Host "- FOCUS: Grid as viewer, not playlist manager" -ForegroundColor Yellow
Write-Host ""

# Compile
$compileArgs = @(
    "/nologo", "/c", "/EHsc", "/MD", "/O2", "/std:c++17", "/DNDEBUG",
    "/DUNICODE", "/D_UNICODE", "/DFOOBAR2000_TARGET_VERSION=80",
    "/I`"$VSDIR\include`"",
    "/I`"$WINKIT\Include\$WINKITVER\um`"",
    "/I`"$WINKIT\Include\$WINKITVER\shared`"",
    "/I`"$WINKIT\Include\$WINKITVER\ucrt`"",
    "/I.",
    "/I$SDK_PATH",
    "/I$SDK_PATH\foobar2000",
    "/I$SDK_PATH\foobar2000\SDK",
    "/I$SDK_PATH\pfc",
    "/Fo:build\",
    "temp_fixed_base_v10_0_20_PERSISTENT_BLACKLIST_FIX.cpp"
)

& "$VSDIR\bin\Hostx64\x64\cl.exe" @compileArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "Compilation failed!" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""
Write-Host "Linking..." -ForegroundColor Yellow

# Link
$linkArgs = @(
    "/nologo", "/DLL", "/out:build\foo_albumart_grid_v10_0_26_DEDICATED_PLAYLIST.dll",
    "/MACHINE:X64",
    "/LIBPATH:`"$VSDIR\lib\x64`"",
    "/LIBPATH:`"$WINKIT\Lib\$WINKITVER\um\x64`"",
    "/LIBPATH:`"$WINKIT\Lib\$WINKITVER\ucrt\x64`"",
    "build\temp_fixed_base_v10_0_20_PERSISTENT_BLACKLIST_FIX.obj",
    "`"$SDK_PATH\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib`"",
    "`"$SDK_PATH\x64\Release\foobar2000_SDK.lib`"",
    "`"$SDK_PATH\x64\Release\pfc.lib`"",
    "`"$SDK_PATH\x64\Release\shared.lib`"",
    "kernel32.lib", "user32.lib", "gdi32.lib", "gdiplus.lib", "comctl32.lib", 
    "shell32.lib", "ole32.lib", "shlwapi.lib", "uxtheme.lib",
    "/NODEFAULTLIB:LIBCMT"
)

& "$VSDIR\bin\Hostx64\x64\link.exe" @linkArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "Linking failed!" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""
Write-Host "===========================================" -ForegroundColor Green
Write-Host "BUILD SUCCESSFUL!" -ForegroundColor Green
Write-Host "===========================================" -ForegroundColor Green
Write-Host ""

# Create component
if (Test-Path "build\foo_albumart_grid_v10_0_26_DEDICATED_PLAYLIST.dll") {
    Get-ChildItem "build\foo_albumart_grid_v10_0_26_DEDICATED_PLAYLIST.dll"
    
    Copy-Item "build\foo_albumart_grid_v10_0_26_DEDICATED_PLAYLIST.dll" "foo_albumart_grid.dll"
    
    Compress-Archive -Path "foo_albumart_grid.dll" -DestinationPath "foo_albumart_grid_v10_0_26_DEDICATED_PLAYLIST.zip" -Force
    Move-Item "foo_albumart_grid_v10_0_26_DEDICATED_PLAYLIST.zip" "foo_albumart_grid_v10_0_26_DEDICATED_PLAYLIST.fb2k-component" -Force
    
    Write-Host ""
    Write-Host "DEDICATED PLAYLIST COMPONENT READY!" -ForegroundColor Yellow
    Write-Host "foo_albumart_grid_v10_0_26_DEDICATED_PLAYLIST.fb2k-component" -ForegroundColor Green
    Get-ChildItem "foo_albumart_grid_v10_0_26_DEDICATED_PLAYLIST.fb2k-component"
    
    Write-Host ""
    Write-Host "ENHANCED WORKFLOW FEATURES:" -ForegroundColor Yellow
    Write-Host "- ELIMINATED playlist multiplication with single 'Album Grid' playlist" -ForegroundColor Green
    Write-Host "- ADDED 'Play in Album Grid Playlist' menu option" -ForegroundColor Green
    Write-Host "- STREAMLINED viewer-to-playback workflow in one action" -ForegroundColor Green
    Write-Host "- MAINTAINED all crash prevention and compatibility fixes" -ForegroundColor Green
    Write-Host "- OPTIMIZED for album browsing and immediate playback" -ForegroundColor Green
    Write-Host ""
    Write-Host "Grid now functions as pure VIEWER with direct playlist access!" -ForegroundColor Green
    Write-Host "No more playlist multiplication - one dedicated playlist for all operations." -ForegroundColor Yellow
} else {
    Write-Host "Build output not found!" -ForegroundColor Red
}