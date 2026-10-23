@echo off
cd /d "%~dp0dist\NeonCore"
if not exist NeonCore.exe (
    echo [ERROR] NeonCore.exe not found. Run build.bat first.
    pause
    exit /b 1
)
start "" NeonCore.exe %*
