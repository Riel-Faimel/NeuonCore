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

本机 12 核，推荐用 Makefile 并行构建，再部署到 `dist\NeonCore`：

```bat
mingw32-make -j12
mingw32-make -j12 deploy
```

- `all` / 默认目标：并行编译并链接 `build\NeonCore.exe` 和 `build\NeonCoreConsole.exe`
- `deploy`：构建后复制 exe、Qt DLL、平台/多媒体插件和 FFmpeg 依赖到 `dist\NeonCore`
- `clean`：删除 `build`

`Makefile` 直接调用 `g++`，不依赖 qmake 或 CMake。`build.bat` 保留为不依赖 make 的手动编译/部署备用路径。构建产物：

```text
build\NeonCore.exe          GUI 版本
build\NeonCoreConsole.exe   控制台版本，用于自动化测试
dist\NeonCore\NeonCore.exe  可直接分发运行的版本
dist\NeonCore\NeonCoreConsole.exe  随包分发的自动化测试入口
```

## 代码结构

```text
cpp/
├─ src/
│  ├─ base/
│  │  ├─ Base.h                唯一的 I/O 基础设施接口类
│  │  └─ Base.cpp              输入状态、音频、字体和绘制原语的隐藏实现
│  ├─ Arena/
│  │  ├─ Arena.h / Arena.cpp   模块入口：组合组件、推进/绘制战场
│  │  ├─ Fighter.h              Player/Enemy 的共同虚接口
│  │  ├─ Player.h / Player.cpp 玩家战斗者
│  │  ├─ Enemy.h / Enemy.cpp   敌人战斗者、Boss
│  │  ├─ Bullet.h / Bullet.cpp 子弹实体
│  │  ├─ Portal.h / Portal.cpp 传送门实体
│  │  ├─ Pickup.h / Pickup.cpp 掉落实体
│  │  ├─ Particle.h / Particle.cpp 粒子实体
│  │  ├─ FloatingText.h / FloatingText.cpp 飘字实体
│  │  ├─ Shockwave.h / Shockwave.cpp 冲击波实体
│  │  ├─ Collision.h / Collision.cpp 碰撞事件
│  │  ├─ Kill.h / Kill.cpp     击杀事件
│  │  ├─ BossState.h / BossState.cpp Boss 阶段状态
│  │  └─ Score.h / Score.cpp   分数/击杀状态
│  ├─ Game.h / Game.cpp         状态机与模块编排
│  ├─ Waves.h / Waves.cpp       波次表和出生节奏
│  ├─ Upgrades.h / Upgrades.cpp 强化池、抽取与应用
│  ├─ Renderer.h / Renderer.cpp 背景、发光、HUD、卡片、结算界面
│  ├─ Random.h / Random.cpp     可注入随机数
│  ├─ Math.h / Math.cpp         向量与几何
│  ├─ GameConfig.h              全局尺寸常量
│  └─ main.cpp                  QWidget 窗口、输入桥接、自测与截图
├─ build.bat                    不依赖 make 的备用编译/部署脚本
├─ Makefile                     mingw32-make -j12 并行构建
├─ run.bat
├─ ARCHITECTURE.md              模块边界、依赖和扩展指南
├─ screenshots/
└─ dist/NeonCore/               已打包的 Windows 运行目录
```

`src/base/` 只对外提供一个 `Base.h` / 一个 `Base` 类；其中的键鼠状态、QSoundEffect、WAV 生成、QFontDatabase 等细节都封装在 `Base.cpp` 的私有 `Impl` 里。业务模块只看到输入查询、音效播放和绘制原语。

`src/Arena/` 是模块目录：`Arena` 是全局模块类，`Arena/` 下的 `Fighter` 是 `Player` / `Enemy` 共同的虚接口；`Player`、`Enemy`、`Bullet`、`Portal`、`Pickup`、`Particle`、`Collision`、`Kill`、`BossState`、`Score` 等是 `namespace arena` 中的组件类。组件由 `Arena` 组合，外部模块只通过 `Arena` 的公共接口访问战斗世界。

渲染使用 Qt 的 `QPainter`：所有形状、发光、粒子、文字和 UI 都由代码绘制，不加载外部图片。音效由代码生成 WAV 后通过 `QSoundEffect` 播放。

窗口内部固定以 **1280×720** 逻辑尺寸渲染，并按屏幕 DPI 调整窗口物理尺寸，保证在 125%/150%/200% 缩放下仍能完整看到 1280×720 的游戏画面。模块细节见 [`ARCHITECTURE.md`](ARCHITECTURE.md)。

## 自动化检查

构建后运行控制台版本：

```powershell
cd cpp
.\dist\NeonCore\NeonCoreConsole.exe --smoke-test
```

测试包含：

- 标题、开局、暂停、继续、鼠标点击选择强化
- 清波、强化生效、下一波
- Boss 生成、Boss 死亡、胜利
- 死亡、失败、重新开始
- 完整 8 波自动游玩，直到击败 Boss
- 音频初始化（可选：设置 `NEON_AUDIO_TEST=1`）
- 输出到当前目录的 `smoke.log`

重新生成离屏截图：

```powershell
.\dist\NeonCore\NeonCoreConsole.exe --capture screenshots
```

真实窗口测试（会短暂打开一个窗口，然后自动截图退出）：

```powershell
.\dist\NeonCore\NeonCoreConsole.exe --window-test screenshots/window.png
```

## 说明

- C++ 版使用 Qt 6 的字体渲染，字形和抗锯齿效果与 pygame 版略有差异；布局、配色和逻辑保持一致。
- 中文使用系统微软雅黑/黑体。若字体缺失，可在 `src/base/Base.cpp` 的 `Base::initialize()` 中更换字体路径。
- 第三方运行库：Qt 6（LGPLv3）与 FFmpeg（LGPLv2.1+）。分发时请保留相应的许可证信息，见 [`THIRD_PARTY.txt`](THIRD_PARTY.txt)。

更多画面：[标题](screenshots/title.png) · [强化协议](screenshots/upgrade.png) · [Boss](screenshots/boss.png) · [胜利](screenshots/victory.png) · [真实窗口](screenshots/window.png)
