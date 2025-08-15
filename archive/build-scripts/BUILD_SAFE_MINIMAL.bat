@echo off
echo Building safe minimal foobar2000 v2 component...

set SDK_PATH=SDK-2025-03-07
set COMPONENT_NAME=foo_albumart_grid_safe

:: Clean up previous builds
if exist %COMPONENT_NAME%.dll del %COMPONENT_NAME%.dll
if exist %COMPONENT_NAME%.obj del %COMPONENT_NAME%.obj
if exist %COMPONENT_NAME%.exp del %COMPONENT_NAME%.exp
if exist %COMPONENT_NAME%.lib del %COMPONENT_NAME%.lib

echo Compiling source files...

:: Compile individual SDK components that we need
cl /c /EHsc /MD /O2 ^
   /I"%SDK_PATH%" ^
   /I"%SDK_PATH%\foobar2000" ^
   /I"%SDK_PATH%\foobar2000\SDK" ^
   /I"%SDK_PATH%\foobar2000\helpers" ^
   /I"%SDK_PATH%\pfc" ^
   /D "WIN32" ^
   /D "NDEBUG" ^
   /D "_WINDOWS" ^
   /D "_USRDLL" ^
   /D "_WINDLL" ^
   /D "_UNICODE" ^
   /D "UNICODE" ^
   /Fo:pfc_string_base.obj ^
   %SDK_PATH%\pfc\string_base.cpp

cl /c /EHsc /MD /O2 ^
   /I"%SDK_PATH%" ^
   /I"%SDK_PATH%\foobar2000" ^
   /I"%SDK_PATH%\foobar2000\SDK" ^
   /I"%SDK_PATH%\foobar2000\helpers" ^
   /I"%SDK_PATH%\pfc" ^
   /D "WIN32" ^
   /D "NDEBUG" ^
   /D "_WINDOWS" ^
   /D "_USRDLL" ^
   /D "_WINDLL" ^
   /D "_UNICODE" ^
   /D "UNICODE" ^
   /Fo:pfc_other.obj ^
   %SDK_PATH%\pfc\other.cpp

cl /c /EHsc /MD /O2 ^
   /I"%SDK_PATH%" ^
   /I"%SDK_PATH%\foobar2000" ^
   /I"%SDK_PATH%\foobar2000\SDK" ^
   /I"%SDK_PATH%\foobar2000\helpers" ^
   /I"%SDK_PATH%\pfc" ^
   /D "WIN32" ^
   /D "NDEBUG" ^
   /D "_WINDOWS" ^
   /D "_USRDLL" ^
   /D "_WINDLL" ^
   /D "_UNICODE" ^
   /D "UNICODE" ^
   /Fo:sdk_service.obj ^
   %SDK_PATH%\foobar2000\SDK\service.cpp

:: Now compile our component
cl /c /EHsc /MD /O2 ^
   /I"%SDK_PATH%" ^
   /I"%SDK_PATH%\foobar2000" ^
   /I"%SDK_PATH%\foobar2000\SDK" ^
   /I"%SDK_PATH%\foobar2000\helpers" ^
   /I"%SDK_PATH%\pfc" ^
   /D "WIN32" ^
   /D "NDEBUG" ^
   /D "_WINDOWS" ^
   /D "_USRDLL" ^
   /D "_WINDLL" ^
   /D "_UNICODE" ^
   /D "UNICODE" ^
   /Fo:safe_component.obj ^
   minimal_safe_component.cpp

echo Linking component...

:: Link everything together
link /DLL /OUT:%COMPONENT_NAME%.dll ^
     /SUBSYSTEM:WINDOWS ^
     safe_component.obj pfc_string_base.obj pfc_other.obj sdk_service.obj ^
     kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib ^
     advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib

:: Clean up object files
if exist *.obj del *.obj

if exist %COMPONENT_NAME%.dll (
    echo Build successful! Component created: %COMPONENT_NAME%.dll
    echo.
    echo File size:
    dir %COMPONENT_NAME%.dll | find ".dll"
    echo.
    echo To install, copy %COMPONENT_NAME%.dll to your foobar2000 v2 components directory
) else (
    echo Build failed!
    echo Check the error messages above for details.
)

pause