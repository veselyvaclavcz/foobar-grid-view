@echo off
echo ============================================
echo Album Art Grid v9.9.9 INSTALLER
echo ============================================
echo.
echo This will install Album Art Grid v9.9.9 with shutdown crash fixes
echo.

set COMPONENT=foo_albumart_grid_v999_FINAL.fb2k-component

if not exist "%COMPONENT%" (
    echo ERROR: Component file not found: %COMPONENT%
    echo Please run BUILD_V999_FINAL_CLEAN.bat first
    pause
    exit /b 1
)

echo Component found: %COMPONENT%
echo.
echo IMPORTANT: This version includes critical fixes for:
echo - Shutdown/sleep crashes
echo - Access violations on exit
echo - Callback race conditions
echo.
echo The component will now be installed...
echo.

rem Try to find foobar2000 installation
set FB2K_PATH=
if exist "%ProgramFiles%\foobar2000\foobar2000.exe" (
    set FB2K_PATH=%ProgramFiles%\foobar2000
) else if exist "%ProgramFiles(x86)%\foobar2000\foobar2000.exe" (
    set FB2K_PATH=%ProgramFiles(x86)%\foobar2000
) else if exist "C:\foobar2000\foobar2000.exe" (
    set FB2K_PATH=C:\foobar2000
)

if defined FB2K_PATH (
    echo Found foobar2000 at: %FB2K_PATH%
    echo.
    echo Launching installer...
    start "" "%FB2K_PATH%\foobar2000.exe" "%CD%\%COMPONENT%"
) else (
    echo foobar2000 not found in standard locations
    echo.
    echo Please install manually by:
    echo 1. Double-clicking on %COMPONENT%
    echo 2. Or drag it to foobar2000 window
    echo 3. Or copy to foobar2000\components folder
    echo.
    explorer /select,"%CD%\%COMPONENT%"
)

echo.
echo ============================================
echo AFTER INSTALLATION:
echo ============================================
echo 1. Restart foobar2000
echo 2. Check console for "Album Art Grid v9.9.9 initialized"
echo 3. In Preferences > Components, verify version shows 9.9.9
echo 4. Test shutdown/sleep - crashes should be fixed
echo.
echo If you still experience crashes, please report with:
echo - The crash dump file
echo - Console output before crash
echo - What you were doing when it crashed
echo.
pause