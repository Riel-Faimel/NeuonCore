# 星核协议 C++ 架构

这份文档只描述 **C++ / Qt 6 版** 的结构。目标是让后续加机制时，知道“这件事应该由谁决定”，而不是在 `Game` 里继续堆分支。

## 设计原则

1. **一个工作只做一次**
   - 波次组成只由 `Waves` 决定。
   - 强化池、可选性和强化效果只由 `Upgrades` 决定。
   - 敌人 AI、碰撞、Boss 阶段、分数只由 `Arena` 决定。
   - 画面布局和表现反馈只由 `Renderer` 决定。
   - 输入、音频、字体和绘制原语只由 `Base` 保存和提供。
   - `Game` 只负责状态机与调用顺序，不负责具体战斗或 UI 细节。

2. **模块是全局单体类，组件是 namespace 中的单体类**
   - 除 `main` / `WinMain` 外，没有自由函数。
   - 模块类在全局命名空间：`GameConfig / Math / Random / Base / Arena / Waves / Upgrades / Renderer / Game / GameWindow / SelfTest`。
   - `Arena` 的组件在 `namespace arena`：`Fighter / Player / Enemy / Bullet / Portal / Pickup / Particle / FloatingText / Shockwave / Collision / Kill / BossState / Score`。
   - 组件由模块组合，模块对全局负责；外部模块不直接持有或调用 `namespace arena` 的组件。
   - `src/base/` 只对外提供一个头文件 `Base.h` 和一个类 `Base`；`Base.cpp` 里的 `Base::Impl` 藏住键盘状态、QSoundEffect、WAV 生成、字体加载等细节。

3. **每个模块回答三个问题**
   - 它从哪里获得工具？
   - 它自己封装哪些逻辑？
   - 它对外提供哪些工具？

4. **Arena 内部按类型拆成组件**
   - 战斗者、实体、事件、战斗状态各自是 `namespace arena` 中的类。
   - `Arena` 只做组合与顺序编排；具体行为落在组件里。

## 模块总览

| 模块 | 获得工具 | 封装职责 | 对外暴露 |
| --- | --- | --- | --- |
| `GameConfig` | 无 | 屏幕、战场边界、帧率常量 | 只读常量 |
| `Math` | 无 | `Vec2`、clamp、ease、线段圆碰撞 | 向量/几何静态工具 |
| `Random` | seed | 一个可注入的随机引擎 | `real`、`integer`、`shuffle` |
| `Base` | Qt 事件 / QPainter / QFont / Qt Multimedia | 键鼠状态、程序化 WAV 与音效池、字体加载、颜色/文字/发光/条形/面板等绘制原语 | 输入查询、`playSound`/`toggleMute`、静态字体/颜色/绘制工具 |
| `Arena` | `Random`、`Base`、`namespace arena` 组件 | 组合玩家、敌人、Boss、子弹、传送门、掉落、粒子、碰撞、击杀、Boss 阶段和分数 | `update`、`draw`、`spawnPortal`、玩家状态、Boss 信息、反馈 |
| `Waves` | `Random`、`Arena` | 波次组成、Boss 波、出生队列、间隔 | `start`、`update`、当前/总波数、剩余威胁、清空判断 |
| `Upgrades` | `Random`、`Arena`、`Base` | 强化定义、上限、随机三选一、效果应用、等级计数 | `open`、`apply`、候选卡、等级查询 |
| `Renderer` | `Random`、`Base`、各模块只读状态 | 背景/星空/发光合成、标题、HUD、Boss 血条、强化卡、暂停/结算、震屏/闪屏/横幅 | `update`、`render`、`frame`、`shake`、`flash`、`showBanner`、卡片命中 |
| `Game` | 上述模块 | Title/Playing/Upgrade/Paused/GameOver/Victory 状态迁移、调用顺序、通关计时 | `update`、`render`、输入转发、`state`、`runTime`、`frame` |
| `GameWindow` | `Game`、Qt QWidget/QTimer | 真实窗口、DPI、把 `Game::frame()` 画到窗口 | Qt 窗口生命周期 |
| `SelfTest` | 所有模块（测试用途） | 自动测试、离屏截图、真实窗口测试 | 测试入口 |

