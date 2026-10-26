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

标题页另有：`↑` `↓` / `W` `S` 切换关卡，`ENTER` 开始“标准协议”；选中“自定义协议”时 `ENTER`（或直接点卡片）进入台本编辑器；`1` `2` 直接开始标准协议 / 进入编辑器。

台本编辑器：`↑` `↓` / `W` `S` 选波次，`←` `→` / `A` `D` 选列（DRONE / SPITTER / TANK / SPLITTER / BOSS），`+` / `-` 调数量或开关 Boss，`N` 新增波次、`X` 删除末波（1–12 波，单格 0–20），`ENTER` 部署，`ESC` 返回。编辑结果只存在内存里，重启回到默认台本。

## 设置

主菜单右上角的齿轮按钮进入设置页，可以分别调整 **音效音量** 和 **背景音乐音量**（0–100%）：

- `↑` `↓` 选择行，`←` `→` 调整（每次 5%），也可以直接点/拖进度条；
- `ESC` 返回主菜单；调整立即生效；
- 音量保存到 **exe 同级的 `settings.ini`**（`QSettings` 的 INI 格式），下次启动自动恢复；
- 同一个 `settings.ini` 还保存 `[score] total`（全局积分钱包）和 `[inventory] mines`（地雷库存）。

## 商店

主菜单右上角的补给箱按钮进入商店，目前售卖 **震荡地雷**（120 积分）：

- 点击 / `ENTER` 购买，从全局积分扣款，库存 +1（写入 `settings.ini`）；
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
- `deploy`：只把 release `NeonCore.exe` 和 Qt/FFmpeg 运行时复制到 `dist\NeonCore`
- `clean`：删除 `build`

`Makefile` 直接调用 `g++`，不依赖 qmake 或 CMake。`build.bat` 现在只是 `mingw32-make -j12 deploy` 的包装。构建产物：

```text
build\NeonCore.exe          release GUI，不包含测试逻辑
build\NeonCoreConsole.exe   debug/test 控制台，包含 SelfTest
dist\NeonCore\NeonCore.exe  交付用 release GUI，不包含测试逻辑
```

测试代码位于 `src/tests/SelfTest.h`，只在 `src/app/main.cpp` 的 `#ifdef DEBUG` 分支里包含；`-DDEBUG` 只用于 `test` 目标。

## 数值数据

可调数值都在 `src/json/`，构建时嵌入、**编译期**解析，运行时不读文件也不解析 JSON：

```text
src/json/*.json
  → build/embed_json.exe 生成 build/generated/GameDataJson.h（原样 raw string）
  → src/app/Data/GameData.h 用 constexpr 解析器（src/lib/Json.h）展开成
     data::kPlayer / kEnemies / kCampaign / kWaveTiming / kUpgradeRanges / k*Bullet
  → 游戏代码直接读这些 inline constexpr 常量
```

| 文件 | 内容 |
| --- | --- |
| `player.json` | 玩家血量、移速、伤害、射速、弹速、弹体半径 |
| `enemies.json` | 各敌人血量 / 速度 / 半径 / 接触伤害 / 分数 |
| `waves.json` | 战役每波组成、出怪节奏、波次成长系数 |
| `upgrades.json` | 强化每轮随机区间（`min` / `max`） |
| `bullets.json` | 敌方子弹速度 / 伤害 / 半径 |
| `items.json` | 商店道具（地雷）价格 / 伤害 / 范围 / 引爆延迟 |

- 改完直接 `make`：JSON 会重新嵌入，包含数值表的 TU 自动重编译；格式写错会在编译期由 `static_assert` 报错；
- 解析器限制：字符串不使用转义、数字不使用指数；表项数量上限见 `GameData.h` 的 `Max*` 常量；
- 运行时开销为零：`data::*` 都是 `inline constexpr`，游戏里没有 JSON 解析逻辑；
- 生成的头文件在 `build/generated/`，不进版本库。

## 代码结构

