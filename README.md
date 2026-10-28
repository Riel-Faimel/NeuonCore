# 星核协议 / NEON CORE — C++ / Qt 6 版

这是 Python + pygame 版本的 C++ 复刻版。玩法、数值和状态机对照 `main.py` 移植，没有改动机制：

- 8 个波次、4 种普通敌人、最终 Boss「虚空吞噬者」；胜负只看台本是否打完，Boss 只是强敌，放在第几波都不改变结束条件
- 背景音乐：`music\` 目录下的音频文件（MP4 / MP3 / M4A / FLAC / OGG / WAV 等）按文件名顺序轮转播放，一首结束自动切下一首
- 同样的波次组成、敌人数值、Boss 血量阶段、弹幕与召唤逻辑
- 同样的 9 种强化协议、叠加效果和生命/伤害/射速数值（每轮数值在区间内随机）
- 同样的碰撞判定、冲刺无敌、击退、掉落概率、波次间隔
- 同样的标题、战斗、强化、暂停、胜利、失败流程

随机数由 C++ `std::mt19937` 产生，因此具体敌人出生位置和强化卡顺序不会与 Python 版逐帧一致；机制和数值保持不变。

![C++ 版游戏画面](screenshots/gameplay.png)

## 直接运行

预编译的 Windows x64 版本位于：

```text
cpp\dist\NeonCore\NeonCore.exe
```

双击仓库根目录的 **`start_cpp.bat`**，或 `cpp\run.bat` 即可启动。`dist\NeonCore` 已经包含运行所需的 Qt 6 和 FFmpeg DLL，不需要安装 Qt。

默认启动方式已经是“客户端 + 服务端”结构：客户端在 127.0.0.1 上自动选择一个空闲端口，启动一个**内嵌的本地服务端**，再通过真实 TCP 连接进入对局。单机和联机使用同一套协议，因此不需要为了单机维护第二条逻辑。协议版本 3 支持真正的多玩家会话，并把快照改为 30Hz 压缩 keyframe：房间里的 2–4 个客户端各拿到一个稳定 playerId/slot，各自控制自己的飞机（不同出生点/颜色），快照同时包含所有玩家并为每个客户端标记 `localPlayerId`。单机仍是 1 架飞机，HUD/操作不变。

## 服务端 / 联机

独立服务端（无窗口，60Hz 固定 tick / 30Hz 压缩快照广播）：

```bat
dist\NeonCore\NeonCoreServer.exe --port 7777
```

远程客户端（先启动上面的服务端）：

```bat
dist\NeonCore\NeonCore.exe --connect 127.0.0.1:7777
```

其他命令行：

```text
--local                 离线直连模式（不经过 TCP；自动化测试和回退用）
--connect host:port     连接远端服务端（也可以从自定义协议的“加入主机”进入）
NeonCoreServer.exe --port 7777
                        独立服务端（无窗口、60Hz 固定 tick、30Hz 压缩快照）
