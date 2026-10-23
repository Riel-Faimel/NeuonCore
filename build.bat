@echo off
setlocal
cd /d "%~dp0"

where mingw32-make >nul 2>nul
if errorlevel 1 (
    echo mingw32-make was not found in PATH.
    echo Install MinGW-w64 or run: mingw32-make -j12 deploy
    exit /b 1
)

echo Building release with mingw32-make -j12...
mingw32-make -j12 deploy
exit /b %ERRORLEVEL%
