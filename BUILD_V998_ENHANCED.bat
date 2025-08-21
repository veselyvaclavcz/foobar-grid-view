@echo off
echo ============================================
echo Building v9.9.8 FINAL with Status Bar Fields
echo ============================================
echo.

cd /d "%~dp0"

set MSVC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207
set WIN_SDK_PATH=C:\Program Files (x86)\Windows Kits\10
set SDK_VERSION=10.0.26100.0

set INCLUDE=%MSVC_PATH%\include;%WIN_SDK_PATH%\Include\%SDK_VERSION%\ucrt;%WIN_SDK_PATH%\Include\%SDK_VERSION%\shared;%WIN_SDK_PATH%\Include\%SDK_VERSION%\um;%WIN_SDK_PATH%\Include\%SDK_VERSION%\winrt
set LIB=%MSVC_PATH%\lib\x64;%WIN_SDK_PATH%\Lib\%SDK_VERSION%\ucrt\x64;%WIN_SDK_PATH%\Lib\%SDK_VERSION%\um\x64

echo [1/7] Cleaning...
del *.obj 2>nul
del foo_albumart_grid.dll 2>nul

echo [2/7] Compiling component client...
"%MSVC_PATH%\bin\Hostx64\x64\cl.exe" /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" /I"SDK-2025-03-07\pfc" ^
   /Fo"component_client.obj" ^
   "SDK-2025-03-07\foobar2000\foobar2000_component_client\component_client.cpp"

echo [3/7] Compiling initquit service...
"%MSVC_PATH%\bin\Hostx64\x64\cl.exe" /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" /I"SDK-2025-03-07\pfc" ^
   /Fo"initquit_v997.obj" ^
   "initquit_v997.cpp"

echo [4/7] Compiling helpers...
"%MSVC_PATH%\bin\Hostx64\x64\cl.exe" /c /O2 /MD /EHsc /DNDEBUG /std:c++17 /nologo ^
   /Fo"helpers_minimal.obj" ^
   "helpers_minimal.cpp"

echo [5/7] Using v9.9.8 FINAL source...
echo Source: grid_v998_enhanced.cpp

echo [6/7] Compiling main grid with titleformat provider...
"%MSVC_PATH%\bin\Hostx64\x64\cl.exe" /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /D_USRDLL /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" ^
   /I"SDK-2025-03-07\foobar2000\helpers" /I"SDK-2025-03-07\foobar2000\shared" ^
   /I"SDK-2025-03-07\pfc" /I"SDK-2025-03-07\libPPUI" ^
   /Fo"grid_v998_enhanced.obj" ^
   "grid_v998_enhanced.cpp"

echo [7/7] Linking DLL...
"%MSVC_PATH%\bin\Hostx64\x64\link.exe" /DLL /OUT:foo_albumart_grid.dll /FORCE:MULTIPLE /nologo ^
   grid_v998_enhanced.obj component_client.obj initquit_v997.obj helpers_minimal.obj ^
   "SDK-2025-03-07\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
   "SDK-2025-03-07\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
   "SDK-2025-03-07\pfc\x64\Release\pfc.lib" ^
   "SDK-2025-03-07\foobar2000\shared\shared-x64.lib" ^
   user32.lib gdi32.lib gdiplus.lib shell32.lib ole32.lib oleaut32.lib ^
   advapi32.lib shlwapi.lib comctl32.lib comdlg32.lib uuid.lib

if exist foo_albumart_grid.dll (
    echo.
    echo Creating component package...
    powershell -Command "Compress-Archive -Path 'foo_albumart_grid.dll' -DestinationPath 'temp.zip' -Force"
    move /Y temp.zip foo_albumart_grid_v998_ENHANCED.fb2k-component >nul
    echo.
    echo ============================================
    echo         BUILD SUCCESSFUL!
    echo ============================================
    echo.
    echo Created: foo_albumart_grid_v998_ENHANCED.fb2k-component
    echo.
) else (
    echo Build failed!
)
pause