```

`build\NeonCoreServer.exe --port 7777` 是独立服务端，保留控制台输出，便于排查端口占用等问题；服务端模式已从 GUI 客户端移除，统一用 `NeonCoreServer.exe`。服务端只跑 `GameSimulation`（没有 Renderer / Menu / 输入采集）。控制面不走 TCP：`StartMatch` 只接受本地宿主（内嵌 `Match` 或 `NeonCoreServer` 的 stdin）；TCP 客户端只做 Hello / Input / 接收 `Snapshot / Event / MatchEnd`，不能开局或改服务器配置。独立服务端的 stdin 接受与 TCP 相同的 `[u32 length][payload]` 控制帧（`ClaimAdmin` / `StartMatch` / `Input`，可用 `InputCommand.playerId` 选择控制哪一架飞机）；宿主先用 Welcome 分配的 sessionId 发 `ClaimAdmin`，服务端据此判定管理员，只有管理员能 start/return，stdout 只输出启动日志；这样只有启动它的父进程能控制对局，网络客户端无法提交 `StartMatch`。没有对局时服务端广播 `RoomList`，客户端显示房间/大厅；房间的名字、容量、房主会话和会话槽位全部由服务端决定，人数直接由实际 welcomed 会话的成员列表派生，连接/断开实时更新（最多 4 人，第 5 个连接会被拒绝）；加入房间不需要单独的请求消息。对局开始时服务端把当前所有 welcomed 会话组成 roster，每个会话的输入只作用于自己的飞机；开局后加入的会话只能观战，输入会被忽略。玩家对局中掉线时服务端只移除那一架飞机，其他玩家继续；全部离开才结束对局。对局开始后才切换到快照。房主解散房间时服务端先广播 `MatchEnd("房间已解散")` 再关闭监听/断开连接；客户端无论收到消息还是纯断线，都会进入独立的“无法连接服务器 / 房间已解散”失败页，而不是空对局或通用失败结算。端口被占用、握手协议版本不匹配、客户端断线、连接失败和重连失败都会返回错误并保持进程不崩溃。

### 局域网房间流程

自定义协议的“局域网主机”选项卡按 `ENTER` 或点“部署”后**不会立即开局**，而是启动内嵌服务端（按设定端口绑定所有网卡）并进入房间页：

- 房间页显示房间名、地址、服务端会话槽位列表（P1–P4、房主/你/对局中标记）、已连接玩家/上限、等待状态和台本摘要；
- 房主按 `ENTER` 才开始对面的权威对局（本地直接调用 `GameServer::startMatch()`，不走 TCP `StartMatch`），当前房间里所有已连接会话一起进入对局、各控制自己的飞机；
- 房主按 `X` 解散房间：服务端关闭，所有加入者进入“房间已解散”失败页；房主回到主菜单；
- 房主/加入者按 `ESC` 离开或返回；加入者不能开始或解散，只能等待房主；对局中离开只移除自己的飞机，其他玩家继续；
- “加入主机”连接到正在等待的房间时显示同一份房间数据；连接失败、服务器不存在或房主解散时显示带 `ENTER` 重试 / `ESC` 返回的专用失败页；开局后加入的会话进入观战；
- “单机”选项卡仍是本地直连、立即开局；“标准协议”与单机内嵌服务端路径完全不变。

## 窗口与 QML 事件

窗口事件、坐标换算、QML 覆盖层、准星和缩放流程见 [`docs/WINDOW_EVENTS.md`](docs/WINDOW_EVENTS.md)。

## 分支维护

- `main`：QML 主开发线；
- `widgets-only`：纯 Widgets 回退线；

两条线的范围、cherry-pick 流程和发布检查见 [`docs/BRANCHES.md`](docs/BRANCHES.md)。

## 操作

| 按键 | 动作 |
| --- | --- |
| `WASD` / 方向键 | 移动 |
| 鼠标 | 瞄准 |
| 鼠标左键 / `J` | 持续射击 |
| `SPACE` | 相位冲刺，短暂无敌 |
| 鼠标右键 | 在光标处布置地雷（消耗库存） |
| `P` / `ESC` | 暂停 / 继续 |
| `M` | 静音 / 恢复音效 |
| `1` `2` `3` | 选择强化协议 |
| `R` | 胜利 / 失败后重开 |
| `Q` | 暂停界面返回标题 |

标题页另有：`↑` `↓` / `W` `S` 切换卡片；选中“标准协议”时 `ENTER`（或点卡片）进入 **星区地图**，地图上 `←` `→` 选择区域、`ENTER` 进入当前区域、`ESC` 返回；选中“自定义协议”时 `ENTER`（或点卡片）进入带三个选项卡的编辑界面：**单机 / 局域网主机 / 加入主机**；单机与局域网主机显示台本编辑，主机额外在波次下方显示服务器端口，加入主机只显示地址和端口；`1` 进地图，`2` 进编辑器，`3` 进战备。标题页右上角还有“战备”入口，可选择发射器（标准/散射/磁轨）。

台本编辑器：表格只显示当前用到的敌人列；从底部“敌人库”把敌人**拖到某一波**（或按 `1`–`9`、`0`）才会加入该列。`↑` `↓` / `W` `S` 选波次，`←` `→` / `A` `D` 选列，`+` / `-` 调数量或开关 Boss（**按住不放会连续快速加减**），`N` 新增波次、`X` 删除末波（1–12 波，单格 0–20），`ENTER` 部署，`ESC` 返回。编辑结果只存在内存里，重启回到默认台本。

房间页：房主按 `ENTER` 开始对局、`X` 解散房间、`ESC` 离开并关闭房间；加入者按 `ESC` 离开。失败页按 `ENTER` 重试连接、`ESC` 返回主菜单。

## 设置

主菜单右上角的齿轮按钮进入设置页，可以分别调整 **音效音量** 和 **背景音乐音量**（0–100%）：

- `↑` `↓` 选择行，`←` `→` 调整（每次 5%），也可以直接点/拖进度条；
- `ESC` 返回主菜单；调整立即生效；
- 音量保存到配置存储，下次启动自动恢复：**Windows 写注册表**（`HKCU\Software\NeonCore\NeonCore`，MSIX/只读安装目录也不会写文件失败），其他平台写 exe 同级的 `settings.ini`（见 `Data/Settings.h` 的条件编译）；
- 同一个配置存储还保存积分钱包和地雷库存（键 `score/total`、`inventory/mines`）。

## 商店

主菜单右上角的补给箱按钮进入商店，目前售卖 **震荡地雷**（120 积分）：

- 点击 / `ENTER` 购买，从全局积分扣款，库存 +1（写入配置存储）；
- 局内 **右键** 在光标处布置，敌人进入感应范围即引爆，对爆炸范围内敌人造成大量伤害；
- 地雷是一次性消耗品：布置即扣库存，每局开始（新一波）会清掉场上未引爆的地雷；
- 价格、伤害、感应/爆炸半径、引爆延迟都在 `src/json/items.json`。

## 构建

需要：

- Qt 6.10.2 `mingw_64`
- Qt 自带的 MinGW 13.1 工具链

本机默认路径：

```text
QTDIR   = D:\Qt\6.10.2\mingw_64
MINGWDIR= D:\Qt\Tools\mingw1310_64
```

如果安装在别处，先设置环境变量，再运行：

```bat
set QTDIR=D:\path\to\Qt\6.x\mingw_64
set MINGWDIR=D:\path\to\Qt\Tools\mingw1310_64
mingw32-make -j12 deploy
```

本机 12 核，推荐用 Makefile 并行构建：

```bat
:: 交付用 release GUI，不包含测试代码
mingw32-make -j12 deploy

