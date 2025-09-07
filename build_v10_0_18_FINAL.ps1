# PowerShell build script for v10.0.18 FINAL - Complete Shutdown Crash Fix
Write-Host "===========================================" -ForegroundColor Green
Write-Host "Building v10.0.18 FINAL - Complete Shutdown Crash Fix" -ForegroundColor Green
Write-Host "CRITICAL: Fixed shutdown crash in refresh_items()" -ForegroundColor Green
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

Write-Host "Compiling v10.0.18 FINAL with complete shutdown protection..." -ForegroundColor Yellow
Write-Host "CRITICAL FIXES:" -ForegroundColor Yellow
Write-Host "- Fixed access violation crash in refresh_items() during shutdown" -ForegroundColor Yellow
Write-Host "- Added core_api::is_shutting_down() checks in all playlist callbacks" -ForegroundColor Yellow
Write-Host "- Enhanced refresh_items() with playlist_manager/library_manager safety" -ForegroundColor Yellow
Write-Host "- Complete exception handling for shutdown scenarios" -ForegroundColor Yellow
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
    "temp_fixed_base_v10_0_18.cpp"
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
    "/DLL",
    "/OUT:build\foo_albumart_grid_v10_0_18_FINAL.dll",
    "/MACHINE:X64",
    "/LIBPATH:`"$VSDIR\lib\x64`"",
    "/LIBPATH:`"$WINKIT\Lib\$WINKITVER\um\x64`"",
    "/LIBPATH:`"$WINKIT\Lib\$WINKITVER\ucrt\x64`"",
    "build\temp_fixed_base_v10_0_18.obj",
    "`"$SDK_PATH\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib`"",
    "`"$SDK_PATH\x64\Release\foobar2000_SDK.lib`"",
    "`"$SDK_PATH\x64\Release\pfc.lib`"",
    "`"$SDK_PATH\x64\Release\shared.lib`"",
    "kernel32.lib", "user32.lib", "gdi32.lib", "gdiplus.lib", "comctl32.lib", 
    "shell32.lib", "ole32.lib", "shlwapi.lib", "uxtheme.lib",
    "/NODEFAULTLIB:LIBCMT"
)

& "$VSDIR\bin\Hostx64\x64\link.exe" @linkArgs

if (Test-Path "build\foo_albumart_grid_v10_0_18_FINAL.dll") {
    Write-Host ""
    Write-Host "===========================================" -ForegroundColor Green
    Write-Host "BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "===========================================" -ForegroundColor Green
    
    Get-ChildItem "build\foo_albumart_grid_v10_0_18_FINAL.dll"
    
    Copy-Item "build\foo_albumart_grid_v10_0_18_FINAL.dll" "foo_albumart_grid.dll"
    
    Compress-Archive -Path "foo_albumart_grid.dll" -DestinationPath "foo_albumart_grid_v10_0_18_FINAL.zip" -Force
    Move-Item "foo_albumart_grid_v10_0_18_FINAL.zip" "foo_albumart_grid_v10_0_18_FINAL.fb2k-component" -Force
    
    Write-Host ""
    Write-Host "SHUTDOWN CRASH FIXED COMPONENT READY!" -ForegroundColor Green -BackgroundColor DarkGreen
    Write-Host "foo_albumart_grid_v10_0_18_FINAL.fb2k-component" -ForegroundColor Green
    Get-ChildItem "foo_albumart_grid_v10_0_18_FINAL.fb2k-component"
    Write-Host ""
    Write-Host "CRITICAL FIXES APPLIED:" -ForegroundColor Yellow
    Write-Host "- Fixed access violation crash in refresh_items() during shutdown (failure_00000005)" -ForegroundColor Green
    Write-Host "- Added core_api::is_shutting_down() checks in all playlist callbacks" -ForegroundColor Green
    Write-Host "- Enhanced refresh_items() with playlist_manager/library_manager safety" -ForegroundColor Green
    Write-Host "- Complete exception handling for shutdown scenarios" -ForegroundColor Green
    Write-Host "- All previous race condition fixes preserved from v10.0.18 FIXED" -ForegroundColor Green
    Write-Host "- Library viewer integration maintained" -ForegroundColor Green
    Write-Host ""
    Write-Host "This should fix the shutdown crash when switching to playlist mode!" -ForegroundColor Cyan
} else {
    Write-Host "BUILD FAILED!" -ForegroundColor Red
}

Write-Host ""
Read-Host "Press Enter to continue"