@echo off
echo Building Album Art Grid v9.7.0 Enhanced Badge and Tooltip Edition...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=SDK-2025-03-07

echo Compiling v9.7.0 with Enhanced Badge and Tooltip features...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=82 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    grid_v97_enhanced.cpp

echo.
echo Linking...
link /DLL /OUT:foo_albumart_grid_v97.dll ^
    grid_v97_enhanced.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib gdiplus.lib comctl32.lib shell32.lib ole32.lib shlwapi.lib uxtheme.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid_v97.dll (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo Component: foo_albumart_grid_v97.dll v9.7.0
    echo.
    echo NEW IN v9.7:
    echo - Track count badge ALWAYS visible when enabled
    echo - Tooltip shows full text for truncated labels
    echo - Badge uses exact UI font for consistency
    echo - All features from v9.6 and v9.5 included
    echo.
    del *.obj 2>nul
) else (
    echo.
    echo BUILD FAILED!
)

pause