:: 需要自动化测试时，构建带 -DDEBUG 的测试控制台
mingw32-make -j12 test
```

- `all` / `release` / 默认目标：只构建 `build\NeonCore.exe`，不编译 `SelfTest`
- `test`：用 `-DDEBUG` 构建 `build\NeonCoreConsole.exe`，包含测试入口
- `server`：构建独立服务端 `build\NeonCoreServer.exe`
- `deploy`：构建 release 客户端 + 独立服务端，并复制 Qt/FFmpeg 运行时到 `dist\NeonCore`
- `clean`：删除 `build`

`Makefile` 直接调用 `g++`，不依赖 qmake 或 CMake。`build.bat` 现在只是 `mingw32-make -j12 deploy` 的包装。构建产物：

```text
build\NeonCore.exe          release GUI 客户端，不包含测试逻辑
build\NeonCoreServer.exe    独立服务端（无窗口）
build\NeonCoreConsole.exe   debug/test 控制台，包含 SelfTest
dist\NeonCore\NeonCore.exe  交付用 release GUI 客户端
dist\NeonCore\NeonCoreServer.exe 交付用独立服务端
```

测试代码位于 `src/tests/SelfTest.h`，只在 `src/app/client/main.cpp` 的 `#ifdef DEBUG` 分支里包含；`-DDEBUG` 只用于 `test` 目标。

## 数值数据

可调数值都在 `src/json/`，构建时嵌入、**编译期**解析，运行时不读文件也不解析 JSON：

```text
src/json/*.json
  → build/embed_json.exe 生成 build/generated/GameDataJson.h（原样 raw string）
  → src/include/Data/GameData.h 用 constexpr 解析器（src/lib/Json.h）展开成
     data::kPlayer / kEnemies / kCampaigns / kLaunchers / kWaveTiming / kUpgradeRanges / k*Bullet
  → 游戏代码直接读这些 inline constexpr 常量
```

| 文件 | 内容 |
| --- | --- |
| `player.json` | 玩家血量、移速、伤害、射速、弹速、弹体半径 |
| `enemies.json` | 各敌人血量 / 速度 / 半径 / 接触伤害 / 分数 |
| `waves/level-01..04.json` | 四个关卡各自的每波组成、出怪节奏、波次成长系数 |
| `loadout.json` | 战备发射器：伤害/射速/弹速/散射/额外弹丸/穿透 |
| `upgrades.json` | 强化每轮随机区间（`min` / `max`） |
| `bullets.json` | 敌方子弹速度 / 伤害 / 半径 |
| `items.json` | 商店道具（地雷）价格 / 伤害 / 范围 / 引爆延迟 |

- 改完直接 `make`：JSON 会重新嵌入，包含数值表的 TU 自动重编译；格式写错会退化成默认值/空表，启动时由 `data::validationError()` 返回错误，`main` 打印并退出；
- 解析器限制：字符串不使用转义、数字不使用指数；表项数量上限见 `GameData.h` 的 `Max*` 常量；
- 运行时开销为零：`data::*` 都是 `inline constexpr`，游戏里没有 JSON 解析逻辑；
- 生成的头文件在 `build/generated/`，不进版本库。

## 代码结构

