@echo off
echo ============================================
echo Album Art Grid v9.9.0 - Final Fixed Build
echo ============================================
echo.

cd /d "%~dp0"

REM Initialize Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)

echo [1/6] Cleaning...
del *.obj 2>nul

echo [2/6] Compiling component client...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /D_WINDOWS /std:c++17 ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" ^
   /I"SDK-2025-03-07\foobar2000" ^
   /I"SDK-2025-03-07\foobar2000\SDK" ^
   /I"SDK-2025-03-07\pfc" ^
   /Fo"component_client.obj" ^
   "SDK-2025-03-07\foobar2000\foobar2000_component_client\component_client.cpp"

echo [3/6] Compiling initquit service...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /D_WINDOWS /std:c++17 ^
   /I"SDK-2025-03-07" ^
   /I"SDK-2025-03-07\foobar2000" ^
   /I"SDK-2025-03-07\foobar2000\SDK" ^
   /I"SDK-2025-03-07\pfc" ^
   /Fo"initquit_service.obj" ^
   "initquit_service.cpp"

echo [4/6] Compiling helpers...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /D_WINDOWS /std:c++17 ^
   /Fo"helpers_minimal.obj" ^
   "helpers_minimal.cpp"

echo [5/6] Compiling main component...
cl /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /D_WINDOWS /D_USRDLL /std:c++17 ^
   /I"SDK-2025-03-07" ^
   /I"SDK-2025-03-07\foobar2000" ^
   /I"SDK-2025-03-07\foobar2000\SDK" ^
   /I"SDK-2025-03-07\foobar2000\helpers" ^
   /I"SDK-2025-03-07\foobar2000\shared" ^
   /I"SDK-2025-03-07\pfc" ^
   /I"SDK-2025-03-07\libPPUI" ^
   /Fo"foo_albumart_grid_v99.obj" ^
   "grid_v99_minimal.cpp"

echo [6/6] Linking...
link /DLL /OUT:"foo_albumart_grid_v99.dll" ^
     /SUBSYSTEM:WINDOWS ^
     /MACHINE:X64 ^
     /OPT:REF /OPT:ICF ^
     /LTCG ^
     "foo_albumart_grid_v99.obj" ^
     "component_client.obj" ^
     "initquit_service.obj" ^
     "helpers_minimal.obj" ^
     "SDK-2025-03-07\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
     "SDK-2025-03-07\pfc\x64\Release\pfc.lib" ^
     "SDK-2025-03-07\foobar2000\shared\x64\Release\shared.lib" ^
     kernel32.lib user32.lib gdi32.lib comctl32.lib uxtheme.lib ^
     shlwapi.lib gdiplus.lib shell32.lib ole32.lib oleaut32.lib ^
     advapi32.lib uuid.lib dwmapi.lib

if exist "foo_albumart_grid_v99.dll" (
    echo.
    echo Creating component package...
    
    if exist "component_package" rd /s /q "component_package"
    mkdir "component_package"
    copy "foo_albumart_grid_v99.dll" "component_package\foo_albumart_grid.dll" >nul
    
    powershell -Command "Compress-Archive -Path 'component_package\*' -DestinationPath 'foo_albumart_grid_v99.zip' -Force"
    move /y "foo_albumart_grid_v99.zip" "foo_albumart_grid_v99.fb2k-component" >nul
    rd /s /q "component_package"
    
    echo.
    echo ============================================
    echo SUCCESS! Component built: foo_albumart_grid_v99.fb2k-component
    echo ============================================
) else (
    echo Build failed!
)

pause