# 星核协议 C++ 架构

这份文档只描述 **C++ / Qt 6 版** 的结构。目标是让后续加机制时，知道“这件事应该由谁决定”，而不是继续往中心类里堆分支。

## 设计原则

1. **一个工作只做一次**
   - 波次台本由 `Menu` 定义成数据（`games::Script`），开局时提交给 `Games`；`games::Waves` 只按台本生成出生队列。
   - 强化池、可选性和强化效果由 `games::Upgrades` 决定。
   - 敌人 AI、碰撞、Boss 阶段、分数由 `Arena` 及其组件决定。
   - 画面布局和局内表现反馈由 `Renderer` 决定。
   - 主菜单、关卡选择和对局结算由 `Menu` 决定。
   - 输入、音频、字体和绘制原语由 `Base` 及其组件提供。
   - `Games` 只负责一局游戏的局内状态机、组件组合和调用顺序。

2. **模块是全局类，组件是 namespace 中的类**
   - 除 `main` / `WinMain` 外，没有自由函数。
   - 全局模块类：`GameConfig / Math / Random / Base / Arena / Games / Menu / Renderer / Pointer / GameWindow / SelfTest`。
   - `Base` 的组件在 `namespace base`：`Input / Audio / Paint`。
   - `Games` 的组件在 `namespace games`：`Waves / Upgrades`。
   - `Arena` 的组件在 `namespace arena`：`Combatants / Entities / Events / State`。
   - 组件由模块组合；外部模块只依赖模块的公共接口。

3. **每个模块回答三个问题**
   - 它从哪里获得工具？
   - 它自己封装哪些逻辑？
   - 它对外提供哪些工具？

## 目录

```text
cpp/src/
├─ app/
│  ├─ Base/
│  │  ├─ Base.h / Base.cpp
│  │  ├─ Input/Input.h/.cpp
│  │  ├─ Audio/Audio.h/.cpp
│  │  └─ Paint/Paint.h/.cpp
│  ├─ Arena/
│  │  ├─ Arena.h / Arena.cpp
│  │  ├─ Events.h / Events.cpp
│  │  ├─ State.h / State.cpp
│  │  ├─ Combatants/
│  │  │  ├─ Combatants.h/.cpp
│  │  │  ├─ Player.h/.cpp
│  │  │  └─ Enemy.h/.cpp
│  │  └─ Entities/
│  │     ├─ Entities.h/.cpp
│  │     ├─ Bullet.h/.cpp
│  │     ├─ Portal.h/.cpp
│  │     ├─ Pickup.h/.cpp
│  │     ├─ Particle.h/.cpp
│  │     ├─ FloatingText.h/.cpp
│  │     └─ Shockwave.h/.cpp
│  ├─ Games/
│  │  ├─ Games.h / Games.cpp
│  │  ├─ Waves/Waves.h/.cpp
│  │  └─ Upgrades/Upgrades.h/.cpp
│  ├─ Menu/Menu.h / Menu.cpp
│  ├─ Renderer.h / Renderer.cpp
│  ├─ Pointer.h / Pointer.cpp
│  └─ main.cpp
├─ lib/
│  ├─ Math.h / Math.cpp
│  └─ Random.h / Random.cpp
├─ include/
│  ├─ GameConfig.h
│  └─ Fighter.h
└─ tests/
   └─ SelfTest.h
```

## 模块与组件

### Base

`Base` 是全局模块类，组合 3 个 `namespace base` 组件：

| 组件 | 职责 |
| --- | --- |
| `Input` | 键鼠状态、移动轴、射击状态 |
| `Audio` | 程序化 WAV 音效、QSoundEffect 池、MP4 背景音乐轮转、静音 |
| `Paint` | 颜色、字体、文字排版、发光、条形/面板等绘制原语，以及鼠标准星 |

`Base::Impl` 持有 `base::Input` 和 `base::Audio`；静态绘制/颜色/字体/准星接口转发给 `base::Paint`。`Base.h` 不暴露组件头。

