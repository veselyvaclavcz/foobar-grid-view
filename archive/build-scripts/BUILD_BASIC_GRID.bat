@echo off
echo Building Basic Grid Component...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=SDK-2025-03-07

cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    grid_component_basic.cpp

link /DLL /OUT:foo_albumart_grid.dll ^
    grid_component_basic.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid.dll (
    echo.
    echo BUILD SUCCESSFUL!
    echo Component: foo_albumart_grid.dll v3.0.0
    echo.
    echo FEATURES:
    echo - Shows album grid in separate window
    echo - Displays album art availability
    echo - Double-click to play album
    echo - Smooth scrolling support
    echo.
    echo To use: View -^> Album Art Grid
    del *.obj 2>nul
)

pause