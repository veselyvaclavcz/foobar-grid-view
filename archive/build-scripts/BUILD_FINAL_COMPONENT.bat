@echo off
echo ========================================================================
echo Building Album Art Grid Component with SDK
echo ========================================================================
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

set SDK_PATH=SDK-2025-03-07

echo Compiling component files...
cl /c /EHsc /MD /O2 /DNDEBUG /DUNICODE /D_UNICODE ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    stdafx.cpp main_component.cpp initquit.cpp

echo Linking component...
link /DLL /OUT:foo_albumart_grid.dll ^
    stdafx.obj main_component.obj initquit.obj ^
    /LIBPATH:. ^
    kernel32.lib user32.lib

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================================================
    echo BUILD SUCCESSFUL!
    echo ========================================================================
    echo Component created: foo_albumart_grid.dll
    del *.obj 2>nul
    del *.exp 2>nul
    del *.lib 2>nul
) else (
    echo Build failed! Check errors above.
)

pause