## 依赖方向

```text
main.cpp
└─ GameWindow ──────────────► Game
                              ├─ Random
                              ├─ Base        I/O：输入 / 音频 / 字体 / 绘制
                              ├─ Arena ◄────────┐
                              ├─ Waves ─────────┤
                              ├─ Upgrades ──────┘
                              └─ Renderer ──────► Base / Random

Arena ──────► Random, Base
Waves ──────► Random, Arena
Upgrades ───► Random, Arena, Base
Renderer ───► Game(只读), Arena(只读), Waves(只读), Upgrades(只读), Base, Random
```

依赖是单向的：

- `Base` 不知道游戏业务，只负责输入、声音和绘制原语。
- `Arena` 不知道 `Game` 和 `Renderer`。
- `Waves` 不知道 UI。
- `Upgrades` 不知道 UI；`Renderer` 负责画候选卡。
- `Renderer` 只读业务状态，不修改战斗。
- `Game` 不直接接触敌人类、子弹类、卡牌布局或绘制原语。

## Arena 组件

`src/Arena/` 是模块目录。`Arena` 是全局模块类，下面这些是 `namespace arena` 中的组件：

| 组件 | 文件 | 职责 |
| --- | --- | --- |
| `Fighter` | `Fighter.h` | Player / Enemy 的共同抽象接口；只声明动作和只读查询 |
| `Player` / `PlayerState` | `Player.h/.cpp` | 玩家移动、瞄准、冲刺、射击、受伤、玩家绘制 |
| `Enemy` / `EnemyStats` | `Enemy.h/.cpp` | 单只敌人的属性、AI、开火、受击、敌人绘制（Boss 也是 kind） |
| `Bullet` | `Bullet.h/.cpp` | 子弹位置、寿命、移动、拖尾绘制 |
| `Portal` | `Portal.h/.cpp` | 出生倒计时、生成敌人、传送门绘制 |
| `Pickup` | `Pickup.h/.cpp` | 掉落吸附、治疗、治疗飘字 |
| `Particle` | `Particle.h/.cpp` | 粒子运动和发光绘制 |
| `FloatingText` | `FloatingText.h/.cpp` | 伤害/治疗飘字 |
| `Shockwave` | `Shockwave.h/.cpp` | 冲击波扩散绘制 |
| `Collision` | `Collision.h/.cpp` | 玩家子弹命中、敌方子弹命中、敌人分离、接触伤害 |
| `Kill` | `Kill.h/.cpp` | 击杀奖励、分裂、掉落、Boss 击败结算 |
| `BossState` | `BossState.h/.cpp` | Boss 二阶段/三阶段、是否被击败 |
| `Score` | `Score.h/.cpp` | 总分、击杀数 |

### Fighter 接口

`arena::Fighter` 是 `Player` 和 `Enemy` 的共同抽象，不是带共享字段的实体基类。

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

设计取舍：

- **`shoot` 是共同动作，`fireAimed` 不是。**
  `Player::shoot` 发射玩家子弹；`Enemy::shoot` 执行这只敌人的标准攻击。Boss 的多发瞄准、Spitter 的抖动射击仍然由 `Enemy` 内部的 private `fireAimed` / 分支实现，不把 `count/spread/bulletSpeed` 这些敌人专用参数塞进接口。
- **`takeDamage` 用 `Damage` 值对象。**
  Player 只需要 `amount`；Enemy 还需要 `critical`（是否显示暴击飘字）和 `source`（子弹命中音效）。以前是一个 `float + bool + QString` 的长参数表，现在调用方只传一个 `Damage`。
