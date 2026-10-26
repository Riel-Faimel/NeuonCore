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
        -Isrc/app -Isrc/include -Isrc/lib -I$(BUILD)/generated
DEFS := -DQT_WIDGETS_LIB -DQT_MULTIMEDIA_LIB -DQT_GUI_LIB \
        -DQT_CORE_LIB -DQT_NETWORK_LIB -DWIN32 -DWIN64 -D_WIN64 \
        -DMINGW_HAS_SECURE_API=1 -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DNDEBUG
LIBS := -L$(QTDIR)/lib \
        -lQt6Widgets -lQt6Multimedia -lQt6Gui -lQt6Network -lQt6Core \
        -lole32 -loleaut32 -luuid -lws2_32 -ladvapi32 -lshell32 \
        -luser32 -lkernel32 -lgdi32 -lwinmm -limm32 -lversion \
        -ldwmapi -luxtheme -lmsimg32 -static-libgcc -static-libstdc++

GAMES_COMPONENTS := Games Waves/Waves Upgrades/Upgrades Renderer/Renderer
GAMES_OBJS := $(addprefix $(BUILD)/app/Games/,$(addsuffix .o,$(GAMES_COMPONENTS)))
MENU_COMPONENTS := Menu Main/Main Map/Map Match/Match Result/Result Editor/Editor Depot/Depot Settings/Settings
MENU_OBJS := $(addprefix $(BUILD)/app/Menu/,$(addsuffix .o,$(MENU_COMPONENTS)))
LIB_SRC := Math Random
LIB_OBJS := $(addprefix $(BUILD)/lib/,$(addsuffix .o,$(LIB_SRC)))
BASE_COMPONENTS := Base Input/Input Audio/Audio Audio/Music Paint/Paint Pointer/Pointer
SHOP_COMPONENTS := Shop Score/Score Inventory/Inventory
SHOP_OBJS := $(addprefix $(BUILD)/app/Shop/,$(addsuffix .o,$(SHOP_COMPONENTS)))
BASE_OBJS := $(addprefix $(BUILD)/app/Base/,$(addsuffix .o,$(BASE_COMPONENTS)))
MAIN_RELEASE_OBJ := $(BUILD)/app/main_release.o
MAIN_TEST_OBJ := $(BUILD)/app/main_test.o
RC := $(MINGWDIR)/bin/windres.exe
ICON_RES_OBJ := $(BUILD)/app/app_res.o