```text
cpp/
├─ src/
│  ├─ qml/
│  │  ├─ MainOverlay.qml      QML 标题/关卡卡/入口
│  │  ├─ SettingsOverlay.qml   QML 音量设置
│  │  ├─ DepotOverlay.qml     QML 补给站
│  │  ├─ ArmoryOverlay.qml    QML 战备/发射器
│  │  ├─ ResultOverlay.qml    QML 结算覆盖层（唯一 UI 路径）
│  │  ├─ FailureOverlay.qml   QML 连接/房间失败页
│  │  ├─ NeonText.qml         六向 glow 文本
│  │  ├─ MainEntry.qml        标题页入口
│  │  └─ Reticle.qml          QML 准星/射线
│  ├─ app/
│  │  ├─ shared/
│  │  │  ├─ Arena/
│  │  │  │  ├─ Arena.h / Arena.cpp   模块入口：私有继承 5 个组件 + 编排
│  │  │  │  ├─ Context.h / Host.h   组件注入与模块回调接口
│  │  │  │  ├─ Events / State        碰撞击杀、Boss 阶段与玩家失败
│  │  │  │  ├─ Bullets/             子弹池 + StandardBullet
│  │  │  │  ├─ Combatants/          Player / Enemy / 敌人注册表与各类敌人
│  │  │  │  └─ Entities/            传送门、掉落、地雷、粒子、飘字、冲击波
│  │  │  └─ Net/
│  │  │     ├─ Protocol.h/.cpp      帧格式、消息 id、FrameBuffer（服务端/客户端共享）
│  │  │     ├─ Input.h/.cpp         客户端 → 服务端输入命令
│  │  │     ├─ Messages.h/.cpp      Hello / Welcome / StartMatch / Event / MatchEnd / RoomList
│  │  │     └─ Snapshot.h/.cpp      只读世界快照模型 + 单帧 QDataStream 序列化
│  │  ├─ server/
│  │  │  ├─ Simulation/
│  │  │  │  ├─ GameSimulation.h/.cpp 模拟：Arena / Waves / Upgrades、状态、计时、输入
│  │  │  │  ├─ Waves/Waves.h/.cpp    按提交的台本出生敌人
│  │  │  │  └─ Upgrades/Upgrades.h/.cpp 强化池、抽取与应用
│  │  │  ├─ Net/Server.h/.cpp        QTcpServer：headless GameSimulation + 30Hz 压缩快照 + 房间生命周期
│  │  │  ├─ Room/Room.h/.cpp         服务端权威房间：名字/容量/房主/等待或对局中（单房间）
│  │  │  ├─ Shop/                    权威积分 + 地雷库存（持久化）
│  │  │  ├─ Loadout/                 权威发射器（持久化）
│  │  │  └─ main.cpp                 独立服务端入口（--port，stdin 控制）
│  │  └─ client/
│  │     ├─ Base/
│  │     │  ├─ Base.h / Base.cpp     模块入口：Input / Audio / Music / Pointer / Paint / ThreadPool + BaseService
│  │     │  ├─ Input/ Audio/ Pointer/ Paint/ ThreadPool/
│  │     ├─ Games/Games.h / Games.cpp 单机适配器：GameSimulation + Renderer
│  │     ├─ Renderer/Renderer.h/.cpp 局内背景、发光、HUD、卡片、暂停（只读 GameView）
│  │     ├─ View/GameView.h/.cpp     客户端视图：快照 → 可绘制的 Arena 只读镜像
│  │     ├─ Net/Client.h/.cpp        QTcpSocket：握手、输入、快照、GameView、Renderer
│  │     ├─ Menu/
│  │     │  ├─ Menu.h/.cpp           屏幕状态机、输入路由、关卡表
│  │     │  ├─ Main/ Map/ Match/ Result/
│  │     │  ├─ Room/                     房间/大厅：房主开始/解散、加入者等待
│  │     │  ├─ Failure/                  无法连接服务器 / 房间已解散（重试/返回）
│  │     │  ├─ Depot/ / Armory/
│  │     │  ├─ Editor/Editor.h/.cpp      自定义协议：三个选项卡 + 台本/服务器编辑
│  │     │  └─ Settings/
│  │     ├─ app.rc                   Windows 资源：嵌入应用图标
│  │     └─ main.cpp                 QWidget 窗口、输入桥接、全局服务初始化、客户端入口
│  ├─ lib/
│  │  ├─ Math.h / Math.cpp        向量与几何
│  │  ├─ Json.h                   constexpr JSON 解析器
│  │  └─ Random.h / Random.cpp    可注入随机数
│  ├─ include/
│  │  ├─ Data/GameData.h          编译期数值表（展开自 src/json）
│  │  ├─ Data/Settings.h          配置存储（Windows 注册表 / 其他平台 ini）
│  │  ├─ GameConfig.h             全局尺寸常量
│  │  ├─ Fighter.h                Player/Enemy 的公共虚接口
│  │  └─ Bullet.h                 子弹的全局纯虚接口（池只认这一层）
│  ├─ json/
│  │  ├─ player.json              玩家基础数值
│  │  ├─ enemies.json             敌人基础数值
│  │  ├─ waves/level-01..04.json  四个关卡的台本 + 出怪节奏
│  │  ├─ loadout.json             战备发射器数值
│  │  ├─ levels.json              星区地图关卡节点
│  │  ├─ items.json               商店道具数值
│  │  ├─ upgrades.json            强化数值区间
│  │  └─ bullets.json             敌方子弹数值
│  └─ tests/
│     └─ SelfTest.h               仅 -DDEBUG 测试构建包含的测试/截图逻辑
├─ tools/
│  └─ embed_json.cpp              构建期把 JSON 嵌成 raw string
├─ build.bat                      调用 mingw32-make -j12 deploy
├─ Makefile                       mingw32-make -j12 并行构建
├─ run.bat
├─ assets/
│  ├─ icon.png                    应用图标（--capture 输出的 256px 版本）
│  └─ icon.ico                    多尺寸图标，由 app.rc 嵌入 exe
├─ ARCHITECTURE.md                模块边界、依赖和扩展指南
├─ screenshots/
└─ dist/NeonCore/                 交付用 release GUI + 运行库
```

`src/app/client/Base/` 是模块目录：`Base` 是全局模块类，组合 `Input`、`Audio`、`Music`、`Pointer`、`Paint`、`ThreadPool` 六个 `namespace base` 组件。`Base` 是**进程级单例**：唯一实例是 `BaseService`（`extern Base BaseService;`，定义在 `Base.cpp`），Menu / Games / Arena / Renderer / Shop 都直接调用 `BaseService`，不再通过构造函数传递 `Base&`。`Base.h` 仍只暴露 Base 的输入/音频/指针/并行/绘制接口；`Input` 的键鼠状态、`Audio` 的 QSoundEffect/WAV、`Music` 的 QMediaPlayer、`Pointer` 的位置/时钟/准星、`Paint` 的 QFontDatabase 细节、`ThreadPool` 的线程管理都在各自组件内。字体与工作线程由 `main()` 在 `QApplication` 之后调 `Base::initialize()` 初始化；音频由 `BaseService.initAudio()` 按需初始化，`main()` 退出前调 `BaseService.shutdown()`（先停线程池，再释放 Qt 音频对象）。窗口通过 `BaseService.pointerMoveTo/pointerSetVisible/pointerUpdate/pointerDraw` 驱动 Widgets 准星；QML 结算/失败页通过 `Reticle.qml` 读取同一组指针状态绘制，详见 `docs/WINDOW_EVENTS.md`。

