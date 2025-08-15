@echo off
echo Building Theme-Aware Album Art Grid Component...
echo.

REM Clean previous build
if exist foo_albumart_grid_themed.dll del foo_albumart_grid_themed.dll
if exist foo_albumart_grid_themed.exp del foo_albumart_grid_themed.exp
if exist foo_albumart_grid_themed.lib del foo_albumart_grid_themed.lib
if exist album_grid_themed.obj del album_grid_themed.obj

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)

echo Compiling album_grid_themed.cpp...
cl.exe /nologo /O2 /MD /W3 /EHsc /std:c++17 /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /DNOMINMAX ^
    /I"SDK-2025-03-07" ^
    /c album_grid_themed.cpp /Fo:album_grid_themed.obj

if errorlevel 1 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo Linking DLL...
link.exe /nologo /DLL /OUT:foo_albumart_grid_themed.dll ^
    /DEF:NUL ^
    /MACHINE:X64 ^
    album_grid_themed.obj ^
    user32.lib gdi32.lib comctl32.lib uxtheme.lib dwmapi.lib advapi32.lib

if errorlevel 1 (
    echo Linking failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Output: foo_albumart_grid_themed.dll
echo.
echo The component now includes:
echo - Theme-aware scrollbar that respects Windows dark mode
echo - Custom rendered scrollbar with smooth animations
echo - Automatic theme detection and switching
echo - Hover and active states for scrollbar thumb
echo - Styled arrows for scrollbar navigation
echo.
echo To install:
echo 1. Close foobar2000
echo 2. Copy foo_albumart_grid_themed.dll to your foobar2000\components folder
echo 3. Restart foobar2000
echo.
pause