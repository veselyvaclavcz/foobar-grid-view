# PowerShell build script for v10.0.18 MPV RETRY FIX - Stop Infinite Album Art Retry Attempts
Write-Host "===========================================" -ForegroundColor Green
Write-Host "Building v10.0.18 MPV RETRY FIX - Stop Infinite Album Art Retry Attempts" -ForegroundColor Green
Write-Host "CRITICAL: Fixed infinite retry attempts that cause mpv libav errors" -ForegroundColor Green
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

Write-Host "Compiling v10.0.18 MPV RETRY FIX with blacklist cache for missing album art..." -ForegroundColor Yellow
Write-Host "CRITICAL FIXES:" -ForegroundColor Yellow
Write-Host "- Fixed infinite retry attempts for tracks without album art (root cause of mpv errors)" -ForegroundColor Yellow
Write-Host "- Added blacklist cache to prevent repeated artwork loading attempts" -ForegroundColor Yellow
Write-Host "- Items without artwork are remembered and not queried again for 5 minutes" -ForegroundColor Yellow
Write-Host "- Completely eliminates exponential retry behavior that floods foo_mpv" -ForegroundColor Yellow
Write-Host "- Fixes marc2k3's reported issue: 'component should remember that nothing was found'" -ForegroundColor Yellow
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
    "temp_fixed_base_v10_0_18_MPV_RETRY_FIX.cpp"
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
    "/OUT:build\foo_albumart_grid_v10_0_18_MPV_RETRY_FIX.dll",
    "/MACHINE:X64",
    "/LIBPATH:`"$VSDIR\lib\x64`"",
    "/LIBPATH:`"$WINKIT\Lib\$WINKITVER\um\x64`"",
    "/LIBPATH:`"$WINKIT\Lib\$WINKITVER\ucrt\x64`"",
    "build\temp_fixed_base_v10_0_18_MPV_RETRY_FIX.obj",
    "`"$SDK_PATH\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib`"",
    "`"$SDK_PATH\x64\Release\foobar2000_SDK.lib`"",
    "`"$SDK_PATH\x64\Release\pfc.lib`"",
    "`"$SDK_PATH\x64\Release\shared.lib`"",
    "kernel32.lib", "user32.lib", "gdi32.lib", "gdiplus.lib", "comctl32.lib", 
    "shell32.lib", "ole32.lib", "shlwapi.lib", "uxtheme.lib",
    "/NODEFAULTLIB:LIBCMT"
)

& "$VSDIR\bin\Hostx64\x64\link.exe" @linkArgs

if (Test-Path "build\foo_albumart_grid_v10_0_18_MPV_RETRY_FIX.dll") {
    Write-Host ""
    Write-Host "===========================================" -ForegroundColor Green
    Write-Host "BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "===========================================" -ForegroundColor Green
    
    Get-ChildItem "build\foo_albumart_grid_v10_0_18_MPV_RETRY_FIX.dll"
    
    Copy-Item "build\foo_albumart_grid_v10_0_18_MPV_RETRY_FIX.dll" "foo_albumart_grid.dll"
    
    Compress-Archive -Path "foo_albumart_grid.dll" -DestinationPath "foo_albumart_grid_v10_0_18_MPV_RETRY_FIX.zip" -Force
    Move-Item "foo_albumart_grid_v10_0_18_MPV_RETRY_FIX.zip" "foo_albumart_grid_v10_0_18_MPV_RETRY_FIX.fb2k-component" -Force
    
    Write-Host ""
    Write-Host "MPV RETRY FIX COMPONENT READY!" -ForegroundColor Green -BackgroundColor DarkGreen
    Write-Host "foo_albumart_grid_v10_0_18_MPV_RETRY_FIX.fb2k-component" -ForegroundColor Green
    Get-ChildItem "foo_albumart_grid_v10_0_18_MPV_RETRY_FIX.fb2k-component"
    Write-Host ""
    Write-Host "CRITICAL FIXES APPLIED:" -ForegroundColor Yellow
    Write-Host "- Fixed infinite retry behavior for tracks without album art (ROOT CAUSE of mpv errors)" -ForegroundColor Green
    Write-Host "- Added intelligent blacklist cache - remembers items without artwork for 5 minutes" -ForegroundColor Green
    Write-Host "- Eliminates exponential retry behavior that was flooding foo_mpv with requests" -ForegroundColor Green
    Write-Host "- Component now properly 'remembers that nothing was found' as marc2k3 suggested" -ForegroundColor Green
    Write-Host "- Blacklist automatically clears after 5 minutes to allow for library updates" -ForegroundColor Green
    Write-Host "- Maintains all v10.0.18 shutdown crash fixes and improvements" -ForegroundColor Green
    Write-Host ""
    Write-Host "This should COMPLETELY ELIMINATE the mpv libav error messages!" -ForegroundColor Cyan -BackgroundColor DarkCyan
    Write-Host "The real problem was infinite retry attempts, not video detection." -ForegroundColor Cyan
} else {
    Write-Host "BUILD FAILED!" -ForegroundColor Red
}

Write-Host ""
Read-Host "Press Enter to continue"