`src/app/client/Games/` 是单机适配器目录：`Games` 公开继承 `games::GameSimulation`，自己只拥有 `games::Renderer` 并实现模拟的表现钩子。`GameSimulation` 在 `src/app/server/Simulation/`，只推进 `Arena / Waves / Upgrades`、一局状态、运行时间和输入，不依赖 Renderer；`Waves` 和 `Upgrades` 也在 `src/app/server/Simulation/`。本地离线路径同样链接这份模拟，但服务端仍是联机时的唯一权威实例。`Games::update()` 每帧先从 `BaseService` 采集 `arena::InputState`（移动/鼠标/射击），再调用模拟，并通过 `on*` 钩子调用渲染器的震屏/闪屏/横幅/彩带和音效。`Games::render()` 先 `writeSnapshot()` 提取 `net::Snapshot`，再让 `client::GameView::apply()` 重建只读世界，最后交给 `Renderer` 绘制——本地离线路径和 TCP 客户端共用同一条表现路径（标题在 `menu::*`，结算/失败页在 `src/qml/`）。四个关卡分别使用 `waves/level-01..04.json`，`GameSimulation::Level.enemyLevel` 为 1–4，作为隐藏的敌人成长等级乘在血量/伤害/速度上。

`src/app/shared/Net/` 同时保存服务端 / 客户端共享网络协议层的头文件与实现：`Protocol` 负责 `[u32 length][u16 type][u16 version][payload]` 帧和 TCP 粘包缓冲，`Input / Messages / Snapshot` 负责各类 payload 的 `QDataStream` 序列化；快照每 2 个 tick 才编码一次（30Hz），payload 经 qCompress 压成 `SnapshotCompressed`；客户端解压后重建视图，不再每 60Hz 发送一份未压缩全量快照。收包端每帧只拆一次帧，handler 直接消费已校验的 payload。`src/app/server/Net/Server.*` 是 `QTcpServer`：拥有 headless `games::GameSimulation`，60Hz tick、30Hz 压缩快照广播；`GameServer::openRoom/startMatch/dissolveRoom` 是对本地宿主的显式房间生命周期，单房间 `RoomManager` 保存服务端权威的房间名/容量/房主/对局状态，`RoomList` 的成员列表由实际 welcomed 会话派生。加入房间不需要额外请求消息：Hello/Welcome 即加入当前房间，未知消息类型会被拒绝。`src/app/client/Net/Client.*` 是 `QTcpSocket`：握手、发送输入、接收快照并维护 `client::GameView`。`menu::Match` 默认使用网络路径：单机时在 127.0.0.1 上自动选端口启动内嵌服务端并立即 startMatch，`Mode::Host` 只开房不开局，`--connect host:port` 连远端并进入房间；`--local` 或 `Menu(..., offline=true)` 保留本地直连模式给自动化测试和回退。自定义协议的联机设置内嵌在 `menu::Editor`：顶部三个选项卡决定下方显示台本编辑（单机/主机）或加入主机表单；局域网主机在波次编辑下方多一行端口。

`src/app/server/Shop/` 和 `Arena/` 同构：`Shop` 是全局模块类（进程级单例 `ShopService`），组合 `Score`（全局钱包/本局统计）和 `Inventory`（地雷库存）两个 `namespace shop` 组件，负责 `load()/save()` 与 `buyMine()`；补给站界面是 `menu::Depot`，只展示和调用接口，不直接改数据。

`src/app/server/Loadout/` 是战备全局模块（`LoadoutService`）：从 `loadout.json` 读发射器数值，选择持久化到 `loadout/launcher`；`Player::reset()` 应用伤害/射速/弹速倍率，`Player::shoot()` 应用散射倍率、额外弹丸和穿透。界面是 `menu::Armory`，默认标准发射器保持原弹道。

`src/app/client/Menu/` 和 `Arena/` 同构：`Menu` 只持有进程级的关卡表、当前屏幕和输入路由（`Base`/`Shop`/`Loadout` 都是全局单例，不在 `Menu` 里），十个界面是 `namespace menu` 组件——`Map`（星区地图）、`Match`（默认持有网络 `GameClient` + 内嵌/远端服务端，`--local`/测试时持有这一局的 `Games`；`Mode::Host` 时只开房不开局）、`Room`（服务端权威房间页：房主开始/解散、加入者等待）、`Failure`（QML `FailureOverlay`，C++ 只提供 `FailureView`）、`Main`/`Settings`（QML `MainOverlay`/`SettingsOverlay`，C++ 只提供数据、几何和输入逻辑）、`Result`（QML `ResultOverlay`，C++ 只提供只读 `ResultSummary`）、`Depot`/`Armory`（QML `DepotOverlay`/`ArmoryOverlay`，C++ 只保留购买/装备逻辑）、`Editor`（自定义协议：顶部三个选项卡，单机/局域网主机显示台本编辑，主机追加服务器端口，加入主机显示地址和端口）。组件把玩家意图作为 `Action` 交回 `Menu`，`Menu` 负责开局、进编辑器、进房间和回主菜单；“自定义协议”点开即进编辑界面，逐波编辑台本（1–12 波、四种敌人数量、单 Boss），`ENTER` 按当前选项卡执行：单机本地立即开局、主机启动服务端并进入房间等待 `ENTER` 开始、加入连接远端并显示同一房间。

