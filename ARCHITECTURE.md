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
│  │  ├─ Audio/Music.h/.cpp
│  │  └─ Paint/Paint.h/.cpp
│  ├─ Arena/
│  │  ├─ Arena.h / Arena.cpp
│  │  ├─ Events.h / Events.cpp
│  │  ├─ State.h / State.cpp
│  │  ├─ Bullets/
│  │  │  ├─ Bullets.h/.cpp
│  │  │  └─ StandardBullet.h/.cpp
│  │  ├─ Combatants/
│  │  │  ├─ Combatants.h/.cpp
│  │  │  ├─ Player.h/.cpp
│  │  │  └─ Enemy.h/.cpp
│  │  └─ Entities/
│  │     ├─ Entities.h/.cpp
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
│  │  ├─ Main/Main.h/.cpp         标题页组件
│  │  ├─ Match/Match.h/.cpp       当前对局组件
│  │  ├─ Result/Result.h/.cpp     结算覆盖层组件
│  │  ├─ Editor/Editor.h/.cpp     台本编辑器组件
│  │  └─ Settings/Settings.h/.cpp 音量设置组件
│  ├─ Renderer.h / Renderer.cpp
│  ├─ Pointer.h / Pointer.cpp
│  ├─ app.rc
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
| `Audio` | 程序化 WAV 音效、QSoundEffect 池、设备重试、静音 |
| `Music` | 背景音乐管理：固定目录轮转、设备等待、静音；播放器常驻，不随界面重建 |
| `Paint` | 颜色、字体、文字排版、发光、条形/面板等绘制原语，鼠标准星、齿轮与应用图标 |

`Base::Impl` 持有 `base::Input` 和 `base::Audio`；静态绘制/颜色/字体/准星接口转发给 `base::Paint`。`Base.h` 不暴露组件头。

音频是进程级服务，拆成两个组件：`Audio` 在 `%TEMP%\neon_core_cpp_audio\<pid>` 下生成自己的 WAV（同时开两个实例不会互相覆盖），启动时设备还没就绪的话会在 `play()` 里节流重试初始化；`Music` 是独立的背景音乐管理器，只初始化一次、由主线程的定时器推进曲目，任何音频重试、界面切换、开局/结算都不会重建播放器。`QMediaPlayer` 自带解码线程，`Music` 只负责在主线程上做状态管理。

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

`Menu` 是顶层界面模块入口，结构和 `Arena` 一样：自己只持有进程级的 `Base`、关卡表、当前屏幕和输入路由，把五个界面拆成 `namespace menu` 组件：

| 组件 | 职责 |
| --- | --- |
| `Main` | 标题页：标题、关卡卡、设置入口；选中状态和命中测试 |
| `Match` | 持有当前这一局 `Games`：开局、推进、渲染、局内输入转发 |
| `Result` | 结算覆盖层：只接收一份只读 `Summary`，不读取 `Games` |
| `Editor` | 自定义台本编辑器：持有 `customScript_`，逐波表格编辑 |
| `Settings` | 音效 / 背景音乐音量，读写 `settings.ini` |

`Menu` 自己只负责：

- 屏幕状态机（`Main / Match / Result / Editor / Settings`）和输入路由；
- 把 `Main` / `Editor` / `Settings` 的意图（`StartLevel / OpenEditor / Deploy / Back` 等 `Action`）翻译成开局、进编辑器、回主菜单；
- 开局时把 `Games::Level`（展示信息 + 台本 + 起始波）交给 `Match`；回主菜单时让 `Match` 销毁这一局；
- 给 `Result` 组装 `Summary`（胜负、得分、击杀、波次、时间）；
- 管理关卡表 `levels_`：标准协议与自定义协议的展示信息，自定义协议部署时用 `Editor::script()` 覆盖台本、固定从第 1 波开始；
- 启动时初始化 `Base`（音频/输入/字体）并按 `settings.ini` 恢复音量。

`Games` 一旦构造就由 `Match` 持有，`Menu` 不再直接访问；`Menu` 不参与战斗、波次或强化计算，也不直接画任何屏幕（组件的 `render()` 才画）。

### Arena

`Arena` 是全局模块类，组合 5 个 `namespace arena` 组件：

| 组件 | 职责 |
| --- | --- |
| `Bullets` | 子弹池：持有 `Bullet*`（纯虚接口），统一推进、回收、绘制；按阵营分两组 |
| `Combatants` | 玩家、敌人容器、Boss 指针；推进/绘制战斗者 |
| `Entities` | 传送门、掉落、粒子、飘字、冲击波 |
| `Events` | 碰撞、接触伤害、敌人分离、击杀 |
| `State` | Boss 阶段、Boss 击败、玩家失败、分数/击杀数 |

