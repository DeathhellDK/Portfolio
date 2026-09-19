@echo off
REM =====================================================
REM  Taken_Tech Run Script - Release (dev, editor ON)
REM =====================================================

setlocal

REM Repo root
set PROJECT_DIR=%~dp0

REM Config / paths
set CONFIG=RELEASE
set BIN_DIR=%PROJECT_DIR%bin\%CONFIG%
set TAKEN_EXE=%BIN_DIR%\Taken.exe
set SHADER_SRC=%PROJECT_DIR%Game\shaders
set SHADER_DST=%BIN_DIR%\shaders
set ASSET_SRC=%PROJECT_DIR%Game\Assets
set ASSET_DST=%BIN_DIR%\assets

echo ============================================
echo Copying Shaders and Assets to %CONFIG%...
echo ============================================
xcopy "%SHADER_SRC%" "%SHADER_DST%" /E /I /Y >nul
xcopy "%ASSET_SRC%" "%ASSET_DST%" /E /I /Y >nul

echo ============================================
echo Setting working directory and launching game...
echo ============================================
cd /d "%BIN_DIR%"

if exist "Taken.exe" (
    echo Running "%TAKEN_EXE%" ...
    echo.
    Taken.exe
    echo.
    echo Game exited with code %ERRORLEVEL%.
) else (
    echo ERROR: Taken.exe not found in %BIN_DIR%
    echo Did you run build_release.bat?
)

echo ============================================
echo Game closed. Press any key to exit.
echo ============================================
pause

endlocal