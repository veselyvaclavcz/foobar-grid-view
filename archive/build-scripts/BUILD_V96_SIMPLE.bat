@echo off
echo Building Album Art Grid v9.6.0 - Track Count Badge Edition...
echo ********************************************************

REM Clean old builds
if exist foo_albumart_grid_v96.dll del foo_albumart_grid_v96.dll
if exist foo_albumart_grid_v96.exp del foo_albumart_grid_v96.exp
if exist foo_albumart_grid_v96.lib del foo_albumart_grid_v96.lib
if exist grid_v96_from_v95.obj del grid_v96_from_v95.obj

REM Initialize VS environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

echo.
echo Compiling v9.6.0 with Track Count Badge feature...
cl /c /O2 /GL /MD /EHsc /std:c++17 /W3 ^
   /DFOOBAR2000_TARGET_VERSION=82 ^
   /DNDEBUG /DWIN32 /D_WINDOWS /D_USRDLL /DUNICODE /D_UNICODE ^
   /I"SDK-2025-03-07" ^
   grid_v96_from_v95.cpp

if errorlevel 1 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Linking...
link /DLL /LTCG /OPT:REF /OPT:ICF /MACHINE:X64 ^
     /OUT:foo_albumart_grid_v96.dll ^
     /EXPORT:foobar2000_get_interface ^
     grid_v96_from_v95.obj ^
     "SDK-2025-03-07\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
     "SDK-2025-03-07\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
     "SDK-2025-03-07\pfc\x64\Release\pfc.lib" ^
     shared-x64.lib ^
     kernel32.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib shell32.lib ole32.lib ^
     shlwapi.lib gdiplus.lib

if errorlevel 1 (
    echo Linking failed!
    pause
    exit /b 1
)

REM Clean up
del grid_v96_from_v95.obj 2>nul

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo Component: foo_albumart_grid_v96.dll v9.6.0
echo.
echo NEW IN v9.6:
echo - Track count badge on album art (when labels hidden)
echo - All features from v9.5 (artist images, etc.)
echo.
echo To install:
echo 1. Close foobar2000
echo 2. Copy foo_albumart_grid_v96.dll to components folder
echo 3. Restart foobar2000
echo ========================================
pause