`src/app/shared/Arena/` 是模块目录：`Arena` 是全局模块类，**私有继承** `Bullets`、`Combatants`、`Entities`、`Events`、`State` 五个 `namespace arena` 组件。私有继承意味着这些组件是 `Arena` 的实现细节：外部只能调用 `Arena` 自己声明的接口（`beginWave/update/draw/placeMine/playerState/bossInfo/clear*/celebrate*/takeFeedback` 等），看不到 `enemies()`、`submit()`、`player()` 这类组件方法。组件之间通过 `Arena/Context.h` 的 `arena::Context` 协作——`Arena` 调用组件时把 `Host&` 和 5 个兄弟组件引用显式注入，组件头文件因此不需要知道 `Arena`；组件 → 模块的反馈（闪屏/震屏）通过 `Arena/Host.h` 的 `arena::Host` 虚接口回调 `Arena::addFeedback()`。`Bullet` 是 `src/include/Bullet.h` 里的全局纯虚接口，具体弹种（目前是 `StandardBullet`）在 `Arena/Bullets/`；发射方只 `submit()` 对象，推进、回收和绘制都由池统一做。`Fighter` 也放在 `src/include/`，`Player`/`Enemy`/实体类型放在各自组件目录，不再各自算组件。`Events` 一个 class 同时处理碰撞与击杀，`State` 一个 class 同时处理 Boss 阶段和玩家失败（积分在 `ShopService` 的 `Score` 组件）。`SelfTest` 是 `Arena` 唯一保留的 friend（测试要直接摆局面）。

每关的波次安排单独放在 `src/json/waves/level-0N.json`（当前 01–04 四关），`levels.json` 的节点用 `waves` 字段登记。

新增的 6 种敌人（数值与光环参数都在 `src/json/enemies.json`）：

- `amplifier`：为范围内友军增伤（子弹与接触都吃加成）；
- `linker`：为范围内友军分担一部分伤害；
- `mender`：持续治疗范围内友军；
- `rusher`：高速突进、攻击欲望高；
- `necromancer`：概率复活范围内阵亡的友军；
- `summoner`：按常规友军阵亡数积累进度（召唤物 mini 不计入），满进度召唤小怪。

台本编辑器底部的敌人库也已包含这 6 种，拖拽或按 `1`–`9`、`0` 加入。

敌人按类型形成阵型：

- 进攻型（drone / mini / rusher）与自毁型（splitter）：直冲玩家、以接触为目标；
- 远程型（spitter / summoner）：保持射程，玩家弹幕逼近时短促侧闪（0.12s，冷却 1.6s）；有防御线时躲到防御者背后；
- 防御型（tank / linker）：站在远程/辅助后排与玩家之间，不主动贴脸；
- 辅助型（amplifier / mender / necromancer）：躲在防御者背后，朝“非自毁型”友军最密集的位置靠拢；
- Boss：独立走位（中距离绕圈 + 定向/环形弹幕）。

敌人实现按 `Enemy` → 进攻 / 远程 / 防御 / 辅助 / 自毁 / Boss 六个虚基类拆分（`Enemies/EnemyTypes.h`），每个具体类型一个 `*Enemy.cpp`。类型身份直接用 RTTI（`arena::EnemyType = std::type_index`）：子类构造时传 `typeid(Self)`，各文件用 `EnemyRegistrar<T>`（CRTP）把 `typeid(T)` 登记到 `Enemy.cpp` 的类型注册表，运行时 `Enemy::create(EnemyType)` 查表构造；`enemies.json` 的字符串键只在 `Menu::standardScript()` / 编辑器等数据边界解析成类型，`games::EnemySpawn` 之后只保存 `EnemyType`，游戏逻辑判断类型用 `typeid` / `dynamic_cast`，不再比较字符串。`Enemy::Archetype` 缓存只用于 AI 筛选友军，避免每帧 RTTI。

`src/lib/` 放不依赖应用层的通用模块：`Math`（向量/几何）和 `Random`（可注入随机数，并提供全局 `RandomService`）。

渲染使用 Qt 的 `QPainter`：所有形状、发光、粒子、文字和 UI 都由代码绘制，不加载外部图片。音效由代码生成 WAV 后通过 `QSoundEffect` 播放。

## 背景音乐

