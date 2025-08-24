@echo off
echo ============================================
echo Building Album Art Grid v10.0.4
echo Menu Fix and Crash Protection
echo ============================================
echo.

cd /d "%~dp0"

REM Clean
del *.obj 2>nul
del foo_albumart_grid.dll 2>nul
del foo_albumart_grid_v10_0_4.fb2k-component 2>nul

REM Setup VS environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

echo.
echo [1/5] Compiling initquit v10.0.4...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" /I"SDK-2025-03-07\pfc" ^
   "initquit_v10_0_4_fix.cpp"

if not exist initquit_v10_0_4_fix.obj (
    echo ERROR: Failed to compile initquit_v10_0_4_fix.cpp
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

echo [3/5] Compiling grid v10.0.4 with menu fix...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" ^
   /I"SDK-2025-03-07\foobar2000\helpers" /I"SDK-2025-03-07\foobar2000\shared" ^
   /I"SDK-2025-03-07\pfc" /I"SDK-2025-03-07\libPPUI" ^
   "grid_v10_0_4_menu_fix.cpp"

if not exist grid_v10_0_4_menu_fix.obj (
    echo ERROR: Failed to compile grid_v10_0_4_menu_fix.cpp
    echo Check error messages above
    pause
    exit /b 1
)

echo [4/5] Checking for helpers file...
if exist helpers_minimal.cpp (
    echo Compiling helpers...
    cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
       /DFOOBAR2000_TARGET_VERSION=80 ^
       /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" ^
       /I"SDK-2025-03-07\foobar2000\helpers" /I"SDK-2025-03-07\foobar2000\shared" ^
       /I"SDK-2025-03-07\pfc" /I"SDK-2025-03-07\libPPUI" ^
       "helpers_minimal.cpp"
)

echo [5/5] Linking DLL...
if exist helpers_minimal.obj (
    link /DLL /OUT:foo_albumart_grid.dll /NOLOGO ^
       /SUBSYSTEM:WINDOWS ^
       /OPT:REF /OPT:ICF ^
       grid_v10_0_4_menu_fix.obj initquit_v10_0_4_fix.obj component_client.obj helpers_minimal.obj ^
       "SDK-2025-03-07\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
       "SDK-2025-03-07\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
       "SDK-2025-03-07\pfc\x64\Release\pfc.lib" ^
       user32.lib gdi32.lib gdiplus.lib shell32.lib ole32.lib oleaut32.lib ^
       advapi32.lib shlwapi.lib comctl32.lib comdlg32.lib uuid.lib kernel32.lib
) else (
    link /DLL /OUT:foo_albumart_grid.dll /NOLOGO ^
       /SUBSYSTEM:WINDOWS ^
       /OPT:REF /OPT:ICF ^
       grid_v10_0_4_menu_fix.obj initquit_v10_0_4_fix.obj component_client.obj ^
       "SDK-2025-03-07\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
       "SDK-2025-03-07\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
       "SDK-2025-03-07\pfc\x64\Release\pfc.lib" ^
       user32.lib gdi32.lib gdiplus.lib shell32.lib ole32.lib oleaut32.lib ^
       advapi32.lib shlwapi.lib comctl32.lib comdlg32.lib uuid.lib kernel32.lib
)

if not exist foo_albumart_grid.dll (
    echo ERROR: Failed to create foo_albumart_grid.dll
    pause
    exit /b 1
)

echo.
echo Creating component package...
powershell -Command "Compress-Archive -Path 'foo_albumart_grid.dll' -DestinationPath 'temp.zip' -Force; Move-Item 'temp.zip' 'foo_albumart_grid_v10_0_4.fb2k-component' -Force"

if exist foo_albumart_grid_v10_0_4.fb2k-component (
    echo.
    echo ============================================
    echo BUILD SUCCESSFUL!
    echo Component: foo_albumart_grid_v10_0_4.fb2k-component
    echo.
    echo Changes in v10.0.4:
    echo - Fixed crash during media library indexing
    echo - Added null handle protection
    echo - Added explicit 'Folder Name' display option
    echo - Menu now has 3 options: Album, Artist-Album, Folder Name
    echo ============================================
) else (
    echo ERROR: Failed to create component package
)

pause