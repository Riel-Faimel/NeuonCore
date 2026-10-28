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
        -Isrc/app/shared -Isrc/app/client -Isrc/app/server -Isrc/app \
        -Isrc/include -Isrc/lib -I$(BUILD)/generated
DEFS := -DQT_WIDGETS_LIB -DQT_MULTIMEDIA_LIB -DQT_GUI_LIB \
        -DQT_CORE_LIB -DQT_NETWORK_LIB -DWIN32 -DWIN64 -D_WIN64 \
        -DMINGW_HAS_SECURE_API=1 -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DNDEBUG
LIBS := -L$(QTDIR)/lib \
        -lQt6Widgets -lQt6Multimedia -lQt6Gui -lQt6Network -lQt6Core \
        -lole32 -loleaut32 -luuid -lws2_32 -ladvapi32 -lshell32 \
        -luser32 -lkernel32 -lgdi32 -lwinmm -limm32 -lversion \
        -ldwmapi -luxtheme -lmsimg32 -static-libgcc -static-libstdc++

LIB_SRC := Math Random
LIB_OBJS := $(addprefix $(BUILD)/lib/,$(addsuffix .o,$(LIB_SRC)))

# 三棵源码树：
#   shared: Arena 数据模型 + 网络协议/快照，服务端和客户端共同链接
#   client: 窗口、输入、音频、菜单、Renderer、本地适配器、TCP 客户端
#   server: GameSimulation、Waves、Upgrades、TCP 服务端
SHARED_COMPONENTS := Arena/Arena Arena/Bullets/Bullets \
                     Arena/Bullets/StandardBullet \
                     Arena/Combatants/Combatants Arena/Entities/Entities \
                     Arena/Events Arena/State \
                     Arena/Combatants/Player Arena/Combatants/Enemy \
                     Arena/Combatants/Enemies/EnemyTypes \
                     Arena/Combatants/Enemies/DroneEnemy Arena/Combatants/Enemies/TankEnemy \
                     Arena/Combatants/Enemies/MiniEnemy Arena/Combatants/Enemies/RusherEnemy \
                     Arena/Combatants/Enemies/SplitterEnemy Arena/Combatants/Enemies/SpitterEnemy \
                     Arena/Combatants/Enemies/SummonerEnemy Arena/Combatants/Enemies/AmplifierEnemy \
                     Arena/Combatants/Enemies/MenderEnemy Arena/Combatants/Enemies/NecromancerEnemy \
                     Arena/Combatants/Enemies/LinkerEnemy Arena/Combatants/Enemies/BossEnemy \
                     Arena/Entities/Mine Arena/Entities/Portal Arena/Entities/Pickup \
                     Arena/Entities/Particle Arena/Entities/FloatingText Arena/Entities/Shockwave \
                     Net/Protocol Net/Snapshot Net/Input Net/Messages
SHARED_OBJS := $(addprefix $(BUILD)/app/shared/,$(addsuffix .o,$(SHARED_COMPONENTS)))

CLIENT_COMPONENTS := Base/Base Base/Input/Input Base/Audio/Audio Base/Audio/Music \
                     Base/Paint/Paint Base/Pointer/Pointer Base/ThreadPool/ThreadPool \
                     Games/Games Renderer/Renderer View/GameView \
                     Net/Client \
                     Menu/Menu Menu/Main/Main Menu/Map/Map Menu/Match/Match \
                     Menu/Room/Room Menu/Failure/Failure \
                     Menu/Result/Result Menu/Depot/Depot \
                     Menu/Editor/Theme Menu/Editor/Tabs Menu/Editor/WaveTable \
                     Menu/Editor/Palette Menu/Editor/ServerRow \
                     Menu/Editor/JoinPanel Menu/Editor/Inspector \
                     Menu/Editor/Actions Menu/Editor/Editor \
                     Menu/Armory/Armory \
                     Menu/Settings/Settings
CLIENT_OBJS := $(addprefix $(BUILD)/app/client/,$(addsuffix .o,$(CLIENT_COMPONENTS)))

