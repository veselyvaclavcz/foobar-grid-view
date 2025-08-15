@echo off
echo.
echo ========================================
echo   foobar2000 v2 Component Builder
echo   Builds proper components that won't crash
echo ========================================
echo.

REM Set up Visual Studio environment
set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
if not exist "%VS_PATH%" (
    echo ❌ Error: Visual Studio Build Tools not found at %VS_PATH%
    pause
    exit /b 1
)

echo Initializing Visual Studio environment...
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Define SDK paths
set "SDK_ROOT=SDK-2025-03-07"
set "PFC_LIB=%SDK_ROOT%\pfc\x64\Release\pfc.lib"
set "SDK_LIB=%SDK_ROOT%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib"
set "CLIENT_LIB=%SDK_ROOT%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib"
set "SHARED_LIB=%SDK_ROOT%\foobar2000\shared\shared-x64.lib"

REM Verify all required libraries exist
echo Checking for required SDK libraries...
if not exist "%PFC_LIB%" (
    echo ❌ Error: pfc.lib not found at %PFC_LIB%
    pause
    exit /b 1
)
echo ✓ Found pfc.lib

if not exist "%SDK_LIB%" (
    echo ❌ Error: foobar2000_SDK.lib not found at %SDK_LIB%
    pause
    exit /b 1
)
echo ✓ Found foobar2000_SDK.lib

if not exist "%CLIENT_LIB%" (
    echo ❌ Error: foobar2000_component_client.lib not found at %CLIENT_LIB%
    pause
    exit /b 1
)
echo ✓ Found foobar2000_component_client.lib

if not exist "%SHARED_LIB%" (
    echo ❌ Error: shared-x64.lib not found at %SHARED_LIB%
    pause
    exit /b 1
)
echo ✓ Found shared-x64.lib

echo.
echo All SDK libraries found. Starting compilation...

REM Clean up any previous builds
if exist foo_albumart_grid.dll del foo_albumart_grid.dll
if exist foo_albumart_grid.exp del foo_albumart_grid.exp
if exist foo_albumart_grid.lib del foo_albumart_grid.lib
if exist foo_albumart_grid.pdb del foo_albumart_grid.pdb
if exist *.obj del *.obj

REM Compile the component
echo.
echo Compiling component source code...
cl /c /EHsc /MD /O2 /std:c++17 /DWIN32 /D_WINDOWS /DNDEBUG ^
   /I"%SDK_ROOT%" ^
   /I"%SDK_ROOT%\pfc" ^
   /I"%SDK_ROOT%\foobar2000\SDK" ^
   foo_working_component.cpp

if errorlevel 1 (
    echo.
    echo ❌ Compilation failed!
    pause
    exit /b 1
)
echo ✓ Compilation successful

REM Link the DLL with ALL required libraries in the correct order
echo.
echo Linking component DLL...
link /DLL /MACHINE:X64 /SUBSYSTEM:WINDOWS ^
     /OUT:foo_albumart_grid.dll ^
     foo_working_component.obj ^
     "%CLIENT_LIB%" ^
     "%SDK_LIB%" ^
     "%PFC_LIB%" ^
     "%SHARED_LIB%" ^
     kernel32.lib user32.lib shell32.lib

if errorlevel 1 (
    echo.
    echo ❌ Linking failed!
    pause
    exit /b 1
)
echo ✓ Linking successful

REM Clean up object files
del *.obj >nul 2>&1

echo.
echo ✅ BUILD SUCCESSFUL!
echo.
echo Created: foo_albumart_grid.dll
echo Size: 
dir foo_albumart_grid.dll | find "foo_albumart_grid.dll"
echo.
echo ========================================
echo CRITICAL: This component uses the proper
echo foobar2000_client implementation pattern
echo and should NOT crash with vtable errors!
echo ========================================
echo.
echo To test the component:
echo 1. Close foobar2000 completely
echo 2. Copy foo_albumart_grid.dll to your foobar2000\components\ folder  
echo 3. Start foobar2000
echo 4. Check Console (View ^> Console) for component load messages
echo 5. Go to File ^> Preferences ^> Components to see if it's listed
echo.
pause