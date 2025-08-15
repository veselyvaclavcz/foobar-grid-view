@echo off
echo Building Album Art Grid v9.5.0 with Artist Image Support...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=SDK-2025-03-07

echo Compiling v9.5.0 with Artist Image Support...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=82 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    grid_v95_artist.cpp

echo.
echo Linking...
link /DLL /OUT:foo_albumart_grid_v95.dll ^
    grid_v95_artist.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib gdiplus.lib comctl32.lib shell32.lib ole32.lib shlwapi.lib uxtheme.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid_v95.dll (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Component: foo_albumart_grid_v95.dll v9.5.0
    echo.
    echo NEW FEATURE - ARTIST IMAGE SUPPORT:
    echo - Shows artist images when grouping by:
    echo   * By Artist
    echo   * By Album Artist  
    echo   * By Artist/Album
    echo   * By Performer
    echo   * By Composer
    echo - Automatic fallback to album art if artist image not found
    echo - Visual indicators in menu (music note emoji)
    echo.
    echo ALL FEATURES FROM v9.4:
    echo - 13 grouping modes
    echo - 11 sorting options
    echo - Auto-fill mode
    echo - Ctrl+Mouse Wheel columns (3-10)
    echo - High-quality image rendering
    echo - Smart tooltips
    echo - Dynamic text display (1-3 lines)
    echo.
    del *.obj 2>nul
) else (
    echo.
    echo BUILD FAILED!
)

pause