- **`draw` 带 `const Arena&`。**
  Enemy 需要读取玩家位置来决定朝向；Player 忽略这个参数。这样两边共用同一个签名，不需要通过全局状态取玩家位置。
- **只放只读访问器，不放共享字段。**
  `PlayerState` 有升级、冲刺、暴击、多发等字段；`Enemy` 有 AI、攻击计时、Boss 标记、分数等字段。强行抽一个公共 `FighterState` 只会制造一个臃肿基类，并把 `protected` 数据泄露给两边。共同点通过 `position / collisionRadius / isAlive` 查询，差异留在各自类内。
- **不给所有字段加 virtual setter。**
  外部读操作走接口；Arena 内部需要写操作时，通过组合它的组件直接完成。
- **`die` 是动作，不是删除。**
  `Player::die` 负责玩家死亡表现和 `playerDefeated_`；`Enemy::die` 只把死亡交给 `Kill` 事件处理。两者都通过同一个虚入口被调用。
- **Arena 不存 `vector<Fighter>` 取代 `vector<Enemy>`。**
  `Kill`、`BossState` 需要敌人的 `score/kind/boss/wave`，这些不是 Fighter 的共性。Arena 保留具体的 Player/Enemy 所有权，只在共同操作上使用 `Fighter&` 或 `const Fighter*`（更新、绘制、碰撞伤害），不做 `dynamic_cast`。

组合关系：

```text
Arena
├─ arena::Player player_
├─ vector<arena::Enemy> enemies_
├─ vector<arena::Bullet> playerBullets_ / enemyBullets_
├─ vector<arena::Portal> / Pickup / Particle / FloatingText / Shockwave
├─ arena::Collision collision_
├─ arena::Kill kills_
├─ arena::BossState bossState_
└─ arena::Score score_
```

`Arena` 负责：
- 何时推进哪个组件；
- 组件之间的顺序（玩家→传送门→玩家子弹→敌方子弹→敌人→接触→掉落→清理→特效）；
- 向 Game/Renderer 暴露只读快照；
- 把组件产生的表现结果汇总成 `Feedback`。

组件负责自己的规则：
- `Enemy::takeDamage` 只处理受击与触发 Boss 阶段检查；
- `Kill::handle` 只处理死亡后的奖励；
- `Collision` 只处理碰撞，不处理死亡奖励；
- `BossState` 只处理 Boss 阶段和击败标志；
- `Score` 只处理分数与击杀计数。

因此加“击杀掉金币”“Boss 第四阶段”“新碰撞类型”时，不需要把逻辑塞回 `Arena` 的 update 大函数。

## 一帧的顺序

```text
GameWindow::tick()
├─ 计算 dt（上限 1/30）
├─ Game::update(dt)
│  ├─ Renderer::update(dt, titleActive)   // 星空、闪烁、震屏、横幅计时
│  ├─ Title:      无游戏逻辑
│  ├─ Playing:
│  │  ├─ Arena::update(dt)                  // 玩家→传送门→子弹→敌人→接触→掉落→清理
│  │  ├─ Waves::update(dt)                // 从队列生成新的传送门
│  │  ├─ Game 消费 Arena 的 Feedback       // 闪屏/震屏
│  │  ├─ 玩家死亡 → GameOver
│  │  ├─ Boss 死亡 → Victory 延迟
│  │  └─ 波次清空 → 0.85s 后进入 Upgrade
│  ├─ Upgrade: Upgrades 候选卡已就绪
│  ├─ Paused: 冻结所有更新
│  ├─ GameOver / Victory: 只推进表现和 Arena 视觉
│  └─ 胜利延迟到点 → Victory
├─ Game::render()
│  └─ Renderer::render(game, arena, waves, upgrades)
└─ QWidget::update() → paintEvent 把 frame_ 画到窗口
```

### 状态机

