@echo off
echo Building Album Art Grid Component...
echo.

cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo Compiling simple_grid.cpp...

cl.exe /LD /O2 /MD /EHsc /std:c++17 simple_grid.cpp /Fe:foo_albumart_grid.dll /link user32.lib gdi32.lib comctl32.lib

if exist foo_albumart_grid.dll (
    echo.
    echo SUCCESS! Component created: foo_albumart_grid.dll
    echo.
    echo Features:
    echo - Grid view with 15 sample albums
    echo - Click to select
    echo - Scroll with mouse wheel
    echo - Colored album placeholders
    echo.
    echo Install to foobar2000\components folder
) else (
    echo Build failed
)

pause