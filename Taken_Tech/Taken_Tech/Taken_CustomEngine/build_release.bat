@echo off
REM =====================================================
REM  Taken_Tech Build Script - Release (dev, editor ON)
REM =====================================================

setlocal ENABLEDELAYEDEXPANSION

set ROOT_DIR=%~dp0
cd /d "%ROOT_DIR%"

call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if not exist out\build-Release (
    mkdir out\build-Release
)

cd out\build-Release

cmake -G "Visual Studio 17 2022" ../..
if errorlevel 1 goto :fail

cmake --build . --config Release
if errorlevel 1 goto :fail

cd ../..
echo.
echo ================================
echo Release (dev) build completed!
echo Output: bin\RELEASE\Taken.exe
echo (Editor is ENABLED)
echo ================================
pause
exit /b 0

:fail
echo Build FAILED.
pause
exit /b 1