```text
Title
  └─ Enter / 点击 → Playing

Playing
  ├─ ESC / P → Paused
  ├─ 波次清空 → Upgrade
  ├─ 玩家 HP=0 → GameOver
  └─ Boss 死亡 → 1.65s 后 Victory

Upgrade
  └─ 选择强化 → Playing（下一波）

Paused
  ├─ P / ESC → Playing
  └─ Q → Title

GameOver / Victory
  ├─ R / Enter → Playing
  └─ ESC / Q → Title
```

## 数据所有权

- `Arena` 模块组合：
  - `arena::Player`（内含 `PlayerState`）
  - `std::vector<std::unique_ptr<arena::Enemy>>`
  - `std::vector<arena::Bullet>`、`arena::Portal`、`arena::Pickup`
  - `arena::Particle`、`arena::FloatingText`、`arena::Shockwave`
  - `arena::Collision`、`arena::Kill`
  - `arena::BossState`、`arena::Score`
  - `boss_`（指向敌人数组内的 Boss）、战斗反馈
- `Waves` 独占波次队列、当前波、总敌人数量。
- `Upgrades` 独占强化定义、等级计数、候选卡。
- `Renderer` 独占所有 QImage、星空、闪光/震动、横幅和卡片布局。
- `Game` 独占状态机、`runTime_`、胜利延迟。
- `Base` 独占键鼠状态、音效池、字体和绘制原语。
- `Random` 独占随机引擎。

组件类型位于 `src/Arena/`，`Game` / `Renderer` / `Waves` / `Upgrades` 不直接使用它们。外部模块只通过 `Arena` 的公共接口访问战斗世界；Arena 内部才组合组件。

## 后续加机制应该改哪里

### 1. 加一个数值型强化

只需要改 `Upgrades`：

1. 在 `Upgrades::catalog()` 加一条定义。
2. 在 `Upgrades::eligible()` 加可选性限制（如果需要）。
3. 在 `Upgrades::applyEffect()` 加效果。
4. 如果效果需要新字段，把字段加到 `arena::PlayerState`。

`Renderer` 会自动画卡片，`Game` 不需要改。

### 2. 加状态效果（减速、燃烧、中毒、易伤）

只改 `Arena/Enemy.*`，不要改 `Game` 或 `Renderer`：

- 短期：给 `arena::Enemy` 加 `slowTimer` 等字段，在 `Enemy::update` 和 `takeDamage` 中处理。
- 多个状态：在 `src/Arena/` 新增一个 `StatusEffect` 组件，由 `Arena` 组合并在 `updateEnemies` 中 tick。
- 如果想在 HUD 显示状态，给 `Arena::BossInfo` / 玩家快照加只读字段，再由 `Renderer` 读取。

这就是“一个工作只做一次”：状态逻辑不会漏到 Game 里。

### 3. 加一个新敌人

改 `src/Arena/Enemy.*` 和 `Waves`：

1. `arena::Enemy::statsFor()` 加属性表项。
2. `arena::Enemy::update()` 加行为分支。
3. `arena::Enemy::drawBody()` 加外观。
4. `arena::Kill::handle()` 加掉落/分裂。
5. `Waves::buildPlan()` 把它加入波次表。

外部模块不需要知道这个敌人的存在。

### 4. 加一个新 Boss / 多 Boss

当前 Boss 仍是 `arena::Enemy` 的一个特殊分支（`kind == "boss"`），阶段状态在 `arena::BossState`。如果只加一个 Boss，继续在 `src/Arena/` 内完成即可；如果要加多个 Boss，建议在 `src/Arena/` 下新增 `BossPattern` 组件，并让 `BossState` 持有它：

```cpp
namespace arena {
enum class BossKind { VoidDevourer, ... };
struct BossPattern {
    float preferredDistance;
    float aimedInterval;
    float radialInterval;
    int radialCount;
    ...
};
}
```

这只影响 `src/Arena/` 内部，不改变 `Game` 和 `Renderer` 的接口。

### 5. 加波次 / 无尽模式

总波数只在 `Waves::TotalWaves` 一处定义。HUD 和结算通过 `Waves::totalWaves()` 读取。