音频是进程级服务：`Audio` 在 `%TEMP%\neon_core_cpp_audio\<pid>` 下生成自己的 WAV（同时开两个实例不会互相覆盖），启动时设备还没就绪的话会在 `play()` 里节流重试初始化。背景音乐交给 `QMediaPlayer`：扫描 exe 同级的 `music\` 目录（可用 `NEON_MUSIC_DIR` 覆盖），按文件名轮转播放，`M` 静音同时作用于音乐和音效。

指针是全局元素，不属于任何一局、也不属于任何画面：`Pointer` 持有指针位置和独立时钟，`GameWindow` 每帧在 `paintEvent` 里把它叠在 `Menu::frame()` 之上。所以主菜单、局内、强化、暂停、结算共用同一个指针，暂停时外弧也照常旋转。准星的画法只有一份，在 `base::Paint::reticle()`；窗口内隐藏系统光标，指针离开窗口时恢复系统箭头。

### Games

`Games` 是全局模块类，组合 2 个 `namespace games` 组件：

| 组件 | 职责 |
| --- | --- |
| `Waves` | 按运行时提交的台本生成出生队列、Boss 波判定、出生间隔 |
| `Upgrades` | 强化定义、可选性、三选一、叠加、效果应用 |

`Games` 还拥有 `Random`、`Arena`、`Renderer`（`Base` 由 `Menu` 持有，以引用传入），负责：

- 一局内的 `Playing / Upgrade / Paused / Victory / Defeat` 状态；
- 将输入转发给 `Base` / `Arena`；
- 推进 `Arena` 和 `games::Waves`；
- 波次清空后打开 `games::Upgrades`；
- 把 `Arena::Feedback` 转发给 `Renderer`；
- 把胜负结果交给 `Menu`，自己不再画标题或结算。

一局的结束条件只由台本决定：清空最后一波即胜利。`Boss` 只是一个带阶段的强敌，击杀它不会结束对局，只触发表现反馈（目标摧毁横幅、震屏、音效），所以它可以被放在任意一波。

### Menu

`Menu` 是顶层界面模块，持有进程级的 `Base`、关卡表，以及当前这一局的 `Games`，负责：

- 主菜单和关卡选择；
- 台本编辑器（`Screen::Editor`）：逐波编辑波数与四种敌人数量、设置唯一 Boss；编辑结果只存在 `customScript_` 里，部署时作为 `Games::Level.script` 提交；
- 定义每个关卡的安排：展示信息、起始波次，以及这一局的台本（`games::Script`，标准协议从第 1 波开始，核心突袭从最后一波开始，自定义协议由菜单上的 `-` / `+` 或 `←` / `→` 选 1–12 波），开局时把 `Games::Level` 传给 `Games` 构造；
- 回到主菜单或重新部署时销毁旧的 `Games`，下一局重新构造；
- 把主菜单/局内/结算输入路由给 `Games`；
- 在对局结束后读取 `Games` 的得分、击杀、波次、时间；
- 绘制失败/胜利结算界面和“重新部署/返回主菜单”操作。

`Games` 一旦构造就接管这一局的安排，不再对外暴露；`Menu` 只保留展示用的关卡表。
`Menu` 不参与战斗、波次或强化计算。

### Arena

`Arena` 是全局模块类，组合 4 个 `namespace arena` 组件：

| 组件 | 职责 |
| --- | --- |
| `Combatants` | 玩家、敌人容器、Boss 指针；推进/绘制战斗者 |
| `Entities` | 子弹、传送门、掉落、粒子、飘字、冲击波 |
| `Events` | 碰撞、接触伤害、敌人分离、击杀 |
| `State` | Boss 阶段、Boss 击败、玩家失败、分数/击杀数 |

`Arena` 只编排顺序并汇总 `Feedback`。

### Renderer

只读 `Games` / `Arena` / `games::Waves` / `games::Upgrades` / `Base` / `Random`，负责：

- 局内背景、星空、发光合成；
- 局内 HUD、Boss 血条、波次横幅；
- 强化卡布局与命中测试；
- 暂停界面；
- 震屏、闪屏、横幅状态。

标题、失败/胜利结算、对局统计不在 Renderer 中，由 `Menu` 绘制。

### lib / include / tests

- `src/lib/Math.h/.cpp`：向量、clamp、ease、线段圆碰撞。
- `src/lib/Random.h/.cpp`：可注入随机引擎。
- `src/include/Fighter.h`：Player / Enemy 的共同虚接口。
- `src/tests/SelfTest.h`：只在 `-DDEBUG` 构建中由 `main.cpp` 包含。

## Fighter 接口

`Player` / `Enemy` 都实现 `arena::Fighter`：

```cpp
namespace arena {

enum class DamageSource { Bullet, Shock, Contact };
struct Damage {
    float amount = 0.0f;
    bool critical = false;
    DamageSource source = DamageSource::Bullet;
};

class Fighter {
public:
    virtual ~Fighter() = default;
    virtual void update(Arena &arena, float dt) = 0;
    virtual void shoot(Arena &arena) = 0;
    virtual void takeDamage(Arena &arena, const Damage &damage) = 0;
    virtual void die(Arena &arena) = 0;
    virtual void draw(QPainter &world, QPainter &glow, const Arena &arena) const = 0;

