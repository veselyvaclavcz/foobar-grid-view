@echo off
echo ========================================================================
echo Building Working Album Art Grid Component
echo ========================================================================
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

set SDK_PATH=SDK-2025-03-07

echo Step 1: Building SDK libraries if needed...
if not exist pfc.lib (
    echo Building pfc.lib...
    cl /c /EHsc /MD /O2 /DNDEBUG /DUNICODE /D_UNICODE /I"%SDK_PATH%" /I"%SDK_PATH%\pfc" ^
        "%SDK_PATH%\pfc\string_base.cpp" ^
        "%SDK_PATH%\pfc\string_conv.cpp" ^
        "%SDK_PATH%\pfc\utf8.cpp" ^
        "%SDK_PATH%\pfc\other.cpp" ^
        "%SDK_PATH%\pfc\guid.cpp"
    lib /OUT:pfc.lib string_base.obj string_conv.obj utf8.obj other.obj guid.obj
    del *.obj 2>nul
)

if not exist foobar2000_SDK.lib (
    echo Building foobar2000_SDK.lib...
    cl /c /EHsc /MD /O2 /DNDEBUG /DUNICODE /D_UNICODE /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
        "%SDK_PATH%\foobar2000\SDK\service.cpp" ^
        "%SDK_PATH%\foobar2000\SDK\componentversion.cpp"
    lib /OUT:foobar2000_SDK.lib service.obj componentversion.obj
    del *.obj 2>nul
)

if not exist foobar2000_component_client.lib (
    echo Building foobar2000_component_client.lib...
    cl /c /EHsc /MD /O2 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
        /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
        "%SDK_PATH%\foobar2000\foobar2000_component_client\component_client.cpp"
    lib /OUT:foobar2000_component_client.lib component_client.obj
    del *.obj 2>nul
)

echo Step 2: Creating shared library if needed...
if not exist shared-x64.lib (
    if exist "C:\Program Files\foobar2000\shared.dll" (
        echo Creating shared-x64.lib from shared.dll...
        REM Try to create a minimal import library
        echo LIBRARY shared > shared.def
        echo EXPORTS >> shared.def
        lib /def:shared.def /out:shared-x64.lib /machine:x64 2>nul
        del shared.def shared.exp 2>nul
    )
)

echo Step 3: Compiling component...
cl /c /EHsc /MD /O2 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=80 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    foo_working_component.cpp

echo Step 4: Linking component...
echo Attempting to link with available libraries...
link /DLL /OUT:foo_albumart_grid.dll ^
    foo_working_component.obj ^
    foobar2000_component_client.lib ^
    foobar2000_SDK.lib ^
    pfc.lib ^
    kernel32.lib user32.lib shlwapi.lib ole32.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_albumart_grid.dll (
    echo.
    echo ========================================================================
    echo BUILD SUCCESSFUL!
    echo ========================================================================
    echo Component created: foo_albumart_grid.dll
    del *.obj 2>nul
    del *.exp 2>nul
    del *.lib 2>nul
    echo.
    echo To install:
    echo 1. Close foobar2000
    echo 2. Run install script or copy to components folder
) else (
    echo.
    echo Build completed with possible warnings.
    echo Check if foo_albumart_grid.dll was created.
)

pause