SERVER_COMPONENTS := Simulation/GameSimulation Simulation/Waves/Waves \
                     Simulation/Upgrades/Upgrades Net/Server Room/Room \
                     Shop/Shop Shop/Score/Score Shop/Inventory/Inventory Loadout/Loadout
SERVER_OBJS := $(addprefix $(BUILD)/app/server/,$(addsuffix .o,$(SERVER_COMPONENTS)))

MAIN_RELEASE_OBJ := $(BUILD)/app/client/main_release.o
MAIN_TEST_OBJ := $(BUILD)/app/client/main_test.o
SERVER_MAIN_OBJ := $(BUILD)/app/server/main.o
RC := $(MINGWDIR)/bin/windres.exe
ICON_RES_OBJ := $(BUILD)/app/client/app_res.o

# 独立服务端只链接模拟和它依赖的基础服务（Base）；权威 Shop/Loadout 已在 server/。
# 不链接 Menu / Renderer / GameView / TCP Client。
SERVER_SUPPORT_COMPONENTS := Base/Base Base/Input/Input Base/Audio/Audio Base/Audio/Music \
                             Base/Paint/Paint Base/Pointer/Pointer Base/ThreadPool/ThreadPool
SERVER_SUPPORT_OBJS := $(addprefix $(BUILD)/app/client/,$(addsuffix .o,$(SERVER_SUPPORT_COMPONENTS)))
SERVER_APP_OBJS := $(LIB_OBJS) $(SHARED_OBJS) $(SERVER_OBJS) $(SERVER_SUPPORT_OBJS)