`Arena` 只编排顺序并汇总 `Feedback`。

### 子弹

`Bullet` 是**全局纯虚接口**（`src/include/Bullet.h`），池不关心具体弹种：

```cpp
class Bullet {
public:
    enum class Standpoint { Player, Enemy };

    virtual void advance(float dt) = 0;                       // 位置推进 + 寿命
    virtual void draw(QPainter &world, QPainter &glow) const = 0;
    virtual bool hit(Arena &arena, arena::Fighter &target) = 0;  // 命中多态；返回 false 结束自己
    virtual bool alive() const = 0;                            // 池据此回收
    virtual const Math::Vec2 &prev() const = 0;                // 扫掠碰撞起点
    virtual const Math::Vec2 &position() const = 0;            // 扫掠碰撞终点
    virtual float radius() const = 0;                          // 碰撞半径
    virtual Standpoint standpoint() const = 0;                 // 阵营 ID
};
```

- 发射方只做 `arena.bullets_.submit(std::make_unique<StandardBullet>(...))`，不自持子弹；
- `arena::Bullets` 按 `standpoint()` 分流到玩家/敌方两组，用 `reserve` + 交换删除（swap-pop）回收，稳态不搬移整段数组；
- 碰撞仍由 `arena::Events` 做几何判定（`segmentCircle(bullet.prev(), bullet.position(), target, targetR + bullet.radius)`），命中后调 `bullet.hit(...)`，伤害、暴击、穿透全部由弹种自己结算；
- 新弹种 = 在 `Arena/Bullets/` 加一个 `Bullet` 实现，提交即可，`Events`/`Renderer` 不用改。

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
- `src/include/Bullet.h`：子弹的全局纯虚接口（`advance / draw / hit / alive / prev / position / radius / standpoint`）。
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
└─ Menu（持有 Base、关卡表，组合 5 个界面组件）
   ├─ menu::Main ─────────► 关卡卡 / 设置入口（数据由 Menu 提供）
   ├─ menu::Match ────────► Games(Level, Base&)   ← 一局一个实例
   │  └─ Games
   │     ├─ Base ─────► base::Input / Audio / Music / Paint（引用，由 Menu 持有）
   │     ├─ Arena ────► arena::Bullets / Combatants / Entities / Events / State
   │     ├─ games::Waves ─────► Arena, Random
   │     ├─ games::Upgrades ──► Arena, Base, Random
   │     ├─ Renderer ─────────► Games/Arena/Waves/Upgrades(只读), Base, Random
   │     └─ Random
   ├─ menu::Result ───────► 只读 Summary
   ├─ menu::Editor ───────► games::Script（自己持有）
   └─ menu::Settings ─────► Base 音量接口 + settings.ini
```

依赖是单向的：

- `GameWindow` 持有 `Menu` 和 `Pointer`，负责把两者的结果合成到窗口上。
- `Pointer` 只依赖 `Base` 的准星画法，不知道游戏业务。
- `Menu` 只做状态机、输入路由和关卡表；界面在 `menu::*` 组件里，`Games` 由 `menu::Match` 持有。
- `Games` 只负责一局内的逻辑，不知道 `Menu`；一局的安排由构造参数进入，之后不再外泄。
- `Games` 只通过 `Arena` 公共接口访问战斗世界。
- `Waves` / `Upgrades` 不知道 UI。
- `Renderer` 只读局内业务状态，不再画标题和结算。
- `menu::Result` 只拿一份 `Summary`，不依赖 `Games`。
- `Base` 不知道游戏业务。
- `lib` 不依赖 `app`。

## 一帧顺序

```text
GameWindow::tick()
├─ Menu::update(dt)
│  ├─ Main:   菜单动画（menu::Main 自己推进时间）
│  ├─ Editor: 输入自处理（menu::Editor），Menu 不更新
│  ├─ Match:  menu::Match → Games::update(dt)
│  │          ├─ Renderer::update(dt)
│  │          ├─ Playing: Arena→Waves→Feedback→胜负/波次判断
│  │          ├─ Upgrade: games::Upgrades 候选卡
│  │          ├─ Paused: 冻结
│  │          └─ Victory/Defeat: 只推进视觉
│  └─ Result: menu::Match 继续 Games::update(dt) 退场表现
├─ Pointer::update(dt)      ← 独立时钟，暂停时也推进
├─ Menu::render()
│  ├─ Main:    menu::Main   绘制菜单
│  ├─ Editor:  menu::Editor 绘制台本表格
│  ├─ Settings: menu::Settings 绘制设置页
│  ├─ Match:  menu::Match → Games::render() → Renderer
│  └─ Result: menu::Match 画面 + menu::Result 结算覆盖层
└─ QWidget::update()

