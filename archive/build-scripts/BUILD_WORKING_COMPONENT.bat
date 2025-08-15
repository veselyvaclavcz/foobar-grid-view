@echo off
echo ========================================================================
echo Building Working Album Art Grid Component
echo ========================================================================
echo.

cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo Building minimal component without SDK dependencies...
echo.

cl.exe /LD /O2 /MD /EHsc /std:c++17 album_grid_minimal.cpp /Fe:foo_albumart_grid.dll

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================================================
    echo BUILD SUCCESSFUL!
    echo ========================================================================
    echo.
    echo Component created: foo_albumart_grid.dll
    echo.
    echo Features:
    echo - 12 sample albums with colored placeholders
    echo - Grid layout with scrolling
    echo - Click to select, double-click shows message
    echo - Dark theme
    echo - No SDK dependencies required
    echo.
    echo To install:
    echo 1. Close foobar2000
    echo 2. Delete old foo_albumart_grid.dll from components
    echo 3. Copy this new foo_albumart_grid.dll to components
    echo 4. Start foobar2000
    echo.
    echo Note: This is a working demonstration. Full library integration
    echo requires the complete SDK setup.
    echo.
) else (
    echo Build failed!
)

pause