游戏启动后会扫描 **exe 同级的 `music\` 目录**，把里面的音频文件按文件名排序后轮转播放：一首放完自动下一首，播到末尾回到第一首；解码失败的文件会被跳过。

- 打包/交付版的目录是 `dist\NeonCore\music\`，直接把音乐文件丢进去即可；
- 支持的扩展名：`.mp4` `.m4a` `.mp3` `.flac` `.ogg` `.opus` `.wav` `.aac`（MP4 只取音频轨）；
- 想换目录可以设置环境变量 `NEON_MUSIC_DIR`（例如 `set NEON_MUSIC_DIR=D:\Music\NeonCore`）；
- 音量固定为 45%（可在设置页调整），`M` 静音会同时静音背景音乐和音效；
- 播放器只创建一次并常驻：切换界面、开局/结算、音频重试都不会重新起播，只有一首放完才切下一首；
- 排查音频问题可设置 `NEON_AUDIO_LOG=1`，事件会写到 `%TEMP%
eon_core_audio.log`。

## 应用图标

图标也是代码绘制的（`Base::appIcon()` → `Paint::appIcon()`），没有任何外部图片依赖：

- 窗口/任务栏：构造窗口时按 16/24/32/48/64/128/256 多档渲染成 `QIcon`；
- 资源管理器里的 exe 图标：`src/app/client/app.rc` 把 `assets/icon.ico`（多尺寸，由 `assets/icon.png` 转出）嵌入 release 可执行文件；
- 修改图标后重新跑 `--capture` 会生成新的 `build/capture/icon.png`，转成多尺寸 `assets/icon.ico` 再 `make deploy` 即可。

窗口内部固定以 **1280×720** 逻辑尺寸渲染，并按屏幕 DPI 调整窗口物理尺寸，保证在 125%/150%/200% 缩放下仍能完整看到 1280×720 的游戏画面。模块细节见 [`ARCHITECTURE.md`](ARCHITECTURE.md)。

## 自动化检查

先构建 `-DDEBUG` 测试控制台（不会进入 `dist`）：

```powershell
cd cpp
mingw32-make -j12 test
.\build\NeonCoreConsole.exe --smoke-test
```

测试包含：

- 标题、开局、暂停、继续、鼠标点击选择强化
- 清波、强化生效、下一波
- Boss 生成、Boss 死亡记录、最后一波清空后胜利
- 中局 Boss：非最后一波击杀 Boss 不会结束对局，清波后照常进入强化
- 死亡、失败、重新开始
- 自定义模式：点开卡片进入带三个选项卡的编辑界面；单机/局域网主机编辑台本，主机在波次下方填端口，加入主机填地址和端口
- 运行时提交的台本：用一份临时两波台本开局，验证波数与出生队列都来自这份数据
- 完整 8 波自动游玩，直到击败 Boss
- 音频初始化（可选：设置 `NEON_AUDIO_TEST=1`）
- 设置页：齿轮按钮进入、音效/背景音乐音量增减、点击进度条设值、Esc 返回
- 积分：击杀累计到全局钱包、消费扣减、余额不足拒绝、开新局只重置本局统计
- 商店：补给箱按钮进入、购买地雷扣积分并加库存、Esc 返回
- 地雷：右键布置扣库存、敌人接触即引爆并造成击杀
- 星区地图：ENTER 进地图、方向键选择、四个区域可进入、关卡 2/4 波数与敌方等级正确
- 编辑器：底部敌人库拖拽新增列、按住 `+`/`-` 连续快速加减；部署后进入联机设置
- 自定义选项卡：默认单机不占端口、局域网主机端口绑定、加入主机地址/端口、F1/F2/F3 切换
- 战备：标题页入口、发射器装备、玩家伤害/射速随发射器变化
- 敌人等级：升级后血量/伤害提高，战斗界面不显示等级
- 支援敌人：增伤光环、伤害分担、群体治疗、高速突进、概率复活、召唤进度
- 快照：完整场景提取 → QDataStream 序列化 → 反序列化，逐字段相等；玩家 id/颜色/localPlayerId 保留；帧长度/协议版本不匹配被拒绝
- GameView：快照重建后的玩家/敌人/子弹/实体/HUD/强化卡/Boss 条正确，并能用现有 `Renderer` 画出完整一帧
- 单机路径：`GameSimulation -> Snapshot -> GameView -> Renderer` 的视图与模拟状态一致
- 多玩家飞机：两个 slot 生成不同 id/颜色/出生点的飞机，输入只移动发送者的飞机，未知/观战 id 被忽略；多玩家快照序列化后每个客户端按自己的 localPlayerId 选中本机；阵亡等待复活、全灭判负
- TCP 多玩家会话：两个客户端开局前各自拿到 playerId、快照含两架飞机且本机飞机正确、输入隔离、开局后加入的会话观战且输入被忽略、对局中一个玩家掉线只移除其飞机且服务器继续运行
- TCP 回环：真实 `QTcpServer/QTcpSocket` 握手、房间大厅、宿主本地 StartMatch、输入命令、权威模拟位移、快照重建、双客户端广播；TCP StartMatch 被拒绝
- 四客户端房间：4 个客户端开局前握手，玩家 id/会话/颜色/出生点各不相同，人数与容量由服务端派生，claim admin 后开局，四端都收到快照且输入隔离；第 5 个连接被拒绝，空出槽位后加入的会话只观战
- 网络竞态：客户端还在连接或握手时就 startMatch（fallback 槽位接管 / 中途加入者观战）、第二个客户端握手途中解散房间、结果页重试原地回房、房主在客户端结算时离开；各端按服务端状态进入房间或失败页，不崩溃、不挂起
- 延迟网络：测试专用 TCP 延迟代理（双向 80–150 ms，仅 `-DDEBUG` 编译）转发握手、房间状态、开局、输入和快照，并断言延迟真实生效
- 连接超时与重试：接受 TCP 但从不 Welcome 的静默服务器会在有界时间内失败，连续重试每次只开一条连接、不产生重复房间
- 管理员/会话生命周期：未 claim 时 start/return 都被拒绝，非管理员 TCP StartMatch 被拒绝且不改服务端状态，claim 幂等，admin session 在 start/return/重试之间保持稳定，管理员掉线后不会转给其他玩家
- 房间生命周期：主机 ENTER 只进房间不开局、房间人数由服务端广播、加入者看到同一房间、房主 ENTER 本地 startMatch、双方进入对局、解散关闭服务端并让客户端进入失败页
- 失败页：连接不存在的服务器显示“无法连接服务器”、房主解散显示“房间已解散”、`ENTER` 重试与 `ESC` 返回
- 错误路径：端口占用、协议版本不匹配、客户端断线、连接失败/重连调度都不会崩溃
- 内嵌单机：默认 `Menu` 网络路径启动 127.0.0.1 自动端口服务端并渲染一帧；握手晚于 startMatch 时仍能接管并操作自己的飞机
- 输出到当前目录的 `smoke.log`

重新生成离屏截图：

```powershell
.\build\NeonCoreConsole.exe --capture screenshots
```

真实窗口测试（会短暂打开一个窗口，然后自动截图退出）：

```powershell
.\build\NeonCoreConsole.exe --window-test screenshots/window.png
```

`NeonCoreConsole.exe` 和 `src/tests/SelfTest.h` 只存在于 debug/test 构建；`dist\NeonCore` 只交付 release `NeonCore.exe`，不包含测试逻辑。

## 性能

局内渲染拆成两条并行通道：世界层（背景/星空/实体/子弹/粒子）在 `ThreadPool` 工作线程写 `frame_`，辉光层在主线程写 `glow_`，字体统一留在主线程，最后主线程合成。`base::Paint::Pass` 让同一批绘制调用按通道裁剪，不需要复制一套绘制代码。

固定场景基准（`--bench`：300 帧 `render` + 600 帧 `update`，同机交替 A/B）：

| 版本 | render | update |
| --- | --- | --- |
| 优化前 | 10.1–13.2 ms/帧（中位 12.4） | ~0.03 ms/帧 |
| 并行渲染 | **6.6–7.3 ms/帧（中位 7.0，−35% ~ −43%）** | ~0.006 ms/帧 |
| `NEON_SINGLE_THREAD=1` | 12.0–12.4 ms/帧 | ~0.008 ms/帧 |

其他改动：绘制原语去掉逐次 `save/restore`；世界层直接画进最终帧、去掉多余整图；标准弹对象池复用（稳态零堆分配）；敌人 `Archetype` 缓存替代 `dynamic_cast`；`Combatants::draw` 使用线程内排序缓冲。

```powershell
# 基准（debug/test 构建）
.\build\NeonCoreConsole.exe --bench

