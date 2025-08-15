@echo off
echo Creating Album Art Grid v9.7.0 Installation Package...
cd /d "%~dp0"

if not exist foo_albumart_grid_v97.dll (
    echo ERROR: foo_albumart_grid_v97.dll not found!
    echo Please build the component first.
    pause
    exit /b 1
)

echo.
echo Creating fb2k-component package...

REM Delete old package if exists
if exist foo_albumart_grid_v97.fb2k-component del foo_albumart_grid_v97.fb2k-component
if exist temp_package rmdir /s /q temp_package

REM Create temporary directory structure
mkdir temp_package

REM Copy the DLL to temp directory
copy foo_albumart_grid_v97.dll temp_package\

REM Create the zip file (fb2k-component is just a renamed zip)
cd temp_package
powershell -Command "Compress-Archive -Path * -DestinationPath ../foo_albumart_grid_v97.zip -Force"
cd ..

REM Rename .zip to .fb2k-component
move /y foo_albumart_grid_v97.zip foo_albumart_grid_v97.fb2k-component

REM Clean up
rmdir /s /q temp_package

if exist foo_albumart_grid_v97.fb2k-component (
    echo.
    echo ========================================
    echo PACKAGE CREATED SUCCESSFULLY!
    echo ========================================
    echo.
    echo Package: foo_albumart_grid_v97.fb2k-component
    echo.
    echo Installation instructions:
    echo 1. Open foobar2000
    echo 2. Go to File - Preferences - Components
    echo 3. Click "Install..." button
    echo 4. Select foo_albumart_grid_v97.fb2k-component
    echo    OR drag and drop the file into the Components window
    echo 5. Restart foobar2000 when prompted
    echo.
    echo The component will then appear in:
    echo - View - Layout - Quick Setup
    echo - Or right-click in layout editing mode to add
    echo.
) else (
    echo.
    echo PACKAGE CREATION FAILED!
)

pause