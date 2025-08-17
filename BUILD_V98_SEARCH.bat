@echo off
echo Building Album Art Grid v9.8.8 - Final...
echo.

REM Navigate to project directory
cd /d "C:\Users\mail\Desktop\Claude Expert Projects\Projects\foo_albumart_grid"

REM Set Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
if errorlevel 1 (
    echo Setting up Visual Studio environment...
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
)

REM Delete old files
del grid_v98_playlist_fixed.obj 2>nul
del foo_albumart_grid_v98.dll 2>nul
del foo_albumart_grid_v98.fb2k-component 2>nul

REM Compile the component
echo Compiling grid_v98_playlist_fixed.cpp...
cl.exe /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=82 ^
    /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" /I"SDK-2025-03-07\pfc" ^
    grid_v98_playlist_fixed.cpp

if errorlevel 1 (
    echo.
    echo Compilation failed!
    pause
    exit /b 1
)

REM Link the DLL
echo Linking foo_albumart_grid_v98.dll...
link.exe /DLL /OUT:foo_albumart_grid_v98.dll grid_v98_playlist_fixed.obj ^
    "SDK-2025-03-07\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "SDK-2025-03-07\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "SDK-2025-03-07\pfc\x64\Release\pfc.lib" ^
    "SDK-2025-03-07\foobar2000\shared\shared-x64.lib" ^
    user32.lib gdi32.lib shell32.lib advapi32.lib comdlg32.lib shlwapi.lib ^
    gdiplus.lib comctl32.lib uxtheme.lib ole32.lib ^
    /SUBSYSTEM:WINDOWS

if errorlevel 1 (
    echo.
    echo Linking failed!
    pause
    exit /b 1
)

REM Create the component package
echo Creating foo_albumart_grid_v98.fb2k-component...
powershell -Command "Compress-Archive -Path 'foo_albumart_grid_v98.dll' -DestinationPath 'foo_albumart_grid_v98.zip' -Force"
ren foo_albumart_grid_v98.zip foo_albumart_grid_v98.fb2k-component

REM Clean up
del grid_v98_playlist_fixed.obj >nul 2>&1
del foo_albumart_grid_v98.exp >nul 2>&1
del foo_albumart_grid_v98.lib >nul 2>&1

echo.
echo ========================================
echo Build complete!
echo.
echo Component: foo_albumart_grid_v98.fb2k-component
echo.
echo NEW IN v9.8.8:
echo - Press Ctrl+Shift+S to toggle search box (open/close)
echo - Type to filter albums in real-time
echo - Searches in album, artist, genre fields
echo - Press Ctrl+Shift+S again to close
echo - Or use right-click menu: Search option
echo.
echo ALSO INCLUDES:
echo - Direct Play/Add commands in context menu
echo - Double-Click Action configuration submenu
echo.
echo To install: Double-click the .fb2k-component file
echo ========================================
pause