# 数值数据：src/json/*.json 在构建期被嵌成 raw string，编译期由 constexpr JSON 解析器读成常量。
JSON_FILES := $(wildcard src/json/*.json) $(wildcard src/json/waves/*.json)
EMBED_TOOL := $(BUILD)/embed_json.exe
GENERATED_JSON := $(BUILD)/generated/GameDataJson.h

ARENA_COMPONENTS := Arena Bullets/Bullets Bullets/StandardBullet \
                    Combatants/Combatants Entities/Entities Events State \
                    Combatants/Player Combatants/Enemy \
                    Combatants/Enemies/EnemyTypes \
                    Combatants/Enemies/DroneEnemy Combatants/Enemies/TankEnemy \
                    Combatants/Enemies/MiniEnemy Combatants/Enemies/RusherEnemy \
                    Combatants/Enemies/SplitterEnemy Combatants/Enemies/SpitterEnemy \
                    Combatants/Enemies/SummonerEnemy Combatants/Enemies/AmplifierEnemy \
                    Combatants/Enemies/MenderEnemy Combatants/Enemies/NecromancerEnemy \
                    Combatants/Enemies/LinkerEnemy Combatants/Enemies/BossEnemy \
                    Entities/Mine Entities/Portal Entities/Pickup \
                    Entities/Particle Entities/FloatingText Entities/Shockwave
ARENA_OBJS := $(addprefix $(BUILD)/app/Arena/,$(addsuffix .o,$(ARENA_COMPONENTS)))

SHARED_OBJS := $(BASE_OBJS) $(LIB_OBJS) $(GAMES_OBJS) $(MENU_OBJS) $(SHOP_OBJS) $(ARENA_OBJS)
RELEASE_OBJS := $(SHARED_OBJS) $(MAIN_RELEASE_OBJ) $(ICON_RES_OBJ)
TEST_OBJS := $(SHARED_OBJS) $(MAIN_TEST_OBJ)
DEPS := $(RELEASE_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# 所有编译单元在生成头就绪后再编译（增量重编译由 .d 依赖负责）。
$(SHARED_OBJS) $(MAIN_RELEASE_OBJ) $(MAIN_TEST_OBJ): | $(GENERATED_JSON)

# 默认目标：第一条规则是上面的 order-only 依赖（空规则），
# 不显式指定的话 `make` 只会构建 build/app/Base/Base.o。
.DEFAULT_GOAL := all

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
	@if not exist "$(BUILD_WIN)\app\Base\Pointer" mkdir "$(BUILD_WIN)\app\Base\Pointer"
	@if not exist "$(BUILD_WIN)\app\Games" mkdir "$(BUILD_WIN)\app\Games"
	@if not exist "$(BUILD_WIN)\app\Games\Waves" mkdir "$(BUILD_WIN)\app\Games\Waves"
	@if not exist "$(BUILD_WIN)\app\Games\Upgrades" mkdir "$(BUILD_WIN)\app\Games\Upgrades"
	@if not exist "$(BUILD_WIN)\app\Games\Renderer" mkdir "$(BUILD_WIN)\app\Games\Renderer"
	@if not exist "$(BUILD_WIN)\app\Menu" mkdir "$(BUILD_WIN)\app\Menu"
	@if not exist "$(BUILD_WIN)\app\Shop" mkdir "$(BUILD_WIN)\app\Shop"
	@if not exist "$(BUILD_WIN)\app\Shop\Score" mkdir "$(BUILD_WIN)\app\Shop\Score"
	@if not exist "$(BUILD_WIN)\app\Shop\Inventory" mkdir "$(BUILD_WIN)\app\Shop\Inventory"
	@if not exist "$(BUILD_WIN)\app\Menu\Main" mkdir "$(BUILD_WIN)\app\Menu\Main"
	@if not exist "$(BUILD_WIN)\app\Menu\Map" mkdir "$(BUILD_WIN)\app\Menu\Map"
	@if not exist "$(BUILD_WIN)\app\Menu\Match" mkdir "$(BUILD_WIN)\app\Menu\Match"
	@if not exist "$(BUILD_WIN)\app\Menu\Result" mkdir "$(BUILD_WIN)\app\Menu\Result"
	@if not exist "$(BUILD_WIN)\app\Menu\Editor" mkdir "$(BUILD_WIN)\app\Menu\Editor"
	@if not exist "$(BUILD_WIN)\app\Menu\Settings" mkdir "$(BUILD_WIN)\app\Menu\Settings"
	@if not exist "$(BUILD_WIN)\app\Menu\Depot" mkdir "$(BUILD_WIN)\app\Menu\Depot"
	@if not exist "$(BUILD_WIN)\app\Arena" mkdir "$(BUILD_WIN)\app\Arena"
	@if not exist "$(BUILD_WIN)\app\Arena\Combatants" mkdir "$(BUILD_WIN)\app\Arena\Combatants"
	@if not exist "$(BUILD_WIN)\app\Arena\Combatants\Enemies" mkdir "$(BUILD_WIN)\app\Arena\Combatants\Enemies"
	@if not exist "$(BUILD_WIN)\app\Arena\Bullets" mkdir "$(BUILD_WIN)\app\Arena\Bullets"
	@if not exist "$(BUILD_WIN)\app\Arena\Entities" mkdir "$(BUILD_WIN)\app\Arena\Entities"

$(BASE_OBJS): $(BUILD)/app/Base/%.o: src/app/Base/%.cpp | $(BUILD)
	@if not exist "$(subst /,\,$(@D))" mkdir "$(subst /,\,$(@D))"
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(LIB_OBJS): $(BUILD)/lib/%.o: src/lib/%.cpp | $(BUILD)
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(GAMES_OBJS): $(BUILD)/app/Games/%.o: src/app/Games/%.cpp | $(BUILD)
	@if not exist "$(subst /,\,$(@D))" mkdir "$(subst /,\,$(@D))"
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(SHOP_OBJS): $(BUILD)/app/Shop/%.o: src/app/Shop/%.cpp | $(BUILD)
	@if not exist "$(subst /,\,$(@D))" mkdir "$(subst /,\,$(@D))"
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(MENU_OBJS): $(BUILD)/app/Menu/%.o: src/app/Menu/%.cpp | $(BUILD)
	@if not exist "$(subst /,\,$(@D))" mkdir "$(subst /,\,$(@D))"
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

$(ICON_RES_OBJ): src/app/app.rc assets/icon.ico | $(BUILD)
	@echo [RC] $<
	@$(RC) -I assets -i $< -o $@

$(EMBED_TOOL): tools/embed_json.cpp | $(BUILD)
	@echo [HOST] $<
	@$(CXX) -std=c++20 -O2 $< -o $@

$(GENERATED_JSON): $(EMBED_TOOL) $(JSON_FILES) | $(BUILD)
	@if not exist "$(BUILD_WIN)\generated" mkdir "$(BUILD_WIN)\generated"
	@echo [JSON] $@
	@$(EMBED_TOOL) $@ player_json src/json/player.json enemies_json src/json/enemies.json waves_level_01 src/json/waves/level-01.json upgrades_json src/json/upgrades.json bullets_json src/json/bullets.json items_json src/json/items.json levels_json src/json/levels.json

$(BUILD)/NeonCoreConsole.exe: $(TEST_OBJS)
	@echo [LD] $@ (debug/tests)
	@$(CXX) $(TEST_OBJS) -o $@ $(LIBS)

$(BUILD)/NeonCore.exe: $(RELEASE_OBJS)
	@echo [LD] $@ (release)
	@$(CXX) -mwindows $(RELEASE_OBJS) -o $@ $(LIBS)

deploy: release
	@if not exist "$(DIST_WIN)" mkdir "$(DIST_WIN)"
	@if not exist "$(DIST_WIN)\music" mkdir "$(DIST_WIN)\music"
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
