@echo off
rem ============================================================
rem  TCtools-installer build script
rem  Usage: build-installer.bat [path-to-makensis.exe]
rem ============================================================
setlocal
set "MKNSIS=%~1"
if "%MKNSIS%"=="" set "MKNSIS=makensis"
pushd "%~dp0"
if not exist "..\dist" mkdir "..\dist"
echo [Building ...]
"%MKNSIS%" TCtools-installer.nsi
if errorlevel 1 (
  echo Installer build FAILED. Build requires NSIS 3.x (Unicode).
  echo   https://nsis.sourceforge.io/
  exit /b 1
)
popd
echo [OK] ..\dist\TCtools-installer-0.1.0-rc1.exe
endlocal
