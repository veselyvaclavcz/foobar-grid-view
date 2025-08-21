@echo off
echo ============================================
echo Building Album Art Grid v9.9.9 FINAL
echo WITH SHUTDOWN CRASH PROTECTION
echo ============================================
echo.

cd /d "%~dp0"

set MSVC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207
set WIN_SDK_PATH=C:\Program Files (x86)\Windows Kits\10
set SDK_VERSION=10.0.26100.0

set INCLUDE=%MSVC_PATH%\include;%WIN_SDK_PATH%\Include\%SDK_VERSION%\ucrt;%WIN_SDK_PATH%\Include\%SDK_VERSION%\shared;%WIN_SDK_PATH%\Include\%SDK_VERSION%\um;%WIN_SDK_PATH%\Include\%SDK_VERSION%\winrt
set LIB=%MSVC_PATH%\lib\x64;%WIN_SDK_PATH%\Lib\%SDK_VERSION%\ucrt\x64;%WIN_SDK_PATH%\Lib\%SDK_VERSION%\um\x64

echo [1/7] Cleaning old builds...
del *.obj 2>nul
del foo_albumart_grid.dll 2>nul
del foo_albumart_grid_v999*.fb2k-component 2>nul

echo [2/7] Compiling SDK component client...
"%MSVC_PATH%\bin\Hostx64\x64\cl.exe" /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" /I"SDK-2025-03-07\pfc" ^
   /Fo"component_client.obj" ^
   "SDK-2025-03-07\foobar2000\foobar2000_component_client\component_client.cpp"

echo [3/7] Compiling v9.9.9 initquit service...
"%MSVC_PATH%\bin\Hostx64\x64\cl.exe" /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" /I"SDK-2025-03-07\pfc" ^
   /Fo"initquit_v999.obj" ^
   "initquit_v999.cpp"

echo [4/7] Compiling helper functions...
"%MSVC_PATH%\bin\Hostx64\x64\cl.exe" /c /O2 /MD /EHsc /DNDEBUG /std:c++17 /nologo ^
   /Fo"helpers_minimal.obj" ^
   "helpers_minimal.cpp"

echo [5/7] Using v9.9.9 FINAL source...
echo Source: grid_v999_final.cpp

echo [6/7] Compiling main component with all features...
"%MSVC_PATH%\bin\Hostx64\x64\cl.exe" /c /O2 /MD /EHsc /DNDEBUG /DUNICODE /D_UNICODE /D_USRDLL /std:c++17 /nologo ^
   /DFOOBAR2000_TARGET_VERSION=80 ^
   /I"SDK-2025-03-07" /I"SDK-2025-03-07\foobar2000" /I"SDK-2025-03-07\foobar2000\SDK" ^
   /I"SDK-2025-03-07\foobar2000\helpers" /I"SDK-2025-03-07\foobar2000\shared" ^
   /I"SDK-2025-03-07\pfc" /I"SDK-2025-03-07\libPPUI" ^
   /Fo"grid_v999_final.obj" ^
   "grid_v999_final.cpp"

echo [7/7] Linking final DLL...
"%MSVC_PATH%\bin\Hostx64\x64\link.exe" /DLL /OUT:foo_albumart_grid.dll /FORCE:MULTIPLE /nologo ^
   grid_v999_final.obj component_client.obj initquit_v999.obj helpers_minimal.obj ^
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
    move /Y temp.zip foo_albumart_grid_v999_FINAL.fb2k-component >nul
    
    echo.
    echo ============================================
    echo       BUILD SUCCESSFUL! Version 9.9.9
    echo ============================================
    echo.
    echo Component: foo_albumart_grid_v999_FINAL.fb2k-component
    echo.
    echo WHAT'S NEW IN v9.9.9:
    echo - CRITICAL: Fixed shutdown/sleep crashes
    echo - CRITICAL: Fixed access violations on exit
    echo - Improved memory management during shutdown
    echo - Protected callback unregistration
    echo.
    echo INCLUDES ALL FEATURES:
    echo - Status bar integration (album count, view mode)
    echo - Now Playing indicator with Ctrl+Q jump
    echo - Enhanced search with Ctrl+Shift+S
    echo - 1-20 column support
    echo - Artist-Album label format
    echo - Multi-disc album support
    echo.
) else (
    echo.
    echo ============================================
    echo             BUILD FAILED!
    echo ============================================
    echo Check errors above.
)

pause