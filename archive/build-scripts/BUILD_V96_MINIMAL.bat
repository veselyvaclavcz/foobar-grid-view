@echo off
echo Building Album Art Grid v9.6.0 Minimal...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Compiling minimal v9.6...
cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE ^
    grid_v96_minimal.cpp

echo Linking...
link /DLL /OUT:foo_albumart_grid_v96.dll ^
    /EXPORT:foobar2000_get_interface ^
    grid_v96_minimal.obj ^
    kernel32.lib user32.lib

if exist foo_albumart_grid_v96.dll (
    echo BUILD SUCCESSFUL!
    del *.obj 2>nul
) else (
    echo BUILD FAILED!
)

pause