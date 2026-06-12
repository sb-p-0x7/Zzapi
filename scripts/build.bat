@echo off
REM =============================================================================
REM PizzaFactory build script (Windows)
REM Usage: scripts\build.bat [Release|Debug]
REM =============================================================================

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug

set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build

echo PizzaFactory build start (%BUILD_TYPE%)
echo ===========================================

cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo CMake configure failed!
    exit /b 1
)

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build succeeded! Run it with:
echo    %BUILD_DIR%\%BUILD_TYPE%\PizzaFactory.exe
