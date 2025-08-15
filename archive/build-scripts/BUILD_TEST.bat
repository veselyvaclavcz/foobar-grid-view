@echo off
echo Building ultra-minimal test component...
echo.

cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

cl.exe /LD /O2 /MD test_minimal.cpp /Fe:foo_albumart_grid.dll /link /SUBSYSTEM:WINDOWS

if exist foo_albumart_grid.dll (
    echo.
    echo Component created: foo_albumart_grid.dll
    echo.
    echo This is the simplest possible component.
    echo If this crashes, the issue is in how foobar2000 handles NULL returns.
) else (
    echo Build failed!
)

pause