GameWindow::paintEvent()
├─ drawImage(Menu::frame())    ← 当前画面
└─ Pointer::draw()             ← 全局指针叠在最上层
```

## 加机制应该改哪里

### 加强化

只改 `src/app/Games/Upgrades/`：

1. `Upgrades::catalog()` 加定义：`{key, name, tag, color, minValue, maxValue}`，`min= max` 表示固定值（如 +1 穿透），否则每轮在区间内掷一次；
2. `Upgrades::eligible()` 加限制；
3. `Upgrades::describe()/valueLabel()` 加文案（卡面显示的数值就是实际生效值）；
4. `Upgrades::applyEffect()` 加效果，直接用 `offer.value`；
5. 新字段加到 `arena::PlayerState`。

`Renderer` 自动画卡片和右上角的 `offer.valueText`，`Games` 不用改。

### 加 / 改波次

只改 `src/app/Menu/Menu.cpp` 的 `Menu::standardScript()`：每一波列出 `{敌人种类, 数量}`，最后一波带 `boss` 即为 Boss 波。

- 台本是数据，开局时作为 `Games::Level.script` 提交，`Waves` 只负责按它排队出生，不再有内置台本；
- 总波数 = 台本长度，HUD 通过 `Waves::totalWaves()` 读取，结算通过 `Games` 的快照读取；
- `boss` 出现在某一波里，那一波就是 Boss 波（`Waves::isBossWave()`），并且 Boss 永远第一个出场；击杀 Boss 不会结束对局，只有最后一波清空才胜利。

### 加关卡

改 `src/app/Menu/Menu.cpp` 的 `levels_`：

```cpp
{id, name, subtitle, startWave, script}
```

`Menu::startLevel()` 把这条 `Level` 交给 `menu::Match`，由 `Match` 构造 `Games`；`Games` 接收后即开局，并只在内部使用它。

“自定义协议”是同一套机制的动态版本：`levels_` 里只放展示信息，真正的台本在 `menu::Editor` 的 `script_` 里，初始只有一波，玩家在编辑器里定义波数和每波组成；`Menu::startLevel()` 在交给 `Match` 前把它写进 `Level.script`，`startWave` 固定为 1。

### 加一个界面 / 界面字段

每个屏幕都是一个 `menu::*` 组件（`src/app/Menu/<名字>/`）：

1. 组件自己持有 UI 状态、绘制和命中测试，用返回的 `Action` 表达意图；
2. `Menu` 的 `Screen` 枚举加一项，`render()` 里调它的 `render()`，输入路由里处理它的 `Action`；
3. 需要的数据由 `Menu` 组装后传入（如 `Main::Card`、`Result::Summary`），组件不反向依赖 `Menu`。

### 加一个结算字段

只改 `menu::Result::Summary` 和 `menu::Result::render()`；数据由 `Menu::summary()` 从 `menu::Match` 的只读 getter 组装，`Renderer` 不参与结算。

### 加敌人 / 状态 / 事件

- 敌人：`src/app/Arena/Combatants/Enemy.*` + `Waves::buildPlan`。
- 状态：`src/app/Arena/State.*`。
- 碰撞/击杀：`src/app/Arena/Events.*`。
- 实体：`src/app/Arena/Entities/*`。

### 加弹种

1. 在 `src/include/Bullet.h` 之外、`src/app/Arena/Bullets/` 里新建实现（参考 `StandardBullet`）；
2. 实现 `advance`（推进/寿命/出屏）、`draw`（样式）、`hit`（命中结算并返回是否继续存活）；
3. 发射处调用 `arena.bullets_.submit(std::make_unique<YourBullet>(...))`，池会按 `standpoint()` 分流；
4. `Events` 的几何判定、`Arena` 的批量清理和绘制都不需要改。

### 加 IO / 音频 / 绘制原语

- 输入：`src/app/Base/Input/`。
- 音效：`src/app/Base/Audio/Audio.*`；背景音乐：`src/app/Base/Audio/Music.*`。
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
