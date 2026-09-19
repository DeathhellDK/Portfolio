@echo off
REM =====================================================
REM  Taken_Tech Build Script
REM  Regenerates CMake project files and builds:
REM    - Debug            (editor ON)
REM    - Release          (dev release, editor ON)
REM    - ReleaseInstaller (installer build, editor OFF)
REM =====================================================

setlocal ENABLEDELAYEDEXPANSION

REM --- Root directory (repo root) ---
set ROOT_DIR=%~dp0
cd /d "%ROOT_DIR%"

REM --- Load Visual Studio 2022 Developer Environment ---
REM (Change "Community" to "Professional" or "Enterprise" if needed)
call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
    echo Failed to initialize Visual Studio 2022 environment.
    pause
    exit /b 1
)

REM =====================================================
REM  Configure CMake once (multi-config generator)
REM =====================================================
echo.
echo ============================================
echo Configuring CMake project (out/build/x64)...
echo ============================================
cmake -S . -B out/build/x64 -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo CMake configuration FAILED.
    pause
    exit /b 1
)

REM =====================================================
REM  Build: Debug (editor ON)
REM =====================================================
echo.
echo ============================================
echo Building Debug (editor ON)...
echo ============================================
cmake --build out/build/x64 --config Debug
if errorlevel 1 (
    echo Build FAILED for Debug.
    pause
    exit /b 1
)

REM =====================================================
REM  Build: Release (dev, editor ON)
REM =====================================================
echo.
echo ============================================
echo Building Release (dev, editor ON)...
echo ============================================
cmake --build out/build/x64 --config Release
if errorlevel 1 (
    echo Build FAILED for Release.
    pause
    exit /b 1
)

REM =====================================================
REM  Build: ReleaseInstaller (installer, editor OFF)
REM =====================================================
echo.
echo ============================================
echo Building ReleaseInstaller (installer, editor OFF)...
echo ============================================
cmake --build out/build/x64 --config ReleaseInstaller
if errorlevel 1 (
    echo Build FAILED for ReleaseInstaller.
    pause
    exit /b 1
)

REM =====================================================
REM  Summary (matches your CMake output dirs)
REM =====================================================
echo.
echo =====================================================
echo All builds completed successfully!
echo.
echo Debug build (editor ON):
echo   bin\DEBUG\Taken.exe
echo.
echo Release build (dev, editor ON):
echo   bin\RELEASE\Taken.exe
echo.
echo ReleaseInstaller build (installer, editor OFF):
echo   bin\RELEASEINSTALLER\Taken.exe
echo   Use this exe + its assets/ and shaders/ folders for the installer.
echo =====================================================
pause

endlocal