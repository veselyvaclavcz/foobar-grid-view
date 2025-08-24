@echo off
echo ============================================
echo Installing Album Art Grid v10.0.5 SAFE
echo Proper shutdown and restart procedure
echo ============================================
echo.

cd /d "%~dp0"

REM Check if component package exists
if not exist "foo_albumart_grid_v10_0_5_shutdown_fix.fb2k-component" (
    echo ERROR: Component package not found!
    echo Please run BUILD_V10_0_5_SHUTDOWN_FIX.bat first
    pause
    exit /b 1
)

echo [1/4] Checking for running foobar2000...
powershell "Get-Process foobar2000 -ErrorAction SilentlyContinue" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo foobar2000 is running. Closing it safely...
    echo IMPORTANT: This will close foobar2000 to prevent memory corruption!
    pause
    powershell "Stop-Process -Name foobar2000 -Force -ErrorAction SilentlyContinue"
    echo Waiting for safe shutdown...
    timeout /t 3 >nul
) else (
    echo foobar2000 is not running.
)

echo.
echo [2/4] Extracting v10.0.5 component...
copy "foo_albumart_grid_v10_0_5_shutdown_fix.fb2k-component" "temp_install.zip" >nul
powershell "Expand-Archive -Path 'temp_install.zip' -DestinationPath 'temp_install' -Force" 2>nul

if not exist "temp_install\foo_albumart_grid.dll" (
    echo ERROR: Failed to extract component DLL
    del temp_install.zip 2>nul
    pause
    exit /b 1
)

echo [3/4] Installing v10.0.5 to foobar2000...
REM Ensure target directory exists
mkdir "C:\Users\mail\AppData\Roaming\foobar2000-v2\user-components-x64\foo_albumart_grid" 2>nul

REM Backup old version
if exist "C:\Users\mail\AppData\Roaming\foobar2000-v2\user-components-x64\foo_albumart_grid\foo_albumart_grid.dll" (
    copy "C:\Users\mail\AppData\Roaming\foobar2000-v2\user-components-x64\foo_albumart_grid\foo_albumart_grid.dll" "foo_albumart_grid_backup.dll" >nul 2>&1
    echo Previous version backed up as foo_albumart_grid_backup.dll
)

REM Install new DLL
copy "temp_install\foo_albumart_grid.dll" "C:\Users\mail\AppData\Roaming\foobar2000-v2\user-components-x64\foo_albumart_grid\foo_albumart_grid.dll"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to install new DLL
    rmdir /S /Q temp_install 2>nul
    del temp_install.zip 2>nul
    pause
    exit /b 1
)

echo [4/4] Starting foobar2000 with v10.0.5...
REM Clean up
rmdir /S /Q temp_install 2>nul
del temp_install.zip 2>nul

echo Starting foobar2000...
start "" "C:\Program Files\foobar2000\foobar2000.exe"

echo.
echo ============================================
echo INSTALLATION COMPLETE!
echo Album Art Grid v10.0.5 with shutdown fix
echo.
echo Changes in v10.0.5:
echo - FIXED: Crash during foobar2000 shutdown
echo - Added robust shutdown handling
echo - Protected helpers module against shutdown access
echo - Thread-safe cleanup sequence
echo.
echo IMPORTANT: Check foobar2000 console to verify:
echo "Album Art Grid v10.0.5 initialized"
echo "Robust shutdown handling enabled"
echo.
echo If you see v10.0.4 in the console, restart foobar2000!
echo ============================================

pause