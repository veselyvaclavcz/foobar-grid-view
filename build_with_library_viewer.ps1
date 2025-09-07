# PowerShell build script for v10.0.18 WITH LIBRARY VIEWER SERVICE
Write-Host "=========================================" -ForegroundColor Green
Write-Host "Building v10.0.18 WITH LIBRARY VIEWER" -ForegroundColor Green
Write-Host "Added library_viewer service interface" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
Write-Host ""

$VSDIR = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
$WINKIT = "C:\Program Files (x86)\Windows Kits\10"
$WINKITVER = "10.0.22621.0"
$SDK_PATH = "SDK-2025-03-07"

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Name "build"
}

Write-Host "Compiling v10.0.18 with library_viewer service..." -ForegroundColor Yellow
Write-Host "CRITICAL ADDITION: Added library_viewer service to appear in Media Library viewers list" -ForegroundColor Yellow
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
    "/OUT:build\foo_albumart_grid_v10_0_18_WITH_LIBRARY_VIEWER.dll",
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

if (Test-Path "build\foo_albumart_grid_v10_0_18_WITH_LIBRARY_VIEWER.dll") {
    Write-Host ""
    Write-Host "=========================================" -ForegroundColor Green
    Write-Host "BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "=========================================" -ForegroundColor Green
    
    Get-ChildItem "build\foo_albumart_grid_v10_0_18_WITH_LIBRARY_VIEWER.dll"
    
    Copy-Item "build\foo_albumart_grid_v10_0_18_WITH_LIBRARY_VIEWER.dll" "foo_albumart_grid.dll"
    
    Compress-Archive -Path "foo_albumart_grid.dll" -DestinationPath "foo_albumart_grid_v10_0_18_WITH_LIBRARY_VIEWER.zip" -Force
    Move-Item "foo_albumart_grid_v10_0_18_WITH_LIBRARY_VIEWER.zip" "foo_albumart_grid_v10_0_18_WITH_LIBRARY_VIEWER.fb2k-component" -Force
    
    Write-Host ""
    Write-Host "COMPONENT WITH LIBRARY VIEWER READY!" -ForegroundColor Green -BackgroundColor DarkGreen
    Write-Host "foo_albumart_grid_v10_0_18_WITH_LIBRARY_VIEWER.fb2k-component" -ForegroundColor Green
    Get-ChildItem "foo_albumart_grid_v10_0_18_WITH_LIBRARY_VIEWER.fb2k-component"
    Write-Host ""
    Write-Host "CRITICAL ADDITION APPLIED:" -ForegroundColor Yellow
    Write-Host "- Added library_viewer service implementation" -ForegroundColor Green
    Write-Host "- Component should now appear in 'Installed media library viewers' dropdown" -ForegroundColor Green
    Write-Host "- Includes all v10.0.18 features (Page Up/Down, Unicode, Letter jump)" -ForegroundColor Green
    Write-Host "- Based on working v10.0.17 FINAL with correct GUID" -ForegroundColor Green
    Write-Host ""
    Write-Host "Album Art Grid should now appear in Media Library preferences!" -ForegroundColor Cyan
} else {
    Write-Host "BUILD FAILED!" -ForegroundColor Red
}

Write-Host ""
Read-Host "Press Enter to continue"