```text
cpp/
├─ src/
│  ├─ app/
│  │  ├─ Arena/
│  │  │  ├─ Arena.h / Arena.cpp   模块入口：私有继承 5 个组件 + 编排
│  │  │  ├─ Context.h             组件之间共享的引用集合（Arena 注入，模块内部）
│  │  │  ├─ Host.h                组件 → 模块的回调接口（虚函数，Arena 实现）
│  │  │  ├─ Events.h / Events.cpp 组件：碰撞 + 击杀（单个 class）
│  │  │  ├─ State.h / State.cpp   组件：Boss 阶段 + 玩家失败
│  │  │  ├─ Bullets/
│  │  │  │  ├─ Bullets.h/.cpp     组件：子弹池（Bullet 纯虚接口）
│  │  │  │  └─ StandardBullet.h/.cpp 标准弹实现
│  │  │  ├─ Combatants/
│  │  │  │  ├─ Combatants.h/.cpp  组件：玩家、敌人、Boss
│  │  │  │  ├─ Player.h/.cpp      玩家类型
│  │  │  │  └─ Enemy.h/.cpp       敌人类型、Boss
│  │  │  └─ Entities/
│  │  │     ├─ Entities.h/.cpp    组件：实体容器
│  │  │     ├─ Mine                   地雷（商店道具）
│  │  │     ├─ Portal / Pickup
│  │  │     └─ Particle / FloatingText / Shockwave
│  │  ├─ Base/
│  │  │  ├─ Base.h / Base.cpp     模块入口：组合 Input / Audio / Music / Paint；含全局单例 BaseService
│  │  │  ├─ Input/Input.h/.cpp    组件：键鼠状态
│  │  │  ├─ Audio/Audio.h/.cpp    组件：程序化 WAV 音效
│  │  │  ├─ Audio/Music.h/.cpp    组件：背景音乐管理器（常驻）
│  │  │  └─ Paint/Paint.h/.cpp    组件：颜色/字体/绘制原语
│  │  ├─ Games/
│  │  │  ├─ Games.h / Games.cpp   模块入口：局内状态机与组件编排
│  │  │  ├─ Waves/Waves.h/.cpp    组件：按提交的台本出生敌人
│  │  │  └─ Upgrades/Upgrades.h/.cpp 组件：强化池、抽取与应用
│  │  ├─ Menu/Menu.h/.cpp          模块入口：屏幕状态机、输入路由、关卡表
│  │  ├─ Menu/Main/Main.h/.cpp     组件：标题页与关卡卡
│  │  ├─ Menu/Match/Match.h/.cpp   组件：当前对局（持有 Games）
│  │  ├─ Menu/Result/Result.h/.cpp 组件：结算覆盖层
│  │  ├─ Menu/Editor/Editor.h/.cpp 组件：自定义台本编辑器
│  │  ├─ Menu/Depot/Depot.h/.cpp   组件：补给站界面（地雷商店）
│  │  ├─ Menu/Settings/Settings.h/.cpp 组件：音量设置
│  │  ├─ Renderer.h / Renderer.cpp 局内背景、发光、HUD、卡片、暂停
│  │  ├─ Shop/Shop.h / Shop.cpp   全局模块：组合 Score / Inventory
│  │  │  ├─ Score/Score.h/.cpp    组件：全局积分（持久化）
│  │  │  └─ Inventory/Inventory.h/.cpp 组件：地雷库存（持久化）
│  │  ├─ Pointer.h / Pointer.cpp  全局鼠标指针（窗口每帧叠加）
│  │  ├─ app.rc                   Windows 资源：嵌入应用图标
│  │  └─ main.cpp                 QWidget 窗口、输入桥接、全局服务初始化、release 入口
│  ├─ Data/GameData.h             编译期数值表（展开自 src/json）
│  ├─ lib/
│  │  ├─ Math.h / Math.cpp        向量与几何
│  │  ├─ Json.h                   constexpr JSON 解析器
│  │  └─ Random.h / Random.cpp    可注入随机数
│  ├─ include/
│  │  ├─ GameConfig.h             全局尺寸常量
│  │  ├─ Fighter.h                Player/Enemy 的公共虚接口
│  │  └─ Bullet.h                 子弹的全局纯虚接口（池只认这一层）
│  ├─ json/
│  │  ├─ player.json              玩家基础数值
│  │  ├─ enemies.json             敌人基础数值
│  │  ├─ waves.json               战役台本 + 出怪节奏
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

`src/app/Base/` 是模块目录：`Base` 是全局模块类，组合 `Input`、`Audio`、`Music`、`Paint` 四个 `namespace base` 组件。`Base` 是**进程级单例**：唯一实例是 `BaseService`（`extern Base BaseService;`，定义在 `Base.cpp`），Menu / Games / Arena / Renderer / Shop 都直接调用 `BaseService`，不再通过构造函数传递 `Base&`。`Base.h` 仍只暴露 Base 的输入/音频/绘制接口；`Input` 的键鼠状态、`Audio` 的 QSoundEffect/WAV、`Music` 的 QMediaPlayer、`Paint` 的 QFontDatabase 细节都在各自组件内。字体由 `main()` 在 `QApplication` 之后调 `Base::initialize()`；音频由 `BaseService.initAudio()` 按需初始化，`main()` 退出前调 `BaseService.shutdown()` 释放 Qt 音频对象。

`src/app/Games/` 是模块目录：`Games` 是全局模块类，组合 `Waves`、`Upgrades` 两个 `namespace games` 组件，并拥有 `Arena`、`Renderer`。一局游戏就是一个 `Games` 实例：`menu::Match` 在开局时把这一局的安排（`Games::Level`，含运行时提交的台本 `games::Script`）交给它，回到主菜单即销毁。没有内置波次表：`Waves` 只按提交的台本生成出生队列，`Upgrades` 只管理强化，`Games` 负责局内状态机和调用顺序。随机数走全局 `RandomService`，不再作为构造参数传入。

积分和库存由进程级单例 `ShopService` 持有（`extern Shop ShopService;`）：`Shop` 组合 `shop::Score`（跨局钱包，写入 `settings.ini` 的 `[score] total`）和 `shop::Inventory`（地雷库存，写入 `[inventory] mines`）。`main()` 启动时调 `ShopService.load()`、退出前调 `ShopService.save()`；`arena::Events::handleKill()` 击杀上报积分；HUD 显示全局钱包，结算显示本局获得（`runScore()`）；`Shop::buyMine()` 负责扣款入库。

`src/app/Shop/` 和 `Arena/` 同构：`Shop` 是全局模块类（进程级单例 `ShopService`），组合 `Score`（全局钱包/本局统计）和 `Inventory`（地雷库存）两个 `namespace shop` 组件，负责 `load()/save()` 与 `buyMine()`；补给站界面是 `menu::Depot`，只展示和调用接口，不直接改数据。

`src/app/Menu/` 和 `Arena/` 同构：`Menu` 只持有进程级的关卡表、当前屏幕和输入路由（`Base`/`Shop` 都是全局单例，不在 `Menu` 里），六个界面是 `namespace menu` 组件——`Main`（标题/关卡卡/设置入口）、`Match`（持有这一局的 `Games`）、`Result`（只读 `Summary` 的结算层）、`Editor`（持有自定义台本）、`Depot`（补给站/商店）、`Settings`（音量 + `settings.ini`）。组件把玩家意图作为 `Action` 交回 `Menu`，`Menu` 负责开局、进编辑器和回主菜单；“自定义协议”点开即进编辑器，逐波编辑台本（1–12 波、四种敌人数量、单 Boss），部署时由 `Menu` 把 `Editor::script()` 写进 `Level.script` 交给 `Match`。

`src/app/Arena/` 是模块目录：`Arena` 是全局模块类，**私有继承** `Bullets`、`Combatants`、`Entities`、`Events`、`State` 五个 `namespace arena` 组件。私有继承意味着这些组件是 `Arena` 的实现细节：外部只能调用 `Arena` 自己声明的接口（`beginWave/update/draw/placeMine/playerState/bossInfo/clear*/celebrate*/takeFeedback` 等），看不到 `enemies()`、`submit()`、`player()` 这类组件方法。组件之间通过 `Arena/Context.h` 的 `arena::Context` 协作——`Arena` 调用组件时把 `Host&` 和 5 个兄弟组件引用显式注入，组件头文件因此不需要知道 `Arena`；组件 → 模块的反馈（闪屏/震屏）通过 `Arena/Host.h` 的 `arena::Host` 虚接口回调 `Arena::addFeedback()`。`Bullet` 是 `src/include/Bullet.h` 里的全局纯虚接口，具体弹种（目前是 `StandardBullet`）在 `Arena/Bullets/`；发射方只 `submit()` 对象，推进、回收和绘制都由池统一做。`Fighter` 也放在 `src/include/`，`Player`/`Enemy`/实体类型放在各自组件目录，不再各自算组件。`Events` 一个 class 同时处理碰撞与击杀，`State` 一个 class 同时处理 Boss 阶段和玩家失败（积分在 `ShopService` 的 `Score` 组件）。`SelfTest` 是 `Arena` 唯一保留的 friend（测试要直接摆局面）。

`src/lib/` 放不依赖应用层的通用模块：`Math`（向量/几何）和 `Random`（可注入随机数，并提供全局 `RandomService`）。

渲染使用 Qt 的 `QPainter`：所有形状、发光、粒子、文字和 UI 都由代码绘制，不加载外部图片。音效由代码生成 WAV 后通过 `QSoundEffect` 播放。

## 背景音乐

游戏启动后会扫描 **exe 同级的 `music\` 目录**，把里面的音频文件按文件名排序后轮转播放：一首放完自动下一首，播到末尾回到第一首；解码失败的文件会被跳过。

- 打包/交付版的目录是 `dist\NeonCore\music\`，直接把音乐文件丢进去即可；
- 支持的扩展名：`.mp4` `.m4a` `.mp3` `.flac` `.ogg` `.opus` `.wav` `.aac`（MP4 只取音频轨）；
- 想换目录可以设置环境变量 `NEON_MUSIC_DIR`（例如 `set NEON_MUSIC_DIR=D:\Music\NeonCore`）；
- 音量固定为 45%（可在设置页调整），`M` 静音会同时静音背景音乐和音效；
- 播放器只创建一次并常驻：切换界面、开局/结算、音频重试都不会重新起播，只有一首放完才切下一首；
- 排查音频问题可设置 `NEON_AUDIO_LOG=1`，事件会写到 `%TEMP%\neon_core_audio.log`。

## 应用图标

图标也是代码绘制的（`Base::appIcon()` → `Paint::appIcon()`），没有任何外部图片依赖：

- 窗口/任务栏：构造窗口时按 16/24/32/48/64/128/256 多档渲染成 `QIcon`；
- 资源管理器里的 exe 图标：`src/app/app.rc` 把 `assets/icon.ico`（多尺寸，由 `assets/icon.png` 转出）嵌入 release 可执行文件；
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
- 自定义模式：点开卡片直接进编辑器，增删波次、调格子数量、开关 Boss、部署后从第 1 波开始、Esc 返回
- 运行时提交的台本：用一份临时两波台本开局，验证波数与出生队列都来自这份数据
- 完整 8 波自动游玩，直到击败 Boss
- 音频初始化（可选：设置 `NEON_AUDIO_TEST=1`）
- 设置页：齿轮按钮进入、音效/背景音乐音量增减、点击进度条设值、Esc 返回
- 积分：击杀累计到全局钱包、消费扣减、余额不足拒绝、开新局只重置本局统计
- 商店：补给箱按钮进入、购买地雷扣积分并加库存、Esc 返回
- 地雷：右键布置扣库存、敌人接触即引爆并造成击杀
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

## 说明

- C++ 版使用 Qt 6 的字体渲染，字形和抗锯齿效果与 pygame 版略有差异；布局、配色和逻辑保持一致。
- 中文使用系统微软雅黑/黑体。若字体缺失，可在 `src/app/Base/Paint/Paint.cpp` 的 `Paint::initialize()` 中更换字体路径。
- 鼠标指针是全局元素（`src/app/Pointer.*`）：窗口每帧把它叠在画面最上层，`Base::reticle()` 是唯一画法，所有界面共用同一个指针和同一个时钟（局内暂停时外弧也照常旋转）；窗口内隐藏系统光标，指针移出窗口时恢复系统箭头。
- 第三方运行库：Qt 6（LGPLv3）与 FFmpeg（LGPLv2.1+）。分发时请保留相应的许可证信息，见 [`THIRD_PARTY.txt`](THIRD_PARTY.txt)。

更多画面：[标题](screenshots/title.png) · [商店](screenshots/shop.png) · [设置](screenshots/settings.png) · [台本编辑器](screenshots/editor.png) · [强化协议](screenshots/upgrade.png) · [Boss](screenshots/boss.png) · [胜利](screenshots/victory.png) · [真实窗口](screenshots/window.png)
