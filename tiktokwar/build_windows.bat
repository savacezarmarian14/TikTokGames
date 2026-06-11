@echo off
setlocal enabledelayedexpansion

set "BUILD_DIR=build-windows"
set "BUILD_CONFIG=Release"
set "RUN_AFTER_BUILD=0"
set "RUN_TOWER_COUNT="
set "SFML_DIR_ARG="
set "CMAKE_GENERATOR="

:parse_args
if "%~1"=="" goto after_args
if /I "%~1"=="--debug" (
    set "BUILD_CONFIG=Debug"
    shift
    goto parse_args
)
if /I "%~1"=="--release" (
    set "BUILD_CONFIG=Release"
    shift
    goto parse_args
)
if /I "%~1"=="--clean" (
    if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%"
    shift
    goto parse_args
)
if /I "%~1"=="--run" (
    set "RUN_AFTER_BUILD=1"
    set "RUN_TOWER_COUNT=%~2"
    if "%RUN_TOWER_COUNT%"=="" goto usage
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--sfml-dir" (
    set "SFML_DIR_ARG=%~2"
    if "%SFML_DIR_ARG%"=="" goto usage
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--generator" (
    set "CMAKE_GENERATOR=%~2"
    if "%CMAKE_GENERATOR%"=="" goto usage
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--help" goto usage
if /I "%~1"=="-h" goto usage
echo Unknown option: %~1
goto usage

:after_args
if "%CMAKE_GENERATOR%"=="" (
    if "%SFML_DIR_ARG%"=="" (
        cmake -S . -B "%BUILD_DIR%"
    ) else (
        cmake -S . -B "%BUILD_DIR%" -DSFML_DIR="%SFML_DIR_ARG%"
    )
) else (
    if "%SFML_DIR_ARG%"=="" (
        cmake -S . -B "%BUILD_DIR%" -G "%CMAKE_GENERATOR%"
    ) else (
        cmake -S . -B "%BUILD_DIR%" -G "%CMAKE_GENERATOR%" -DSFML_DIR="%SFML_DIR_ARG%"
    )
)
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config "%BUILD_CONFIG%"
if errorlevel 1 exit /b 1

set "EXE_DIR=%BUILD_DIR%\%BUILD_CONFIG%"
if not exist "%EXE_DIR%\tiktok_war.exe" set "EXE_DIR=%BUILD_DIR%"

call :copy_sfml_dlls "%EXE_DIR%"
if errorlevel 1 exit /b 1

if "%RUN_AFTER_BUILD%"=="1" (
    pushd "%EXE_DIR%"
    tiktok_war.exe "%RUN_TOWER_COUNT%"
    popd
)

echo.
echo Build complete: %EXE_DIR%\tiktok_war.exe
exit /b 0

:copy_sfml_dlls
set "TARGET_EXE_DIR=%~1"
set "SFML_BIN_DIR="

if not "%SFML_ROOT%"=="" if exist "%SFML_ROOT%\bin" set "SFML_BIN_DIR=%SFML_ROOT%\bin"

if "%SFML_BIN_DIR%"=="" if not "%SFML_DIR_ARG%"=="" (
    for %%I in ("%SFML_DIR_ARG%\..\..\..") do set "CANDIDATE_SFML_ROOT=%%~fI"
    if exist "!CANDIDATE_SFML_ROOT!\bin" set "SFML_BIN_DIR=!CANDIDATE_SFML_ROOT!\bin"
)

if "%SFML_BIN_DIR%"=="" (
    echo SFML DLL copy skipped. Set SFML_ROOT=C:\path\to\SFML or pass --sfml-dir C:\path\to\SFML\lib\cmake\SFML.
    exit /b 0
)

copy /Y "%SFML_BIN_DIR%\sfml-*.dll" "%TARGET_EXE_DIR%" >nul
if exist "%SFML_BIN_DIR%\openal32.dll" copy /Y "%SFML_BIN_DIR%\openal32.dll" "%TARGET_EXE_DIR%" >nul
echo Copied SFML runtime DLLs from %SFML_BIN_DIR%
exit /b 0

:usage
echo Usage: build_windows.bat [--release] [--debug] [--clean] [--run N] [--sfml-dir PATH] [--generator NAME]
echo.
echo Examples:
echo   build_windows.bat --sfml-dir C:\SFML-2.6.1\lib\cmake\SFML
echo   build_windows.bat --debug --run 3 --sfml-dir C:\SFML-2.6.1\lib\cmake\SFML
echo.
echo You can also set SFML_ROOT=C:\SFML-2.6.1 so the script can copy DLLs from %%SFML_ROOT%%\bin.
exit /b 1
