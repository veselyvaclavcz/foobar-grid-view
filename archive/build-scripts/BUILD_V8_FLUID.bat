@echo off
echo Building Album Art Grid v8.3.0 with Fluid Dynamic Resizing...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=SDK-2025-03-07

echo Compiling v8.3.0 with Fluid Dynamic Resizing...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    grid_v8_fluid.cpp

echo.
echo Linking...
link /DLL /OUT:foo_albumart_grid.dll ^
    grid_v8_fluid.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib gdiplus.lib comctl32.lib shell32.lib ole32.lib shlwapi.lib uxtheme.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Component: foo_albumart_grid.dll v8.3.0
    echo.
    echo NEW FEATURES:
    echo - Fluid Dynamic Resizing Modes:
    echo   * Auto-Fit: Minimizes gaps between items
    echo   * Auto-Fill: Stretches items to fill width
    echo   * Fluid: Dynamically adjusts size and padding
    echo - Automatic container width detection
    echo - Perfect grid alignment
    echo - Maintains aspect ratio
    echo - Debug info overlay showing current mode
    echo.
    echo EXISTING FEATURES:
    echo - Dark mode scrollbar support
    echo - Multi-select support
    echo - Full Unicode text
    echo - Group by Folder/Album/Artist
    echo - High-quality thumbnails
    echo.
    del *.obj 2>nul
) else (
    echo.
    echo BUILD FAILED!
)

pause