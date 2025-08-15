@echo off
echo Building Album Art Grid v9.1.0 FINAL - Dynamic Text Sizing...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=SDK-2025-03-07

echo Compiling v9.1.0 FINAL...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    grid_v8_final.cpp

echo.
echo Linking...
link /DLL /OUT:foo_albumart_grid_v91.dll ^
    grid_v8_final.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib gdiplus.lib comctl32.lib shell32.lib ole32.lib shlwapi.lib uxtheme.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid_v91.dll (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Component: foo_albumart_grid_v91.dll v9.1.0
    echo.
    echo IMPROVEMENTS:
    echo - Dynamic text height based on actual font metrics
    echo - Proper multi-line text display (1-3 lines)
    echo - Uses foobar2000's font preferences
    echo - Auto-fill mode with Ctrl+Wheel (3-10 columns)
    echo - High-quality image rendering
    echo - Smart tooltips (only when text is hidden)
    echo - Fixed selection alignment
    echo - Dark mode support
    echo.
    del *.obj 2>nul
) else (
    echo.
    echo BUILD FAILED!
)

pause