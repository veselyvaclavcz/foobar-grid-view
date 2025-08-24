@echo off
echo ============================================
echo Building Album Art Grid v10.0.5 SHUTDOWN FIX
echo Robust shutdown handling
echo ============================================
echo.

cd /d "%~dp0"

REM Clean previous build
del *.obj 2>nul
del foo_albumart_grid.dll 2>nul
del foo_albumart_grid_v10_0_5_shutdown_fix.fb2k-component 2>nul

REM Setup VS environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

echo.
echo [1/5] Compiling initquit v10.0.5 with shutdown protection...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" /I"SDK-2025-03-07\pfc" ^
   "initquit_v10_0_5.cpp"

if not exist initquit_v10_0_5.obj (
    echo ERROR: Failed to compile initquit_v10_0_5.cpp
    pause
    exit /b 1
)

echo [2/5] Compiling SDK component client...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" /I"SDK-2025-03-07\pfc" ^
   "SDK-2025-03-07\foobar2000\foobar2000_component_client\component_client.cpp"

if not exist component_client.obj (
    echo ERROR: Failed to compile component_client.cpp
    pause
    exit /b 1
)

echo [3/5] Compiling grid v10.0.5 with shutdown fix...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" ^
   /I"SDK-2025-03-07\foobar2000\helpers" /I"SDK-2025-03-07\foobar2000\shared" ^
   /I"SDK-2025-03-07\pfc" /I"SDK-2025-03-07\libPPUI" ^
   "grid_v10_0_5_shutdown_fix.cpp"

if not exist grid_v10_0_5_shutdown_fix.obj (
    echo ERROR: Failed to compile grid_v10_0_5_shutdown_fix.cpp
    echo Check error messages above
    pause
    exit /b 1
)

echo [4/5] Compiling helpers v10.0.5 with shutdown protection...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" ^
   /I"SDK-2025-03-07\foobar2000\helpers" /I"SDK-2025-03-07\foobar2000\shared" ^
   /I"SDK-2025-03-07\pfc" /I"SDK-2025-03-07\libPPUI" ^
   "helpers_minimal_v10_0_5.cpp"

if not exist helpers_minimal_v10_0_5.obj (
    echo ERROR: Failed to compile helpers_minimal_v10_0_5.cpp
    pause
    exit /b 1
)

echo [5/5] Linking DLL with shutdown protection...
link /DLL /OUT:foo_albumart_grid.dll /NOLOGO ^
   /SUBSYSTEM:WINDOWS ^
   /OPT:REF /OPT:ICF ^
   grid_v10_0_5_shutdown_fix.obj initquit_v10_0_5.obj component_client.obj helpers_minimal_v10_0_5.obj ^
   "SDK-2025-03-07\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
   "SDK-2025-03-07\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
   "SDK-2025-03-07\pfc\x64\Release\pfc.lib" ^
   user32.lib gdi32.lib gdiplus.lib shell32.lib ole32.lib oleaut32.lib ^
   advapi32.lib shlwapi.lib comctl32.lib comdlg32.lib uuid.lib kernel32.lib

if not exist foo_albumart_grid.dll (
    echo ERROR: Failed to create foo_albumart_grid.dll
    pause
    exit /b 1
)

echo.
echo Creating component package...
powershell -Command "Compress-Archive -Path 'foo_albumart_grid.dll' -DestinationPath 'temp.zip' -Force; Move-Item 'temp.zip' 'foo_albumart_grid_v10_0_5_shutdown_fix.fb2k-component' -Force"

if exist foo_albumart_grid_v10_0_5_shutdown_fix.fb2k-component (
    echo.
    echo ============================================
    echo BUILD SUCCESSFUL!
    echo Component: foo_albumart_grid_v10_0_5_shutdown_fix.fb2k-component
    echo.
    echo Changes in v10.0.5:
    echo - FIXED: Crash during foobar2000 shutdown
    echo - Added robust shutdown handling to all components
    echo - Protected helpers module against shutdown access
    echo - Thread-safe cleanup sequence
    echo - Critical section protection during shutdown
    echo.
    echo All v10.0.4 features included:
    echo - 4 display label options (Album, Artist, Artist-Album, Folder)
    echo - Fixed version display
    echo - Crash protection during library indexing
    echo ============================================
) else (
    echo ERROR: Failed to create component package
)

pause