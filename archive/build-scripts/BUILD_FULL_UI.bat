@echo off
echo ========================================================================
echo Building Full Album Art Grid Component with Complete UI
echo ========================================================================
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

set SDK_PATH=SDK-2025-03-07

echo Compiling full UI component with C++17...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    albumart_grid_full.cpp

if exist albumart_grid_full.obj (
    echo.
    echo Linking component with all required libraries...
    link /DLL /OUT:foo_albumart_grid.dll ^
        albumart_grid_full.obj ^
        "%SDK_PATH%\foobar2000\foobar2000_component_client\foobar2000_component_client.lib" ^
        "%SDK_PATH%\foobar2000\SDK\foobar2000_SDK.lib" ^
        "%SDK_PATH%\pfc\pfc.lib" ^
        kernel32.lib user32.lib gdi32.lib comctl32.lib shlwapi.lib ole32.lib gdiplus.lib ^
        /NODEFAULTLIB:LIBCMT

    if exist foo_albumart_grid.dll (
        echo.
        echo ========================================================================
        echo BUILD SUCCESSFUL!
        echo ========================================================================
        echo Component: foo_albumart_grid.dll (v2.0.0)
        echo.
        echo FEATURES:
        echo - Full grid view with album artwork
        echo - Three grouping modes: Album, Folder, Artist
        echo - Double-click to play albums
        echo - Smooth scrolling and hover effects
        echo - Right-click context menu for options
        echo.
        echo INSTALLATION:
        echo 1. Copy foo_albumart_grid.dll to your foobar2000 components folder
        echo 2. Restart foobar2000
        echo 3. Go to View -^> Layout -^> Edit Layout
        echo 4. Right-click and choose "Replace UI Element"
        echo 5. Select "Album Art Grid" from the list
        echo.
        del *.obj 2>nul
    ) else (
        echo.
        echo ERROR: Linking failed! Check for missing libraries above.
    )
) else (
    echo.
    echo ERROR: Compilation failed! Check for syntax errors above.
)

pause