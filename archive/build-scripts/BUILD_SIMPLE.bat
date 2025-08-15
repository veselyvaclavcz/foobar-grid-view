@echo off
echo Building Simple Working Component...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

set SDK_PATH=SDK-2025-03-07

cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    simple_working.cpp

link /DLL /OUT:foo_albumart_grid.dll ^
    simple_working.obj ^
    foobar2000_component_client.lib ^
    foobar2000_SDK.lib ^
    pfc.lib ^
    kernel32.lib user32.lib shlwapi.lib ole32.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid.dll (
    echo BUILD SUCCESSFUL!
    echo Component created: foo_albumart_grid.dll v1.5.0
    echo.
    echo Features:
    echo - Shows in Components list
    echo - Adds "Album Art Grid" to View menu
    echo - Shows library statistics
    del *.obj 2>nul
)

pause