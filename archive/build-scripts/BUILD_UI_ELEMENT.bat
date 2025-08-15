@echo off
echo Building Album Art Grid UI Element...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=SDK-2025-03-07

echo Compiling UI Element (v4.0)...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    grid_ui_element.cpp

echo.
echo Linking...
link /DLL /OUT:foo_albumart_grid.dll ^
    grid_ui_element.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Component: foo_albumart_grid.dll v4.0.0
    echo.
    echo FEATURES:
    echo - UI Element for Layout Editor
    echo - Grid view with album placeholders
    echo - Double-click to play album
    echo - Right-click context menu
    echo - Hover and selection effects
    echo - Smooth scrolling
    echo.
    echo TO USE:
    echo 1. Copy to foobar2000 components folder
    echo 2. Restart foobar2000
    echo 3. View -^> Layout -^> Edit Layout
    echo 4. Right-click -^> Add New UI Element
    echo 5. Select "Album Art Grid"
    echo.
    del *.obj 2>nul
) else (
    echo.
    echo BUILD FAILED!
)

pause