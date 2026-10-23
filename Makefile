# Neon Core C++ / Qt 6
# Usage:
#   mingw32-make -j12          release GUI (no test code)
#   mingw32-make -j12 test     debug/test console
#   mingw32-make -j12 deploy   release GUI + Qt runtime
#   mingw32-make clean
#
# QTDIR / MINGWDIR may be overridden in the environment.

QTDIR ?= D:/Qt/6.10.2/mingw_64
MINGWDIR ?= D:/Qt/Tools/mingw1310_64
CXX := $(MINGWDIR)/bin/g++.exe

BUILD := build
DIST := dist/NeonCore
BUILD_WIN := $(subst /,\,$(BUILD))
DIST_WIN := $(subst /,\,$(DIST))
QTDIR_WIN := $(subst /,\,$(QTDIR))

CXXFLAGS ?= -std=c++20 -O2 -fexceptions -Wno-narrowing -MMD -MP
INCS := -I$(QTDIR)/include/QtWidgets \
        -I$(QTDIR)/include \
        -I$(QTDIR)/include/QtMultimedia \
        -I$(QTDIR)/include/QtGui \
        -I$(QTDIR)/include/QtNetwork \
        -I$(QTDIR)/include/QtCore \
        -I$(QTDIR)/mkspecs/win32-g++ \
        -Isrc/app -Isrc/include -Isrc/lib
DEFS := -DQT_WIDGETS_LIB -DQT_MULTIMEDIA_LIB -DQT_GUI_LIB \
        -DQT_CORE_LIB -DQT_NETWORK_LIB -DWIN32 -DWIN64 -D_WIN64 \
        -DMINGW_HAS_SECURE_API=1 -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DNDEBUG
LIBS := -L$(QTDIR)/lib \
        -lQt6Widgets -lQt6Multimedia -lQt6Gui -lQt6Network -lQt6Core \
        -lole32 -loleaut32 -luuid -lws2_32 -ladvapi32 -lshell32 \
        -luser32 -lkernel32 -lgdi32 -lwinmm -limm32 -lversion \
        -ldwmapi -luxtheme -lmsimg32 -static-libgcc -static-libstdc++

APP_SRC := Renderer
APP_OBJS := $(addprefix $(BUILD)/app/,$(addsuffix .o,$(APP_SRC)))
GAMES_COMPONENTS := Games Waves/Waves Upgrades/Upgrades
GAMES_OBJS := $(addprefix $(BUILD)/app/Games/,$(addsuffix .o,$(GAMES_COMPONENTS)))
MENU_OBJ := $(BUILD)/app/Menu/Menu.o
LIB_SRC := Math Random
LIB_OBJS := $(addprefix $(BUILD)/lib/,$(addsuffix .o,$(LIB_SRC)))
BASE_COMPONENTS := Base Input/Input Audio/Audio Paint/Paint
BASE_OBJS := $(addprefix $(BUILD)/app/Base/,$(addsuffix .o,$(BASE_COMPONENTS)))
MAIN_RELEASE_OBJ := $(BUILD)/app/main_release.o
MAIN_TEST_OBJ := $(BUILD)/app/main_test.o

ARENA_COMPONENTS := Arena Combatants/Combatants Entities/Entities Events State \
                    Combatants/Player Combatants/Enemy \
                    Entities/Bullet Entities/Portal Entities/Pickup \
                    Entities/Particle Entities/FloatingText Entities/Shockwave
ARENA_OBJS := $(addprefix $(BUILD)/app/Arena/,$(addsuffix .o,$(ARENA_COMPONENTS)))