# 数值数据：src/json/*.json 在构建期被嵌成 raw string，编译期由 constexpr JSON 解析器读成常量。
JSON_FILES := $(wildcard src/json/*.json) $(wildcard src/json/waves/*.json)
EMBED_TOOL := $(BUILD)/embed_json.exe
GENERATED_JSON := $(BUILD)/generated/GameDataJson.h

LINK_OBJS := $(LIB_OBJS) $(SHARED_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS)
RELEASE_OBJS := $(LINK_OBJS) $(MAIN_RELEASE_OBJ) $(ICON_RES_OBJ)
TEST_OBJS := $(LINK_OBJS) $(MAIN_TEST_OBJ)
DEPS := $(RELEASE_OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(SERVER_APP_OBJS:.o=.d) $(SERVER_MAIN_OBJ:.o=.d)

# 所有编译单元在生成头就绪后再编译（增量重编译由 .d 依赖负责）。
$(LINK_OBJS) $(MAIN_RELEASE_OBJ) $(MAIN_TEST_OBJ) $(SERVER_APP_OBJS) $(SERVER_MAIN_OBJ): | $(GENERATED_JSON)

# 默认目标：第一条规则是上面的 order-only 依赖（空规则），
# 不显式指定的话 `make` 只会构建 build/app/Base/Base.o。
.DEFAULT_GOAL := all

.PHONY: all release test server deploy clean

all: $(BUILD)/NeonCore.exe
release: $(BUILD)/NeonCore.exe
test: $(BUILD)/NeonCoreConsole.exe
server: $(BUILD)/NeonCoreServer.exe

$(BUILD):
	@if not exist "$(BUILD_WIN)" mkdir "$(BUILD_WIN)"
	@if not exist "$(BUILD_WIN)\app" mkdir "$(BUILD_WIN)\app"
	@if not exist "$(BUILD_WIN)\app\client" mkdir "$(BUILD_WIN)\app\client"
	@if not exist "$(BUILD_WIN)\app\server" mkdir "$(BUILD_WIN)\app\server"
	@if not exist "$(BUILD_WIN)\app\shared" mkdir "$(BUILD_WIN)\app\shared"
	@if not exist "$(BUILD_WIN)\lib" mkdir "$(BUILD_WIN)\lib"

$(SHARED_OBJS): $(BUILD)/app/shared/%.o: src/app/shared/%.cpp | $(BUILD)
	@if not exist "$(subst /,\,$(@D))" mkdir "$(subst /,\,$(@D))"
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(CLIENT_OBJS): $(BUILD)/app/client/%.o: src/app/client/%.cpp | $(BUILD)
	@if not exist "$(subst /,\,$(@D))" mkdir "$(subst /,\,$(@D))"
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(SERVER_OBJS): $(BUILD)/app/server/%.o: src/app/server/%.cpp | $(BUILD)
	@if not exist "$(subst /,\,$(@D))" mkdir "$(subst /,\,$(@D))"
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(LIB_OBJS): $(BUILD)/lib/%.o: src/lib/%.cpp | $(BUILD)
	@if not exist "$(BUILD_WIN)\lib" mkdir "$(BUILD_WIN)\lib"
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(MAIN_RELEASE_OBJ): src/app/client/main.cpp | $(BUILD)
	@if not exist "$(BUILD_WIN)\app\client" mkdir "$(BUILD_WIN)\app\client"
	@echo [CXX] $< (release)
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(MAIN_TEST_OBJ): src/app/client/main.cpp | $(BUILD)
	@if not exist "$(BUILD_WIN)\app\client" mkdir "$(BUILD_WIN)\app\client"
	@echo [CXX] $< (debug/tests)
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -DDEBUG -c $< -o $@

$(SERVER_MAIN_OBJ): src/app/server/main.cpp | $(BUILD)
	@if not exist "$(BUILD_WIN)\app\server" mkdir "$(BUILD_WIN)\app\server"
	@echo [CXX] $< (server)
	@$(CXX) $(CXXFLAGS) $(INCS) $(DEFS) -c $< -o $@

$(ICON_RES_OBJ): src/app/client/app.rc assets/icon.ico | $(BUILD)
	@if not exist "$(BUILD_WIN)\app\client" mkdir "$(BUILD_WIN)\app\client"
	@echo [RC] $<
	@$(RC) -I assets -i $< -o $@

$(EMBED_TOOL): tools/embed_json.cpp | $(BUILD)
	@echo [HOST] $<
	@$(CXX) -std=c++20 -O2 $< -o $@

$(GENERATED_JSON): $(EMBED_TOOL) $(JSON_FILES) | $(BUILD)
	@if not exist "$(BUILD_WIN)\generated" mkdir "$(BUILD_WIN)\generated"
	@echo [JSON] $@
	@$(EMBED_TOOL) $@ player_json src/json/player.json enemies_json src/json/enemies.json waves_level_01 src/json/waves/level-01.json waves_level_02 src/json/waves/level-02.json waves_level_03 src/json/waves/level-03.json waves_level_04 src/json/waves/level-04.json loadout_json src/json/loadout.json upgrades_json src/json/upgrades.json bullets_json src/json/bullets.json items_json src/json/items.json levels_json src/json/levels.json

$(BUILD)/NeonCoreConsole.exe: $(TEST_OBJS)
	@echo [LD] $@ (debug/tests)
	@$(CXX) $(TEST_OBJS) -o $@ $(LIBS)

$(BUILD)/NeonCore.exe: $(RELEASE_OBJS)
	@echo [LD] $@ (release)
	@$(CXX) -mwindows $(RELEASE_OBJS) -o $@ $(LIBS)

$(BUILD)/NeonCoreServer.exe: $(SERVER_APP_OBJS) $(SERVER_MAIN_OBJ)
	@echo [LD] $@ (server)
	@$(CXX) $(SERVER_APP_OBJS) $(SERVER_MAIN_OBJ) -o $@ $(LIBS)

deploy: release server
	@if not exist "$(DIST_WIN)" mkdir "$(DIST_WIN)"
	@if not exist "$(DIST_WIN)\music" mkdir "$(DIST_WIN)\music"
	@if not exist "$(DIST_WIN)\platforms" mkdir "$(DIST_WIN)\platforms"
	@if not exist "$(DIST_WIN)\multimedia" mkdir "$(DIST_WIN)\multimedia"
	@if not exist "$(DIST_WIN)\styles" mkdir "$(DIST_WIN)\styles"
	@copy /Y "$(BUILD_WIN)\NeonCore.exe" "$(DIST_WIN)\NeonCore.exe" >nul
	@copy /Y "$(BUILD_WIN)\NeonCoreServer.exe" "$(DIST_WIN)\NeonCoreServer.exe" >nul
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
