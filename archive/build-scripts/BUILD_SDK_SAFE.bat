@echo off
echo ========================================================================
echo Building Safe SDK-based Component for foobar2000 v2
echo ========================================================================
echo.

cd /d "%~dp0"

:: Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

set SDK_PATH=SDK-2025-03-07

echo Compiling minimal_safe_component.cpp with SDK...
echo.

:: Compile with SDK includes
cl.exe /LD /O2 /MD /EHsc /GR ^
    /I"%SDK_PATH%" ^
    /I"%SDK_PATH%\foobar2000" ^
    /I"%SDK_PATH%\foobar2000\SDK" ^
    /I"%SDK_PATH%\pfc" ^
    /DUNICODE /D_UNICODE ^
    /DWIN32 /D_WINDOWS ^
    /DFOOBAR2000_TARGET_VERSION=80 ^
    minimal_safe_component.cpp ^
    /Fe:foo_albumart_grid.dll ^
    /link /SUBSYSTEM:WINDOWS user32.lib

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================================================
    echo BUILD SUCCESSFUL!
    echo ========================================================================
    echo.
    echo Component created: foo_albumart_grid.dll
    echo.
    echo This component includes the actual SDK headers and should be
    echo compatible with foobar2000 v2.
    echo.
) else (
    echo.
    echo Build may have failed. Check for compiler errors above.
    echo If there are missing symbols, we may need to compile SDK sources.
)

pause