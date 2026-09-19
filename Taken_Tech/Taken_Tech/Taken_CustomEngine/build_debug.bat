@echo off
REM =====================================================
REM  Taken_Tech Build Script - Debug (editor ON)
REM =====================================================

setlocal ENABLEDELAYEDEXPANSION

set ROOT_DIR=%~dp0
cd /d "%ROOT_DIR%"

REM Load VS environment
call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Create build directory
if not exist out\build-Debug (
    mkdir out\build-Debug
)

cd out\build-Debug

REM Configure
cmake -G "Visual Studio 17 2022" ../..
if errorlevel 1 goto :fail

REM Build Debug
cmake --build . --config Debug
if errorlevel 1 goto :fail

cd ../..
echo.
echo ================================
echo Debug build completed successfully!
echo Output: bin\DEBUG\Taken.exe
echo ================================
pause
exit /b 0

:fail
echo Build FAILED.
pause
exit /b 1