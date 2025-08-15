@echo off
echo ========================================================================
echo Building Minimal Working foobar2000 v2 Component
echo ========================================================================
echo.

cd /d "%~dp0"

rem Setup Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
if errorlevel 1 (
    echo Error: Could not initialize Visual Studio environment
    exit /b 1
)

echo Compiling minimal working component...
cl.exe /LD /O2 /MD /EHsc /DNDEBUG /D_WINDLL /DUNICODE /D_UNICODE ^
    minimal_working_component.cpp ^
    /Fe:foo_minimal_working.dll ^
    /link /SUBSYSTEM:WINDOWS winmm.lib kernel32.lib user32.lib

if exist foo_minimal_working.dll (
    echo.
    echo ========================================================================
    echo BUILD SUCCESSFUL!
    echo ========================================================================
    echo.
    echo Component created: foo_minimal_working.dll
    echo.
    echo This minimal component implements the correct foobar2000_client interface
    echo without depending on SDK headers. It should load without crashes.
    echo.
    echo To install:
    echo 1. Close foobar2000
    echo 2. Copy foo_minimal_working.dll to your foobar2000 components folder
    echo 3. Start foobar2000
    echo 4. Check Preferences -^> Components to verify it loaded
    echo.
) else (
    echo.
    echo Build failed! Check compiler output above.
)

pause