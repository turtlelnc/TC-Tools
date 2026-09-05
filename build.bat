@echo off
rem ============================================================
rem  TC-tools build script (MinGW-w64 g++)
rem  Usage: build.bat            (uses g++ from PATH)
rem         build.bat C:\Qt\Tools\mingw1310_64\bin\g++.exe
rem ============================================================
setlocal
set "GPP=%~1"
if "%GPP%"=="" set "GPP=g++"
set "OUT=dist"
if not exist "%OUT%" mkdir "%OUT%"

echo [1/2] Compiling ...
"%GPP%" -std=c++20 -O2 -Wall -Wextra -static -static-libgcc -static-libstdc++ ^
        -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
        src\main.cpp src\pages.cpp src\tools.cpp src\cli.cpp src\http.cpp src\json.cpp src\util.cpp src\app.cpp src\lang.cpp ^
        -o "%OUT%\tctool.exe" -lwinhttp -lshell32 -luser32 -lversion -ladvapi32
if errorlevel 1 (
  echo Build FAILED.
  exit /b 1
)
echo [2/2] OK: %OUT%\tctool.exe
endlocal