# 关闭并行 / 指定工作线程数（0–8，默认 min(核数-1, 3)）
$env:NEON_SINGLE_THREAD=1
$env:NEON_RENDER_THREADS=2
```

没有走 GPU：当前是 `QImage` + `QPainter` 光栅管线，GPU 化需要重写为 `QRhi`/`QOpenGLWidget` 渲染器，并且会破坏 offscreen 截图与冒烟测试；多线程通道已经拿到主要收益。

## 说明

- C++ 版使用 Qt 6 的字体渲染，字形和抗锯齿效果与 pygame 版略有差异；布局、配色和逻辑保持一致。
- 中文使用系统微软雅黑/黑体。若字体缺失，可在 `src/app/client/Base/Paint/Paint.cpp` 的 `Paint::initialize()` 中更换字体路径。
- 鼠标指针是全局元素，属于 `Base` 的 `base::Pointer` 组件（`src/app/client/Base/Pointer/`）：Widgets 画面用 `BaseService.pointerDraw()`，QML 结算/失败页用 `Reticle.qml` 镜像同一位置/时钟/可见性；`base::Paint::reticle()` 是 Widgets 画法。窗口内隐藏系统光标，指针移出窗口时恢复系统箭头。
- 第三方运行库：Qt 6（LGPLv3）与 FFmpeg（LGPLv2.1+）。分发时请保留相应的许可证信息，见 [`THIRD_PARTY.txt`](THIRD_PARTY.txt)。

更多画面：[标题](screenshots/title.png) · [星区地图](screenshots/map.png) · [战备](screenshots/armory.png) · [商店](screenshots/shop.png) · [设置](screenshots/settings.png) · [台本编辑器](screenshots/editor.png) · [局域网主机](screenshots/server_setup.png) · [加入主机](screenshots/server_join.png) · [房间（房主）](screenshots/room_host.png) · [房间（加入者）](screenshots/room_join.png) · [多玩家](screenshots/multiplayer.png) · [强化协议](screenshots/upgrade.png) · [Boss](screenshots/boss.png) · [真实窗口](screenshots/window.png)