- 加第 9 波：改 `Waves::buildPlan` 和 `Waves::TotalWaves`。
- 无尽模式：把 `TotalWaves` 改成运行配置，`Waves::start` 后由 Game 在波次清空时决定“继续还是结算”。
- 不需要再改 HUD、Renderer 的 `/08` 或 `/8`，这些已经由 `Waves` 提供。

### 6. 加新武器 / 子弹行为

改 `Arena`：

- 在 `arena::PlayerState` 加武器状态。
- 在 `arena::Player::shoot` 里决定发射方式。
- 在 `arena::Bullet` 和 `arena::Collision::resolvePlayerBullets` 里加弹道/命中行为。
- 在 `Upgrades` 里加武器相关强化。

### 7. 加界面/模式

- 状态迁移加在 `Game`。
- 新的绘制层加在 `Renderer`。
- 新的输入动作由 `Game` 解释，`Base` 只提供键鼠状态。
- 不要把新的战斗计算放回 Game。

## 当前已知的边界

- 数值表仍是 C++ 静态表，不是外部 JSON；改平衡需要重新编译。
- 敌人/强化/音效仍以字符串 ID 传递，未来可以改成 `enum class EnemyKind / UpgradeId / SoundId`。
- `Arena` 对 `namespace arena` 内部组件开放内部访问；组件属于同一模块。`Upgrades` 是唯一一个模块外业务 friend，因为强化效果要直接修改玩家属性。`SelfTest` 的 friend 是测试专用口。
- `Arena` 同时负责战斗组件编排和战斗世界绘制；这是刻意的，避免把组件内部状态暴露给 Renderer。若以后战场视觉极其复杂，再考虑把绘制拆分出去。
- 没有 ECS。以当前规模，这比 ECS 更直接。

## 测试与交付

测试代码独立在 `tests/SelfTest.h`，不在 `src/` 的 release 编译单元里：

```cpp
// src/main.cpp
#ifdef DEBUG
#include "../tests/SelfTest.h"
#endif
```

- `main.cpp` 的参数解析、`--smoke-test` / `--capture` / `--window-test` 分支都在 `#ifdef DEBUG` 内。
- Makefile 的 `test` 目标用 `-DDEBUG` 编译 `build/main_test.o`，生成 `build/NeonCoreConsole.exe`。
- 默认 `all` / `release` 目标只编译 `main_release.o`，不包含测试头；`build/NeonCore.exe` 里没有 `SelfTest`。
- `deploy` 只复制 release `NeonCore.exe`，并从 `dist/NeonCore` 删除旧的测试用 `NeonCoreConsole.exe`。

因此交付的 `dist/NeonCore/NeonCore.exe` 不包含测试逻辑；测试控制台只存在于开发构建目录。

## 文件地图

```text
src/GameConfig.h
src/Math.h / Math.cpp
src/Random.h / Random.cpp
src/base/Base.h / Base.cpp
src/Arena/Arena.h / Arena.cpp
src/Arena/Fighter.h
src/Arena/Player.h / Player.cpp
src/Arena/Enemy.h / Enemy.cpp
src/Arena/Bullet.h / Bullet.cpp
src/Arena/Portal.h / Portal.cpp
src/Arena/Pickup.h / Pickup.cpp
src/Arena/Particle.h / Particle.cpp
src/Arena/FloatingText.h / FloatingText.cpp
src/Arena/Shockwave.h / Shockwave.cpp
src/Arena/Collision.h / Collision.cpp
src/Arena/Kill.h / Kill.cpp
src/Arena/BossState.h / BossState.cpp
src/Arena/Score.h / Score.cpp
src/Waves.h / Waves.cpp
src/Upgrades.h / Upgrades.cpp
src/Renderer.h / Renderer.cpp
src/Game.h / Game.cpp
src/main.cpp
tests/SelfTest.h          # 仅 -DDEBUG 测试构建包含
```
