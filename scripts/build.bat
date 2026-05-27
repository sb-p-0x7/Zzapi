@echo off
REM =============================================================================
REM PizzaFactory 빌드 스크립트 (Windows)
REM 사용법: scripts\build.bat [Release|Debug]
REM =============================================================================

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug

set PROJECT_DIR=%~dp0..
set BUILD_DIR=%PROJECT_DIR%\build

echo 🍕 PizzaFactory 빌드 시작 (%BUILD_TYPE%)
echo ===========================================

cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo ❌ CMake 구성 실패!
    exit /b 1
)

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel
if errorlevel 1 (
    echo ❌ 빌드 실패!
    exit /b 1
)

echo.
echo 🎉 빌드 성공! 실행하려면:
echo    %BUILD_DIR%\%BUILD_TYPE%\PizzaFactory.exe