SHARED_OBJS := $(BASE_OBJS) $(LIB_OBJS) $(APP_OBJS) $(GAMES_OBJS) $(MENU_OBJ) $(ARENA_OBJS)
RELEASE_OBJS := $(SHARED_OBJS) $(MAIN_RELEASE_OBJ)
TEST_OBJS := $(SHARED_OBJS) $(MAIN_TEST_OBJ)
DEPS := $(RELEASE_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

.PHONY: all release test deploy clean

all: $(BUILD)/NeonCore.exe
release: $(BUILD)/NeonCore.exe
test: $(BUILD)/NeonCoreConsole.exe

$(BUILD):
	@if not exist "$(BUILD_WIN)" mkdir "$(BUILD_WIN)"
	@if not exist "$(BUILD_WIN)\app" mkdir "$(BUILD_WIN)\app"
	@if not exist "$(BUILD_WIN)\lib" mkdir "$(BUILD_WIN)\lib"
	@if not exist "$(BUILD_WIN)\app\Base" mkdir "$(BUILD_WIN)\app\Base"
	@if not exist "$(BUILD_WIN)\app\Base\Input" mkdir "$(BUILD_WIN)\app\Base\Input"
	@if not exist "$(BUILD_WIN)\app\Base\Audio" mkdir "$(BUILD_WIN)\app\Base\Audio"
	@if not exist "$(BUILD_WIN)\app\Base\Paint" mkdir "$(BUILD_WIN)\app\Base\Paint"
	@if not exist "$(BUILD_WIN)\app\Games" mkdir "$(BUILD_WIN)\app\Games"
	@if not exist "$(BUILD_WIN)\app\Games\Waves" mkdir "$(BUILD_WIN)\app\Games\Waves"
	@if not exist "$(BUILD_WIN)\app\Games\Upgrades" mkdir "$(BUILD_WIN)\app\Games\Upgrades"
	@if not exist "$(BUILD_WIN)\app\Menu" mkdir "$(BUILD_WIN)\app\Menu"
	@if not exist "$(BUILD_WIN)\app\Arena" mkdir "$(BUILD_WIN)\app\Arena"
	@if not exist "$(BUILD_WIN)\app\Arena\Combatants" mkdir "$(BUILD_WIN)\app\Arena\Combatants"
	@if not exist "$(BUILD_WIN)\app\Arena\Entities" mkdir "$(BUILD_WIN)\app\Arena\Entities"

$(BASE_OBJS): $(BUILD)/app/Base/%.o: src/app/Base/%.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(LIB_OBJS): $(BUILD)/lib/%.o: src/lib/%.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(GAMES_OBJS): $(BUILD)/app/Games/%.o: src/app/Games/%.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(MENU_OBJ): src/app/Menu/Menu.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(APP_OBJS): $(BUILD)/app/%.o: src/app/%.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(ARENA_OBJS): $(BUILD)/app/Arena/%.o: src/app/Arena/%.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(MAIN_RELEASE_OBJ): src/app/main.cpp | $(BUILD)
	@echo [CXX] $< (release)
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(MAIN_TEST_OBJ): src/app/main.cpp | $(BUILD)
	@echo [CXX] $< (debug/tests)
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -DDEBUG -c $< -o $@

$(BUILD)/NeonCoreConsole.exe: $(TEST_OBJS)
	@echo [LD] $@ (debug/tests)
	@$(CXX) $(TEST_OBJS) -o $@ $(LIBS)

$(BUILD)/NeonCore.exe: $(RELEASE_OBJS)
	@echo [LD] $@ (release)
	@$(CXX) -mwindows $(RELEASE_OBJS) -o $@ $(LIBS)

deploy: release
	@if not exist "$(DIST_WIN)" mkdir "$(DIST_WIN)"
	@if not exist "$(DIST_WIN)\platforms" mkdir "$(DIST_WIN)\platforms"
	@if not exist "$(DIST_WIN)\multimedia" mkdir "$(DIST_WIN)\multimedia"
	@if not exist "$(DIST_WIN)\styles" mkdir "$(DIST_WIN)\styles"
	@copy /Y "$(BUILD_WIN)\NeonCore.exe" "$(DIST_WIN)\NeonCore.exe" >nul
	@if exist "$(DIST_WIN)\NeonCoreConsole.exe" del /Q "$(DIST_WIN)\NeonCoreConsole.exe"
	@copy /Y "THIRD_PARTY.txt" "$(DIST_WIN)\THIRD_PARTY.txt" >nul
	@for %%F in (Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Multimedia.dll Qt6Network.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll avcodec-61.dll avformat-61.dll avutil-59.dll swresample-5.dll swscale-8.dll) do @copy /Y "$(QTDIR_WIN)\bin\%%F" "$(DIST_WIN)\%%F" >nul
	@copy /Y "$(QTDIR_WIN)\plugins\platforms\qwindows.dll" "$(DIST_WIN)\platforms\" >nul
	@copy /Y "$(QTDIR_WIN)\plugins\platforms\qoffscreen.dll" "$(DIST_WIN)\platforms\" >nul
	@copy /Y "$(QTDIR_WIN)\plugins\multimedia\ffmpegmediaplugin.dll" "$(DIST_WIN)\multimedia\" >nul
	@copy /Y "$(QTDIR_WIN)\plugins\multimedia\windowsmediaplugin.dll" "$(DIST_WIN)\multimedia\" >nul
	@copy /Y "$(QTDIR_WIN)\plugins\styles\qmodernwindowsstyle.dll" "$(DIST_WIN)\styles\" >nul
	@echo Deployed to $(DIST)

clean:
	@if exist "$(BUILD_WIN)" rmdir /S /Q "$(BUILD_WIN)"

-include $(DEPS)
