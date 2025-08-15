@echo off
echo ========================================================================
echo Building Album Art Grid UI Component
echo ========================================================================
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

set SDK_PATH=SDK-2025-03-07

echo Building component with UI element (C++17)...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    album_grid_ui.cpp

if exist album_grid_ui.obj (
    echo Linking component...
    link /DLL /OUT:foo_albumart_grid.dll ^
        album_grid_ui.obj ^
        foobar2000_component_client.lib ^
        foobar2000_SDK.lib ^
        pfc.lib ^
        kernel32.lib user32.lib gdi32.lib comctl32.lib shlwapi.lib ole32.lib gdiplus.lib ^
        /NODEFAULTLIB:LIBCMT

    if exist foo_albumart_grid.dll (
        echo.
        echo ========================================================================
        echo BUILD SUCCESSFUL!
        echo ========================================================================
        echo Component created: foo_albumart_grid.dll
        echo.
        echo This version includes a UI element that can be added to layouts.
        echo.
        echo To use:
        echo 1. Install the component
        echo 2. Restart foobar2000
        echo 3. Go to View -^> Layout -^> Edit Layout
        echo 4. Right-click and choose "Replace UI Element"
        echo 5. Look for "Album Art Grid" in the list
        del *.obj 2>nul
    ) else (
        echo Linking failed. Check for errors above.
    )
) else (
    echo Compilation failed. Check for errors above.
)

pause