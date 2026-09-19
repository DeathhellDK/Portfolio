@echo off
REM =====================================================
REM  Taken_Tech Clean Script (Full Version)
REM  Completely resets all build, cache, and IDE artifacts
REM =====================================================

echo ============================================
echo Cleaning build directories and binaries...
echo ============================================

REM --- Remove out folder ---
if exist out (
    echo Removing out folder...
    attrib -r -h -s out /S /D >nul 2>&1
    rmdir /s /q out 2>nul
)

REM --- Remove bin folder ---
if exist bin (
    echo Removing bin folder...
    attrib -r -h -s bin /S /D >nul 2>&1
    rmdir /s /q bin 2>nul
)

REM --- Remove Visual Studio cache folder ---
if exist .vs (
    echo Removing Visual Studio .vs folder...
    attrib -r -h -s .vs /S /D >nul 2>&1
    rmdir /s /q .vs 2>nul
)

REM --- Remove top-level CMake files ---
del /q CMakeCache.txt 2>nul
del /q cmake_install.cmake 2>nul
rmdir /s /q CMakeFiles 2>nul

REM --- Remove stray Visual Studio user/project files ---
del /q *.vcxproj.user 2>nul
del /q *.sln 2>nul

echo ============================================
echo Clean complete! Project reset to fresh state.
echo ============================================
pause