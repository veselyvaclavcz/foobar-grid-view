@echo off
echo Building Album Art Grid v9.6.0 with Track Count Badge...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=SDK-2025-03-07

echo Compiling v9.6.0 with Track Count Badge feature...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=82 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    grid_v96_with_badges.cpp

echo.
echo Linking...
link /DLL /OUT:foo_albumart_grid_v96.dll ^
    grid_v96_with_badges.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib gdiplus.lib comctl32.lib shell32.lib ole32.lib shlwapi.lib uxtheme.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid_v96.dll (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Component: foo_albumart_grid_v96.dll v9.6.0
    echo.
    echo NEW IN v9.6 - TRACK COUNT BADGE:
    echo - Shows track count badge on album art when labels are hidden
    echo - Badge appears in top-right corner with theme colors
    echo - All features from v9.5 included including artist images
    echo.
    del *.obj 2>nul
) else (
    echo.
    echo BUILD FAILED!
)

pause