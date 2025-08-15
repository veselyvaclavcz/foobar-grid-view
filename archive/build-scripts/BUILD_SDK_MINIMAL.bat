@echo off
echo ========================================================================
echo Building SDK-based Minimal Component for foobar2000 v2
echo ========================================================================
echo.

cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo Compiling album_grid_sdk_minimal.cpp...
echo This version implements the correct foobar2000_client interface.
echo.

cl.exe /LD /O2 /MD /GR album_grid_sdk_minimal.cpp /Fe:foo_albumart_grid.dll /link /SUBSYSTEM:WINDOWS

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================================================
    echo BUILD SUCCESSFUL!
    echo ========================================================================
    echo.
    echo Component created: foo_albumart_grid.dll
    echo.
    echo This minimal component implements the correct interface and should
    echo load without crashes. It doesn't provide functionality yet but 
    echo establishes the foundation for adding features.
    echo.
) else (
    echo.
    echo Build failed! Check compiler output above.
)

pause