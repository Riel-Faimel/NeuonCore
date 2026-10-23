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
        -Isrc
DEFS := -DQT_WIDGETS_LIB -DQT_MULTIMEDIA_LIB -DQT_GUI_LIB \
        -DQT_CORE_LIB -DQT_NETWORK_LIB -DWIN32 -DWIN64 -D_WIN64 \
        -DMINGW_HAS_SECURE_API=1 -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DNDEBUG
LIBS := -L$(QTDIR)/lib \
        -lQt6Widgets -lQt6Multimedia -lQt6Gui -lQt6Network -lQt6Core \
        -lole32 -loleaut32 -luuid -lws2_32 -ladvapi32 -lshell32 \
        -luser32 -lkernel32 -lgdi32 -lwinmm -limm32 -lversion \
        -ldwmapi -luxtheme -lmsimg32 -static-libgcc -static-libstdc++

ROOT_SRC := Math Random Waves Upgrades Renderer Game
ROOT_OBJS := $(addprefix $(BUILD)/,$(addsuffix .o,$(ROOT_SRC)))
ARENA_COMPONENTS := Arena Player Enemy Bullet Portal Pickup Particle \
                    FloatingText Shockwave Collision Kill BossState Score
ARENA_OBJS := $(addprefix $(BUILD)/Arena_,$(addsuffix .o,$(ARENA_COMPONENTS)))

SHARED_OBJS := $(BUILD)/Base.o $(ROOT_OBJS) $(ARENA_OBJS)
RELEASE_OBJS := $(SHARED_OBJS) $(BUILD)/main_release.o
TEST_OBJS := $(SHARED_OBJS) $(BUILD)/main_test.o
DEPS := $(RELEASE_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

.PHONY: all release test deploy clean

all: $(BUILD)/NeonCore.exe
release: $(BUILD)/NeonCore.exe
test: $(BUILD)/NeonCoreConsole.exe

$(BUILD):
	@if not exist "$(BUILD_WIN)" mkdir "$(BUILD_WIN)"

$(BUILD)/Base.o: src/base/Base.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(ROOT_OBJS): $(BUILD)/%.o: src/%.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(ARENA_OBJS): $(BUILD)/Arena_%.o: src/Arena/%.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(BUILD)/main_release.o: src/main.cpp | $(BUILD)
	@echo [CXX] $< (release)
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(BUILD)/main_test.o: src/main.cpp | $(BUILD)
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
