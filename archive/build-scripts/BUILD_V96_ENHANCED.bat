@echo off
echo Building Album Art Grid Component v9.6 (Enhanced)...
echo ================================================

REM Clean previous builds
if exist foo_albumart_grid_v96.dll del foo_albumart_grid_v96.dll
if exist foo_albumart_grid_v96.exp del foo_albumart_grid_v96.exp
if exist foo_albumart_grid_v96.lib del foo_albumart_grid_v96.lib

REM Set paths
set SDK_PATH=SDK-2025-03-07
set SHARED_LIB=shared-x64.lib

REM Build SDK libraries if needed
if not exist "%SDK_PATH%\foobar2000\SDK\Release\foobar2000_SDK.lib" (
    echo Building foobar2000 SDK...
    msbuild "%SDK_PATH%\foobar2000\SDK\foobar2000_SDK.vcxproj" /p:Configuration=Release /p:Platform=x64
)

if not exist "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" (
    echo Building foobar2000 component client...
    msbuild "%SDK_PATH%\foobar2000\foobar2000_component_client\foobar2000_component_client.vcxproj" /p:Configuration=Release /p:Platform=x64
)

if not exist "%SDK_PATH%\pfc\x64\Release\pfc.lib" (
    echo Building PFC...
    msbuild "%SDK_PATH%\pfc\pfc.vcxproj" /p:Configuration=Release /p:Platform=x64
)

REM Build the component
echo.
echo Compiling grid_v96_enhanced.cpp...
cl /c /O2 /GL /MD /EHsc /std:c++17 /W3 ^
   /DNDEBUG /DWIN32 /D_WINDOWS /D_USRDLL /DUNICODE /D_UNICODE ^
   /I"%SDK_PATH%" ^
   grid_v96_enhanced.cpp

if errorlevel 1 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Linking foo_albumart_grid_v96.dll...
link /DLL /LTCG /OPT:REF /OPT:ICF /MACHINE:X64 ^
     /OUT:foo_albumart_grid_v96.dll ^
     /DEF /EXPORT:foobar2000_get_interface ^
     grid_v96_enhanced.obj ^
     "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
     "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
     "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
     "%SHARED_LIB%" ^
     kernel32.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib shell32.lib ole32.lib ^
     shlwapi.lib gdiplus.lib msimg32.lib

if errorlevel 1 (
    echo Linking failed!
    pause
    exit /b 1
)

REM Clean up object files
del grid_v96_enhanced.obj

echo.
echo ========================================
echo Build completed successfully!
echo Output: foo_albumart_grid_v96.dll
echo.
echo Version 9.6 Features:
echo - Track count badge when labels hidden
echo - Custom themed scrollbar
echo - Artist image support
echo ========================================
pause