@echo off
setlocal EnableExtensions

REM Builds GflessClientNoIP (DEFINES+=NO_PROXY_MODE) with Qt 5.15.0 MSVC 64-bit.
REM Usage: double-click or run from cmd: compile_noip.bat
REM Optional overrides: set QTDIR / VCVARS before running.

set "ROOT=%~dp0"
set "LAUNCHER=%ROOT%Launcher"
set "BUILD_DIR=%ROOT%build-noip"
set "LOG=%BUILD_DIR%\build.log"

if not defined QTDIR set "QTDIR=C:\Qt\5.15.0\msvc2019_64"
if not defined VCVARS set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%LAUNCHER%\GflessClient.pro" (
    echo [ERROR] Project not found: "%LAUNCHER%\GflessClient.pro"
    goto :fail
)
if not exist "%QTDIR%\bin\qmake.exe" (
    echo [ERROR] qmake not found: "%QTDIR%\bin\qmake.exe"
    echo Set QTDIR to your Qt MSVC kit, e.g. C:\Qt\5.15.0\msvc2019_64
    goto :fail
)
if not exist "%VCVARS%" (
    echo [ERROR] vcvars64.bat not found: "%VCVARS%"
    echo Set VCVARS to your Visual Studio vcvars64.bat path.
    goto :fail
)

echo === MSVC environment ===
call "%VCVARS%"
if errorlevel 1 (
    echo [ERROR] Failed to load Visual Studio build environment.
    goto :fail
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
pushd "%BUILD_DIR%" || goto :fail

echo === qmake (release + NO_PROXY_MODE) ===
"%QTDIR%\bin\qmake.exe" "%LAUNCHER%\GflessClient.pro" -spec win32-msvc "CONFIG+=release" "DEFINES+=NO_PROXY_MODE" > "%LOG%" 2>&1
if errorlevel 1 (
    echo [ERROR] qmake failed. See "%LOG%"
    type "%LOG%"
    popd
    goto :fail
)

echo === nmake release ===
echo.>> "%LOG%"
echo ===== nmake release =====>> "%LOG%"
nmake release >> "%LOG%" 2>&1
if errorlevel 1 (
    echo [ERROR] Build failed. Last lines of log:
    echo ----------------------------------------
    powershell -NoProfile -Command "Get-Content -LiteralPath '%LOG%' -Tail 60"
    echo ----------------------------------------
    echo Full log: "%LOG%"
    popd
    goto :fail
)

set "EXE="
for /f "delims=" %%F in ('dir /s /b "GflessClientNoIP.exe" 2^>nul') do set "EXE=%%F"

if not defined EXE (
    echo [ERROR] Build reported success but GflessClientNoIP.exe was not found.
    echo Check "%LOG%"
    popd
    goto :fail
)

echo.
echo === Done ===
echo EXE: %EXE%
popd
echo Build dir: "%BUILD_DIR%"
echo Log: "%LOG%"
if /I "%~1"=="nopause" exit /b 0
pause
exit /b 0

:fail
echo.
echo Build FAILED.
if /I "%~1"=="nopause" exit /b 1
pause
exit /b 1