    virtual bool isAlive() const = 0;
    virtual const Math::Vec2 &position() const = 0;
    virtual float collisionRadius() const = 0;
    virtual int id() const = 0;
    virtual float contactDamage() const = 0;
};

}
```

接口只放共同动作和只读查询，不把 `PlayerState` / `Enemy` 字段塞进基类。`Enemy::fireAimed` 是 private helper，不进入接口。

## 依赖方向

```text
GameWindow
├─ Pointer ────────────► Base（准星画法）
└─ Menu（持有 Base、关卡表，按局构造 Games）
   └─ Games(Level, Base&)   ← 一局一个实例
      ├─ Base ─────► base::Input / Audio / Paint（引用，由 Menu 持有）
      ├─ Arena ────► arena::Combatants / Entities / Events / State
      ├─ games::Waves ─────► Arena, Random
      ├─ games::Upgrades ──► Arena, Base, Random
      ├─ Renderer ─────────► Games/Arena/Waves/Upgrades(只读), Base, Random
      └─ Random
```

依赖是单向的：

- `GameWindow` 持有 `Menu` 和 `Pointer`，负责把两者的结果合成到窗口上。
- `Pointer` 只依赖 `Base` 的准星画法，不知道游戏业务。
- `Menu` 持有 `Base`、关卡表和一局的 `Games`，负责主菜单、关卡选择和结算。
- `Games` 只负责一局内的逻辑，不知道 `Menu`；一局的安排由构造参数进入，之后不再外泄。
- `Games` 只通过 `Arena` 公共接口访问战斗世界。
- `Waves` / `Upgrades` 不知道 UI。
- `Renderer` 只读局内业务状态，不再画标题和结算。
- `Base` 不知道游戏业务。
- `lib` 不依赖 `app`。

## 一帧顺序

```text
GameWindow::tick()
├─ Menu::update(dt)
│  ├─ Main:   菜单动画
│  ├─ Editor: 台本编辑（输入自处理，只推进菜单动画）
│  ├─ Match:  Games::update(dt)
│  │          ├─ Renderer::update(dt)
│  │          ├─ Playing: Arena→Waves→Feedback→胜负/波次判断
│  │          ├─ Upgrade: games::Upgrades 候选卡
│  │          ├─ Paused: 冻结
│  │          └─ Victory/Defeat: 只推进视觉
│  └─ Result: Games::update(dt) 继续退场表现
├─ Pointer::update(dt)      ← 独立时钟，暂停时也推进
├─ Menu::render()
│  ├─ Main:   绘制菜单
│  ├─ Editor: 绘制台本表格
│  ├─ Match:  Games::render() → Renderer
│  └─ Result: Games::render() + Menu 结算覆盖层
└─ QWidget::update()

