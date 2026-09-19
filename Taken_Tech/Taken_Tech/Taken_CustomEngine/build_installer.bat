@echo off
REM =====================================================
REM  Taken_Tech Build Script - ReleaseInstaller (installer)
REM =====================================================

setlocal ENABLEDELAYEDEXPANSION

set ROOT_DIR=%~dp0
cd /d "%ROOT_DIR%"

call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if not exist out\build-ReleaseInstaller (
    mkdir out\build-ReleaseInstaller
)

cd out\build-ReleaseInstaller

cmake -G "Visual Studio 17 2022" ../..
if errorlevel 1 goto :fail

cmake --build . --config ReleaseInstaller
if errorlevel 1 goto :fail

cd ../..
echo.
echo ==========================================
echo ReleaseInstaller build completed!
echo Output: bin\RELEASEINSTALLER\Taken.exe
echo (Editor is DISABLED - use this for installer)
echo ==========================================
pause
exit /b 0

:fail
echo Build FAILED.
pause
exit /b 1