@echo off
echo ========================================================================
echo Building Album Art Grid Component for foobar2000 v2
echo ========================================================================
echo.

cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo Compiling album_grid_v2_fixed.cpp for foobar2000 v2...
cl.exe /LD /O2 /MD /EHsc /std:c++17 album_grid_v2_fixed.cpp /Fe:foo_albumart_grid.dll

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================================================
    echo BUILD SUCCESSFUL!
    echo ========================================================================
    echo.
    echo Component created: foo_albumart_grid.dll
    echo Version: 2.0.0 (for foobar2000 v2)
    echo.
    echo Features:
    echo - foobar2000 v2 compatible entry points
    echo - Proper component interface structure
    echo - 15 sample albums with colored placeholders
    echo - Smooth scrolling and navigation
    echo - Dark theme interface
    echo.
    echo Installation:
    echo 1. Close foobar2000 v2 completely
    echo 2. Copy foo_albumart_grid.dll to:
    echo    %%APPDATA%%\foobar2000-v2\user-components-x64\foo_albumart_grid\
    echo 3. Start foobar2000 v2
    echo.
    echo The component should now load without errors.
    echo.
) else (
    echo.
    echo Build failed! Check compiler output above.
)

pause