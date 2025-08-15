@echo off
echo ========================================================================
echo Building Minimal Album Art Grid Component for foobar2000 v2
echo ========================================================================
echo.

cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo Compiling album_grid_minimal_v2.cpp...
cl.exe /LD /O2 /MD album_grid_minimal_v2.cpp /Fe:foo_albumart_grid.dll /link /SUBSYSTEM:WINDOWS

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================================================
    echo BUILD SUCCESSFUL!
    echo ========================================================================
    echo.
    echo Component created: foo_albumart_grid.dll
    echo Version: 2.0.2 (Minimal)
    echo.
    echo This is the absolute minimum component that should load without crashes.
    echo It doesn't provide any functionality but proves the component can load.
    echo.
) else (
    echo.
    echo Build failed! Check compiler output above.
)

pause