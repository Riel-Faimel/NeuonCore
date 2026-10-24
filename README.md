# 星核协议 / NEON CORE — C++ / Qt 6 版

这是 Python + pygame 版本的 C++ 复刻版。玩法、数值和状态机对照 `main.py` 移植，没有改动机制：

- 8 个波次、4 种普通敌人、最终 Boss「虚空吞噬者」
- 同样的波次组成、敌人数值、Boss 血量阶段、弹幕与召唤逻辑
- 同样的 9 种强化协议、叠加效果和生命/伤害/射速数值
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
| `P` / `ESC` | 暂停 / 继续 |
| `M` | 静音 / 恢复音效 |
| `1` `2` `3` | 选择强化协议 |
| `R` | 胜利 / 失败后重开 |
| `Q` | 暂停界面返回标题 |

标题页另有：`↑` `↓` / `W` `S` 切换关卡，`←` `→` / `A` `D` 调整“自定义协议”的起始波次（也可点卡片上的 `-` / `+`），`E` 或点卡片上的“编辑台本”进入台本编辑器，`ENTER` 部署；`1` `2` `3` 直接开始对应关卡。

台本编辑器：`↑` `↓` / `W` `S` 选波次，`←` `→` / `A` `D` 选列（DRONE / SPITTER / TANK / SPLITTER / BOSS），`+` / `-` 调数量或开关 Boss，`N` 新增波次、`X` 删除末波（1–12 波，单格 0–20），`ENTER` 部署，`ESC` 返回。编辑结果只存在内存里，重启回到默认台本。

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

## 代码结构

```text
cpp/
├─ src/
│  ├─ app/
│  │  ├─ Arena/
│  │  │  ├─ Arena.h / Arena.cpp   模块入口：组合 4 个组件
│  │  │  ├─ Events.h / Events.cpp 组件：碰撞 + 击杀（单个 class）
│  │  │  ├─ State.h / State.cpp   组件：Boss 阶段 + 分数 + 玩家失败
│  │  │  ├─ Combatants/
│  │  │  │  ├─ Combatants.h/.cpp  组件：玩家、敌人、Boss
│  │  │  │  ├─ Player.h/.cpp      玩家类型
│  │  │  │  └─ Enemy.h/.cpp       敌人类型、Boss
│  │  │  └─ Entities/
│  │  │     ├─ Entities.h/.cpp    组件：实体容器
│  │  │     ├─ Bullet / Portal / Pickup
│  │  │     └─ Particle / FloatingText / Shockwave
│  │  ├─ Base/
│  │  │  ├─ Base.h / Base.cpp     模块入口：组合 Input / Audio / Paint
│  │  │  ├─ Input/Input.h/.cpp    组件：键鼠状态
│  │  │  ├─ Audio/Audio.h/.cpp    组件：程序化 WAV + 音效池
│  │  │  └─ Paint/Paint.h/.cpp    组件：颜色/字体/绘制原语
│  │  ├─ Games/
│  │  │  ├─ Games.h / Games.cpp   模块入口：局内状态机与组件编排
│  │  │  ├─ Waves/Waves.h/.cpp    组件：按提交的台本出生敌人
│  │  │  └─ Upgrades/Upgrades.h/.cpp 组件：强化池、抽取与应用
│  │  ├─ Menu/Menu.h/.cpp          主菜单、关卡选择、对局信息、结算
│  │  ├─ Renderer.h / Renderer.cpp 局内背景、发光、HUD、卡片、暂停
│  │  ├─ Pointer.h / Pointer.cpp  全局鼠标指针（窗口每帧叠加）
│  │  ├─ GameConfig.h             全局尺寸常量
│  │  └─ main.cpp                 QWidget 窗口、输入桥接、release 入口
│  ├─ lib/
│  │  ├─ Math.h / Math.cpp        向量与几何
│  │  └─ Random.h / Random.cpp    可注入随机数
│  ├─ include/
│  │  └─ Fighter.h                Player/Enemy 的公共虚接口
│  └─ tests/
│     └─ SelfTest.h               仅 -DDEBUG 测试构建包含的测试/截图逻辑
├─ build.bat                      调用 mingw32-make -j12 deploy
├─ Makefile                       mingw32-make -j12 并行构建
├─ run.bat
├─ ARCHITECTURE.md                模块边界、依赖和扩展指南
├─ screenshots/
└─ dist/NeonCore/                 交付用 release GUI + 运行库
```

`src/app/Base/` 是模块目录：`Base` 是全局模块类，组合 `Input`、`Audio`、`Paint` 三个 `namespace base` 组件。`Base.h` 仍只暴露 Base 的输入/音频/绘制接口；`Input` 的键鼠状态、`Audio` 的 QSoundEffect/WAV、`Paint` 的 QFontDatabase 细节都在各自组件内。

`src/app/Games/` 是模块目录：`Games` 是全局模块类，组合 `Waves`、`Upgrades` 两个 `namespace games` 组件，并拥有 `Random`、`Arena`、`Renderer`。一局游戏就是一个 `Games` 实例：`Menu` 在开局时把这一局的安排（`Games::Level`，含运行时提交的台本 `games::Script`）交给它，回到主菜单即销毁。没有内置波次表：`Waves` 只按提交的台本生成出生队列，`Upgrades` 只管理强化，`Games` 负责局内状态机和调用顺序。

`src/app/Menu/` 是顶层入口模块：负责主菜单、关卡选择（标准协议 / 核心突袭 / 自定义协议）、台本编辑、对局结果和结算界面。它持有进程级的 `Base` 服务和关卡表，按局构造 `Games` 并把 `Level` 传进去；“自定义协议”支持逐波编辑台本（1–12 波、四种敌人数量、单 Boss），部署时作为 `Level.script` 提交；`Games` 不再负责标题、失败/胜利画面，`Renderer` 不再负责标题和结算。

`src/app/Arena/` 是模块目录：`Arena` 是全局模块类，只组合 `Combatants`、`Entities`、`Events`、`State` 四个 `namespace arena` 组件。`Fighter` 放在 `src/include/`，`Player`/`Enemy`/实体类型放在各自组件目录，不再各自算组件。`Events` 一个 class 同时处理碰撞与击杀，`State` 一个 class 同时处理 Boss 阶段、分数和玩家失败。

`src/lib/` 放不依赖应用层的通用模块：`Math`（向量/几何）和 `Random`（可注入随机数）。

渲染使用 Qt 的 `QPainter`：所有形状、发光、粒子、文字和 UI 都由代码绘制，不加载外部图片。音效由代码生成 WAV 后通过 `QSoundEffect` 播放。

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
- Boss 生成、Boss 死亡、胜利
- 死亡、失败、重新开始
- 自定义关卡：起始波次上下界钳制，以及按所选波次开局
- 运行时提交的台本：用一份临时两波台本开局，验证波数与出生队列都来自这份数据
- 台本编辑器：增删波次、调格子数量、开关 Boss、部署后波数生效、Esc 返回
- 完整 8 波自动游玩，直到击败 Boss
- 音频初始化（可选：设置 `NEON_AUDIO_TEST=1`）
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

更多画面：[标题](screenshots/title.png) · [台本编辑器](screenshots/editor.png) · [强化协议](screenshots/upgrade.png) · [Boss](screenshots/boss.png) · [胜利](screenshots/victory.png) · [真实窗口](screenshots/window.png)