GameWindow::paintEvent()
├─ drawImage(Menu::frame())    ← 当前画面
└─ Pointer::draw()             ← 全局指针叠在最上层
```

## 加机制应该改哪里

### 加强化

只改 `src/app/Games/Upgrades/`：

1. `Upgrades::catalog()` 加定义。
2. `Upgrades::eligible()` 加限制。
3. `Upgrades::applyEffect()` 加效果。
4. 新字段加到 `arena::PlayerState`。

`Renderer` 自动画卡片，`Games` 不用改。

### 加 / 改波次

只改 `src/app/Menu/Menu.cpp` 的 `Menu::standardScript()`：每一波列出 `{敌人种类, 数量}`，最后一波带 `boss` 即为 Boss 波。

- 台本是数据，开局时作为 `Games::Level.script` 提交，`Waves` 只负责按它排队出生，不再有内置台本；
- 总波数 = 台本长度，HUD 通过 `Waves::totalWaves()` 读取，结算通过 `Games` 的快照读取；
- `boss` 出现在某一波里，那一波就是 Boss 波（`Waves::isBossWave()`），并且 Boss 永远第一个出场；击杀 Boss 不会结束对局，只有最后一波清空才胜利。

### 加关卡

改 `src/app/Menu/Menu.cpp` 的 `levels_`：

```cpp
{id, name, subtitle, startWave}
```

`Menu` 负责构造 `Games`，把这条 `Level` 作为构造参数传进去；`Games` 接收后即开局，并只在内部使用它。

“自定义协议”是同一套机制的动态版本：`levels_` 里放固定条目，`customWave_` 保存玩家选的起始波次，`Menu::startLevel()` 在构造前把它写进 `Level.startWave`。可选范围来自 `Games::waveCount()`，不在 `Menu` 里重复写死波数。

### 加一个结算字段

只改 `Menu::drawResult()`；数据通过 `Games` 的只读 getter 获取。Renderer 不参与结算。

### 加敌人 / 状态 / 事件

- 敌人：`src/app/Arena/Combatants/Enemy.*` + `Waves::buildPlan`。
- 状态：`src/app/Arena/State.*`。
- 碰撞/击杀：`src/app/Arena/Events.*`。
- 实体：`src/app/Arena/Entities/*`。

### 加 IO / 音频 / 绘制原语

- 输入：`src/app/Base/Input/`。
- 音频：`src/app/Base/Audio/`。
- 字体/颜色/绘制：`src/app/Base/Paint/`。
- 鼠标指针：`src/app/Pointer.*`（位置 + 时钟 + 叠加时机）；准星画法在 `base::Paint::reticle()`。
- `Base` 只保留转发接口。

## 测试与交付

测试代码在 `src/tests/SelfTest.h`，只在 debug/test 构建中由 `main.cpp` 包含：

```cpp
#ifdef DEBUG
#include "../tests/SelfTest.h"
#endif
```

- `mingw32-make -j12` → release `build/NeonCore.exe`，不编译测试。
- `mingw32-make -j12 test` → `-DDEBUG` 测试控制台。
- `mingw32-make -j12 deploy` → 只复制 release GUI 到 `dist/NeonCore`。
- `dist/NeonCore` 不包含测试逻辑。

## 已知边界

- 台本是运行时提交的 `games::Script` 数据，但默认战役仍在 `Menu` 里用 C++ 写（还没有外部 JSON/文件格式）。
- 自定义台本编辑器只改内存里的 `customScript_`，没有存档；重启回到默认战役台本。
- 敌人/强化/音效仍用字符串 ID。
- 一局一个 `Games`：`Base`（输入/音频/字体）是进程级服务，由 `Menu` 持有，否则切局会重复初始化音频。
- 一局的安排只经构造参数进入 `Games`；`Games` 不提供任何读回安排的接口，`Menu` 只保留展示用的关卡表。
- `Arena` 对 `games::Upgrades` 开放玩家状态编辑权限；这是必要的模块间 friend。
- 组件之间的 private 访问通过 friend 完成；它们属于同一个模块。
- 没有 ECS。当前 4 个 Arena 组件 + 2 个 Games 组件的粒度更适合这个规模。
- `Menu` 只做界面/关卡创建，不读取 `Arena` 内部实体；它通过 `Games` 的 getter 获取对局信息。
