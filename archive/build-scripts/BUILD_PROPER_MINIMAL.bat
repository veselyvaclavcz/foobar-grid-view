@echo off
echo ========================================================================
echo Building Proper Minimal Component for foobar2000 v2
echo ========================================================================
echo.

cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo Compiling album_grid_proper_minimal.cpp...
echo This version has correct vtable layout and should not crash.
echo.

cl.exe /LD /O2 /MD /EHsc /GR album_grid_proper_minimal.cpp /Fe:foo_albumart_grid.dll /link /SUBSYSTEM:WINDOWS user32.lib

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================================================
    echo BUILD SUCCESSFUL!
    echo ========================================================================
    echo.
    echo Component created: foo_albumart_grid.dll
    echo.
    echo This component:
    echo - Has correct vtable layout
    echo - Implements all required methods
    echo - Should load without crashes
    echo.
    echo To install:
    echo 1. Close foobar2000 v2
    echo 2. Run: powershell -ExecutionPolicy Bypass -File install_component.ps1
    echo 3. Start foobar2000 v2
    echo.
) else (
    echo.
    echo Build failed! Check compiler output above.
)

pause