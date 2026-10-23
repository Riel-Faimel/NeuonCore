# 星核协议 C++ 架构

这份文档只描述 **C++ / Qt 6 版** 的结构。目标是让后续加机制时，知道“这件事应该由谁决定”，而不是继续往中心类里堆分支。

## 设计原则

1. **一个工作只做一次**
   - 波次组成由 `games::Waves` 决定。
   - 强化池、可选性和强化效果由 `games::Upgrades` 决定。
   - 敌人 AI、碰撞、Boss 阶段、分数由 `Arena` 及其组件决定。
   - 画面布局和表现反馈由 `Renderer` 决定。
   - 输入、音频、字体和绘制原语由 `Base` 及其组件提供。
   - `Games` 只负责状态机、组件组合和调用顺序。

2. **模块是全局类，组件是 namespace 中的类**
   - 除 `main` / `WinMain` 外，没有自由函数。
   - 全局模块类：`GameConfig / Math / Random / Base / Arena / Games / Renderer / GameWindow / SelfTest`。
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
│  ├─ Renderer.h / Renderer.cpp
│  ├─ GameConfig.h
│  └─ main.cpp
├─ lib/
│  ├─ Math.h / Math.cpp
│  └─ Random.h / Random.cpp
├─ include/
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
| `Audio` | 程序化 WAV、QSoundEffect 池、静音 |
| `Paint` | 颜色、字体、文字排版、发光、条形/面板等绘制原语 |

`Base::Impl` 持有 `base::Input` 和 `base::Audio`；静态绘制/颜色/字体接口转发给 `base::Paint`。`Base.h` 不暴露组件头。

### Games

`Games` 是全局模块类，组合 2 个 `namespace games` 组件：

| 组件 | 职责 |
| --- | --- |
| `Waves` | 波次表、Boss 波、出生队列、出生间隔 |
| `Upgrades` | 强化定义、可选性、三选一、叠加、效果应用 |

`Games` 还拥有 `Base`、`Arena`、`Renderer`、`Random`，负责：

- Title / Playing / Upgrade / Paused / GameOver / Victory 状态迁移；
- 将输入转发给 `Base` / `Arena`；
- 推进 `Arena` 和 `games::Waves`；
- 波次清空后打开 `games::Upgrades`；
- 把 `Arena::Feedback` 转发给 `Renderer`。

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

- 背景、星空、发光合成；
- 标题、HUD、Boss 血条、波次横幅；
- 强化卡布局与命中测试；
- 暂停、失败、胜利界面；
- 震屏、闪屏、横幅状态。

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
└─ Games
   ├─ Base ─────► base::Input / Audio / Paint
   ├─ Arena ────► arena::Combatants / Entities / Events / State
   ├─ games::Waves ─────► Arena, Random
   ├─ games::Upgrades ──► Arena, Base, Random
   ├─ Renderer ─────────► Games/Arena/Waves/Upgrades(只读), Base, Random
   └─ Random
```

依赖是单向的：

- `Games` 知道业务模块；业务模块不知道 `Games`。
- `Games` 只通过 `Arena` 公共接口访问战斗世界。
- `Waves` / `Upgrades` 不知道 UI。
- `Renderer` 只读业务状态。
- `Base` 不知道游戏业务。
- `lib` 不依赖 `app`。

## 一帧顺序

```text
GameWindow::tick()
├─ Games::update(dt)
│  ├─ Renderer::update(dt, titleActive)
│  ├─ Playing:
│  │  ├─ Arena::update(dt)       // 玩家→传送门→子弹→敌人→接触→掉落→清理→特效
│  │  ├─ games::Waves::update(dt) // 生成传送门
│  │  ├─ Feedback → Renderer
│  │  ├─ 玩家死亡 → GameOver
│  │  ├─ Boss 死亡 → Victory 延迟
│  │  └─ 波次清空 → 0.85s 后 Upgrade
│  ├─ Upgrade: games::Upgrades 候选卡
│  ├─ Paused: 冻结
│  └─ GameOver / Victory: 只推进表现
├─ Games::render()
│  └─ Renderer::render(...)
└─ QWidget::update()
```

## 加机制应该改哪里

### 加强化

只改 `src/app/Games/Upgrades/`：

1. `Upgrades::catalog()` 加定义。
2. `Upgrades::eligible()` 加限制。
3. `Upgrades::applyEffect()` 加效果。
4. 新字段加到 `arena::PlayerState`。

`Renderer` 自动画卡片，`Games` 不用改。

### 加波次

只改 `src/app/Games/Waves/`：

- `buildPlan()` 加组合；
- `TotalWaves` 是唯一的波数来源；
- HUD/结算通过 `Waves::totalWaves()` 读取。

### 加敌人 / 状态 / 事件

- 敌人：`src/app/Arena/Combatants/Enemy.*` + `Waves::buildPlan`。
- 状态：`src/app/Arena/State.*`。
- 碰撞/击杀：`src/app/Arena/Events.*`。
- 实体：`src/app/Arena/Entities/*`。

### 加 IO / 音频 / 绘制原语

- 输入：`src/app/Base/Input/`。
- 音频：`src/app/Base/Audio/`。
- 字体/颜色/绘制：`src/app/Base/Paint/`。
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

- 数值表仍是 C++ 静态表，不是 JSON。
- 敌人/强化/音效仍用字符串 ID。
- `Arena` 对 `games::Upgrades` 开放玩家状态编辑权限；这是必要的模块间 friend。
- 组件之间的 private 访问通过 friend 完成；它们属于同一个模块。
- 没有 ECS。当前 4 个 Arena 组件 + 2 个 Games 组件的粒度更适合这个规模。
