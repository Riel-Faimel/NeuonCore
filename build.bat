@echo off
setlocal
cd /d "%~dp0"

if not defined QTDIR set "QTDIR=D:\Qt\6.10.2\mingw_64"
if not defined MINGWDIR set "MINGWDIR=D:\Qt\Tools\mingw1310_64"

if not exist "%MINGWDIR%\bin\g++.exe" (
    echo [ERROR] MinGW compiler not found at "%MINGWDIR%\bin\g++.exe"
    echo Set MINGWDIR to the Qt MinGW toolchain directory.
    pause
    exit /b 1
)
if not exist "%QTDIR%\bin\Qt6Core.dll" (
    echo [ERROR] Qt not found at "%QTDIR%"
    echo Set QTDIR to the Qt 6 mingw_64 directory.
    pause
    exit /b 1
)

set "PATH=%MINGWDIR%\bin;%QTDIR%\bin;%PATH%"
set "CXX=%MINGWDIR%\bin\g++.exe"

set "INCS=-I"%CD%\src" -I"%QTDIR%\include\QtWidgets" -I"%QTDIR%\include" -I"%QTDIR%\include\QtMultimedia" -I"%QTDIR%\include\QtGui" -I"%QTDIR%\include\QtNetwork" -I"%QTDIR%\include\QtCore" -I"%QTDIR%\mkspecs\win32-g++""
set "DEFS=-DQT_WIDGETS_LIB -DQT_MULTIMEDIA_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_NETWORK_LIB -DWIN32 -DWIN64 -D_WIN64 -DMINGW_HAS_SECURE_API=1 -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DNDEBUG"
set "LIBS=-L"%QTDIR%\lib" -lQt6Widgets -lQt6Multimedia -lQt6Gui -lQt6Network -lQt6Core -lole32 -loleaut32 -luuid -lws2_32 -ladvapi32 -lshell32 -luser32 -lkernel32 -lgdi32 -lwinmm -limm32 -lversion -ldwmapi -luxtheme -lmsimg32 -static-libgcc -static-libstdc++"
set "CXXFLAGS=-std=c++20 -O2 -fexceptions -Wno-narrowing"

if not exist build mkdir build
if not exist dist\NeonCore mkdir dist\NeonCore

echo [1/2] Compiling modules...
"%CXX%" %CXXFLAGS% %INCS% %DEFS% -c src\base\Base.cpp -o build\Base.o || goto :error
for %%C in (Arena Player Enemy Bullet Portal Pickup Particle FloatingText Shockwave Collision Kill BossState Score) do (
    echo   - Arena\%%C.cpp
    "%CXX%" %CXXFLAGS% %INCS% %DEFS% -c src\Arena\%%C.cpp -o build\Arena_%%C.o || goto :error
)
for %%M in (Math Random Waves Upgrades Renderer Game main) do (
    echo   - %%M.cpp
    "%CXX%" %CXXFLAGS% %INCS% %DEFS% -c src\%%M.cpp -o build\%%M.o || goto :error
)

set "OBJS=build\Base.o build\Math.o build\Random.o build\Arena_Arena.o build\Arena_Player.o build\Arena_Enemy.o build\Arena_Bullet.o build\Arena_Portal.o build\Arena_Pickup.o build\Arena_Particle.o build\Arena_FloatingText.o build\Arena_Shockwave.o build\Arena_Collision.o build\Arena_Kill.o build\Arena_BossState.o build\Arena_Score.o build\Waves.o build\Upgrades.o build\Renderer.o build\Game.o build\main.o"
echo [2/2] Linking...
"%CXX%" %OBJS% -o build\NeonCoreConsole.exe %LIBS% || goto :error
"%CXX%" -mwindows %OBJS% -o build\NeonCore.exe %LIBS% || goto :error

copy /Y build\NeonCore.exe dist\NeonCore\NeonCore.exe >nul
copy /Y build\NeonCoreConsole.exe dist\NeonCore\NeonCoreConsole.exe >nul
copy /Y THIRD_PARTY.txt dist\NeonCore\THIRD_PARTY.txt >nul
echo Deploying Qt runtime...
if not exist dist\NeonCore\platforms mkdir dist\NeonCore\platforms
if not exist dist\NeonCore\multimedia mkdir dist\NeonCore\multimedia
if not exist dist\NeonCore\styles mkdir dist\NeonCore\styles

for %%F in (
    Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Multimedia.dll Qt6Network.dll
    libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll
    avcodec-61.dll avformat-61.dll avutil-59.dll swresample-5.dll swscale-8.dll
) do copy /Y "%QTDIR%\bin\%%F" dist\NeonCore\%%F >nul

copy /Y "%QTDIR%\plugins\platforms\qwindows.dll" dist\NeonCore\platforms\ >nul
copy /Y "%QTDIR%\plugins\platforms\qoffscreen.dll" dist\NeonCore\platforms\ >nul
copy /Y "%QTDIR%\plugins\multimedia\ffmpegmediaplugin.dll" dist\NeonCore\multimedia\ >nul
copy /Y "%QTDIR%\plugins\multimedia\windowsmediaplugin.dll" dist\NeonCore\multimedia\ >nul
copy /Y "%QTDIR%\plugins\styles\qmodernwindowsstyle.dll" dist\NeonCore\styles\ >nul

echo.
echo Build complete:
echo   GUI:     build\NeonCore.exe
echo   Console: build\NeonCoreConsole.exe  ^(for --smoke-test^)
echo   Deploy:  dist\NeonCore\NeonCore.exe
echo.
echo Run dist\NeonCore\NeonCore.exe or run.bat.
exit /b 0

:error
echo.
echo [ERROR] Build failed.
pause
exit /b 1
