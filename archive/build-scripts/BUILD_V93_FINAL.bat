@echo off
echo Building Album Art Grid v9.3.0 FINAL - Extended Grouping Options...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=SDK-2025-03-07

echo Compiling v9.3.0 FINAL...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    grid_v8_final.cpp

echo.
echo Linking...
link /DLL /OUT:foo_albumart_grid_v93.dll ^
    grid_v8_final.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib gdiplus.lib comctl32.lib shell32.lib ole32.lib shlwapi.lib uxtheme.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid_v93.dll (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Component: foo_albumart_grid_v93.dll v9.3.0
    echo.
    echo EXTENDED GROUPING OPTIONS:
    echo - By Folder (original)
    echo - By Directory (parent directory name)
    echo - By Album
    echo - By Artist
    echo - By Album Artist
    echo - By Artist/Album
    echo - By Performer
    echo - By Composer
    echo - By Genre
    echo - By Year
    echo - By Label (record label)
    echo - By Rating
    echo - By Comment
    echo - By Custom Pattern (titleformat)
    echo.
    echo OTHER FEATURES:
    echo - Fixed tooltip positioning
    echo - Dynamic text height
    echo - Auto-fill mode with Ctrl+Wheel
    echo - High-quality image rendering
    echo.
    del *.obj 2>nul
) else (
    echo.
    echo BUILD FAILED!
)

pause