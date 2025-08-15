@echo off
echo Building Enhanced Album Art Grid...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=SDK-2025-03-07

echo Compiling Enhanced UI Element (v5.0)...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    grid_ui_enhanced.cpp

echo.
echo Linking with all libraries...
link /DLL /OUT:foo_albumart_grid.dll ^
    grid_ui_enhanced.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib gdiplus.lib comctl32.lib shell32.lib ole32.lib shlwapi.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Component: foo_albumart_grid.dll v5.0.0
    echo.
    echo NEW FEATURES:
    echo - ACTUAL ALBUM ART DISPLAY!
    echo - Resizable grid (Ctrl+Mouse Wheel)
    echo - Right-click menu with size options
    echo - Uses foobar2000 theme colors
    echo - Configurable text display
    echo - High-quality image rendering
    echo.
    echo USAGE:
    echo - Right-click for size options
    echo - Ctrl+Wheel to resize on the fly
    echo - Double-click albums to play
    echo.
    del *.obj 2>nul
) else (
    echo.
    echo BUILD FAILED!
)

pause