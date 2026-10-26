# 星核协议 C++ 架构

这份文档只描述 **C++ / Qt 6 版** 的结构。目标是让后续加机制时，知道“这件事应该由谁决定”，而不是继续往中心类里堆分支。

## 设计原则

1. **一个工作只做一次**
   - 波次台本由 `Menu` 定义成数据（`games::Script`），开局时提交给 `Games`；`games::Waves` 只按台本生成出生队列。
   - 强化池、可选性和强化效果由 `games::Upgrades` 决定。
   - 敌人 AI、碰撞、Boss 阶段、分数由 `Arena` 及其组件决定。
   - 画面布局和局内表现反馈由 `Games` 的 `Renderer` 组件决定。
   - 主菜单、关卡选择和对局结算由 `Menu` 决定。
   - 输入、音频、字体和绘制原语由 `Base` 及其组件提供；`Base` 是进程级单例 `BaseService`，任何模块直接使用，不通过构造参数传递。
   - `Games` 只负责一局游戏的局内状态机、组件组合和调用顺序。

2. **模块是全局类，组件是 namespace 中的类**
   - 除 `main` / `WinMain` 外，没有自由函数。
   - 全局模块类：`GameConfig / Math / Random / Base / Arena / Games / Shop / Menu / GameWindow / SelfTest`。
   - 进程级单例服务（`extern` 全局对象，不挂在任何模块上）：`BaseService`（输入/音频/字体）、`RandomService`（随机引擎）、`ShopService`（钱包/库存）。业务模块直接引用它们，不通过构造参数传递。
   - `Base` 的组件在 `namespace base`：`Input / Audio / Music / Paint`。
   - `Games` 的组件在 `namespace games`：`Waves / Upgrades`。
   - `Arena` 的组件在 `namespace arena`：`Bullets / Combatants / Entities / Events / State`；它们是 `Arena` 的**私有基类**，不是成员，也不对外暴露。
   - 组件由模块组合（`Arena` 用私有继承 + `arena::Context` 注入，其余用成员）；外部模块只依赖模块的公共接口。

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
│  │  ├─ Pointer/Pointer.h/.cpp   全局鼠标指针
│  │  └─ Paint/Paint.h/.cpp
│  ├─ Arena/
│  │  ├─ Arena.h / Arena.cpp      私有继承组件 + 编排
│  │  ├─ Context.h                组件之间共享的引用（Arena 注入，模块内部）
│  │  ├─ Host.h                   组件 → 模块的虚函数回调接口
│  │  ├─ Events.h / Events.cpp
│  │  ├─ State.h / State.cpp
│  │  ├─ Bullets/
│  │  │  ├─ Bullets.h/.cpp
│  │  │  └─ StandardBullet.h/.cpp
│  │  ├─ Combatants/
│  │  │  ├─ Combatants.h/.cpp
│  │  │  ├─ Player.h/.cpp
│  │  │  ├─ Enemy.h/.cpp              敌人基类 + kind→工厂表
│  │  │  └─ Enemies/
│  │  │     ├─ EnemyTypes.h/.cpp      六个虚基类
│  │  │     ├─ EnemyRegistry.h        CRTP 注册器
│  │  │     ├─ DroneEnemy.h/.cpp
│  │  │     ├─ SpitterEnemy.h/.cpp
│  │  │     ├─ TankEnemy.h/.cpp
│  │  │     ├─ SplitterEnemy.h/.cpp
│  │  │     ├─ MiniEnemy.h/.cpp
│  │  │     ├─ RusherEnemy.h/.cpp
│  │  │     ├─ AmplifierEnemy.h/.cpp
│  │  │     ├─ LinkerEnemy.h/.cpp
│  │  │     ├─ MenderEnemy.h/.cpp
│  │  │     ├─ NecromancerEnemy.h/.cpp
│  │  │     ├─ SummonerEnemy.h/.cpp
│  │  │     └─ BossEnemy.h/.cpp
│  │  └─ Entities/
│  │     ├─ Entities.h/.cpp
│  │     ├─ Mine.h/.cpp
│  │     ├─ Portal.h/.cpp
│  │     ├─ Pickup.h/.cpp
│  │     ├─ Particle.h/.cpp
│  │     ├─ FloatingText.h/.cpp
│  │     └─ Shockwave.h/.cpp
│  ├─ Games/
│  │  ├─ Games.h / Games.cpp
│  │  ├─ Waves/Waves.h/.cpp       组件：按台本出生敌人
│  │  ├─ Upgrades/Upgrades.h/.cpp 组件：强化池、抽取与应用
│  │  └─ Renderer/Renderer.h/.cpp 组件：局内画面与特效
│  ├─ Data/GameData.h              编译期数值表（展开自 src/json）
│  ├─ Data/Settings.h              配置存储（注册表 / ini）
│  ├─ Menu/Menu.h / Menu.cpp
│  │  ├─ Main/Main.h/.cpp         标题页组件
│  │  ├─ Map/Map.h/.cpp           星区地图组件
│  │  ├─ Match/Match.h/.cpp       当前对局组件
│  │  ├─ Result/Result.h/.cpp     结算覆盖层组件
│  │  ├─ Editor/Editor.h/.cpp     台本编辑器组件
│  │  ├─ Depot/Depot.h/.cpp       补给站界面组件
│  │  └─ Settings/Settings.h/.cpp 音量设置组件
│  ├─ Shop/Shop.h / Shop.cpp      模块入口：组合 Score / Inventory
│  │  ├─ Score/Score.h/.cpp       组件：全局积分
│  │  └─ Inventory/Inventory.h/.cpp 组件：消耗品库存
│  ├─ app.rc
│  └─ main.cpp
├─ lib/
│  ├─ Math.h / Math.cpp
│  ├─ Json.h                        constexpr JSON 解析器
│  └─ Random.h / Random.cpp
├─ include/
│  ├─ GameConfig.h
│  ├─ Bullet.h
│  └─ Fighter.h
├─ json/
│  ├─ player.json                   玩家基础数值
│  ├─ enemies.json                  敌人基础数值
│  ├─ waves/level-01.json           第 1 关台本 + 出怪节奏
│  ├─ upgrades.json                 强化数值区间
│  ├─ items.json                    商店道具（地雷）
│  ├─ levels.json                   星区地图节点
│  └─ bullets.json                  敌方子弹数值
├─ tools/
│  └─ embed_json.cpp                构建期把 JSON 嵌成 raw string
└─ tests/
   └─ SelfTest.h
```

## 模块与组件

### Base

`Base` 是全局模块类，组合 5 个 `namespace base` 组件（`Input / Audio / Music / Pointer` 由 `Impl` 持有，`Paint` 是全静态）：

| 组件 | 职责 |
| --- | --- |
| `Input` | 键鼠状态、移动轴、射击状态 |
| `Audio` | 程序化 WAV 音效、QSoundEffect 池、设备重试、静音 |
| `Music` | 背景音乐管理：固定目录轮转、设备等待、静音；播放器常驻，不随界面重建 |
| `Pointer` | 全局鼠标指针：位置、独立时钟、可见性；准星画法转发 `Paint::reticle` |
| `Paint` | 颜色、字体、文字排版、发光、条形/面板等绘制原语，鼠标准星、齿轮与应用图标 |

`Base` 是**进程级单例**：唯一实例是 `BaseService`（`extern Base BaseService;`，定义在 `Base.cpp`），所有模块直接调用 `BaseService.playSound(...)` / `BaseService.movementAxis()`，不再通过构造函数把 `Base&` 一层层传下去。`Base::Impl` 持有 `base::Input`、`base::Audio`、`base::Music`、`base::Pointer`；静态绘制/颜色/字体/准星接口转发给 `base::Paint`。`Base.h` 不暴露组件头，指针的对外接口是 `pointerMoveTo / pointerSetVisible / pointerUpdate / pointerDraw`。

单例的构造只分配组件对象（不创建 `QSoundEffect` / `QMediaPlayer`，也不要求 `QApplication` 已存在）；字体在 `main()` 里 `QApplication` 之后用 `Base::initialize()` 初始化；音频仍由 `BaseService.initAudio()` 按需初始化，`main()` 在退出前调用 `BaseService.shutdown()`，保证 Qt 音频对象在 `QApplication` 还活着时释放。

音频是进程级服务，拆成两个组件：`Audio` 在 `%TEMP%\neon_core_cpp_audio\<pid>` 下生成自己的 WAV（同时开两个实例不会互相覆盖），启动时设备还没就绪的话会在 `play()` 里节流重试初始化；`Music` 是独立的背景音乐管理器，只初始化一次、由主线程的定时器推进曲目，任何音频重试、界面切换、开局/结算都不会重建播放器。`QMediaPlayer` 自带解码线程，`Music` 只负责在主线程上做状态管理。

指针是全局元素，不属于任何一局、也不属于任何画面，现在是 `Base` 的 `Pointer` 组件：`BaseService` 持有指针位置和独立时钟，`GameWindow` 每帧在 `paintEvent` 里通过 `BaseService.pointerDraw()` 把它叠在 `Menu::frame()` 之上（`pointerMoveTo` / `pointerSetVisible` 由窗口事件驱动）。所以主菜单、局内、强化、暂停、结算共用同一个指针，暂停时外弧也照常旋转。准星的画法只有一份，在 `base::Paint::reticle()`；窗口内隐藏系统光标，指针离开窗口时恢复系统箭头。

### Games

`Games` 是全局模块类，组合 3 个 `namespace games` 组件：

| 组件 | 职责 |
| --- | --- |
| `Waves` | 按运行时提交的台本生成出生队列、Boss 波判定、出生间隔 |
| `Upgrades` | 强化定义、可选性、三选一、叠加、效果应用 |
| `Renderer` | 局内画面与特效：背景/星空/发光合成、震屏闪屏、HUD、Boss 条、强化卡、暂停遮罩 |

`Games` 还拥有 `Arena`，构造时只接收 `Level`（台本在里面）。`BaseService` / `RandomService` / `ShopService` 都是全局单例，不在构造参数里，负责：

- 一局内的 `Playing / Upgrade / Paused / Victory / Defeat` 状态；
- 将输入转发给 `BaseService` / `Arena`；
- 推进 `Arena` 和 `games::Waves`；
- 波次清空后打开 `games::Upgrades`；
- 把 `Arena::Feedback` 转发给 `games::Renderer`；
- 把胜负结果交给 `Menu`，自己不再画标题或结算。

积分由全局 `ShopService.score()`（`shop::Score`，`src/app/Shop/Score/`）统一持有：`Games` 构造时调用 `beginRun()`（只重置本局统计），`arena::Events::handleKill()` 击杀时调用 `addKill(enemy.score)`。全局余额跨局累计并写入配置存储（Windows 注册表 `HKCU\Software\NeonCore\NeonCore`，其他平台 exe 同级 `settings.ini`；键 `score/total`），HUD 显示钱包，结算显示本局获得（`Games::runScore()`）；`Score::spend()` 供商店使用。

一局的结束条件只由台本决定：清空最后一波即胜利。`Boss` 只是一个带阶段的强敌，击杀它不会结束对局，只触发表现反馈（目标摧毁横幅、震屏、音效），所以它可以被放在任意一波。

### Menu

`Menu` 是顶层界面模块入口，结构和 `Arena` 一样：自己只持有进程级的关卡表、当前屏幕和输入路由（`Base`/`Shop` 都是全局单例，不在 `Menu` 里），把六个界面拆成 `namespace menu` 组件：

| 组件 | 职责 |
| --- | --- |
| `Main` | 标题页：标题、关卡卡、商店/设置入口；选中状态和命中测试 |
| `Map` | 星区地图：关卡节点、锁定状态、选择与进入 |
| `Match` | 持有当前这一局 `Games`：开局、推进、渲染、局内输入转发 |
| `Result` | 结算覆盖层：只接收一份只读 `Summary`，不读取 `Games` |
| `Editor` | 自定义台本编辑器：动态列（底部敌人库拖入）、逐波编辑 |
| `Depot` | 补给站界面：展示钱包/库存，把购买意图转给全局 `Shop` |
| `Settings` | 音效 / 背景音乐音量，读写配置存储 |

`Menu` 自己只负责：

- 屏幕状态机（`Main / Map / Match / Result / Editor / Shop / Settings`）和输入路由；
- 把 `Main` / `Map` / `Editor` / `Depot` / `Settings` 的意图（`OpenMap / Start / OpenEditor / OpenShop / Deploy / Back` 等 `Action`）翻译成进地图、开局、进编辑器、进商店、回主菜单；
- 开局时把 `Games::Level`（展示信息 + 台本 + 起始波）交给 `Match`；回主菜单时让 `Match` 销毁这一局；
- 给 `Result` 组装 `Summary`（胜负、得分、击杀、波次、时间）；
- 管理关卡表 `levels_`：标准协议与自定义协议的展示信息，自定义协议部署时用 `Editor::script()` 覆盖台本、固定从第 1 波开始；
- 启动时按需初始化音频（`BaseService.initAudio()`），并按配置存储恢复音量（`Shop` 的加载/保存在 `main()`）；

`Games` 一旦构造就由 `Match` 持有，`Menu` 不再直接访问；`Menu` 不参与战斗、波次或强化计算，也不直接画任何屏幕（组件的 `render()` 才画）。

### Shop

`Shop` 是进程级单例 `ShopService`（`extern Shop ShopService;`，定义在 `Shop.cpp`），组合 2 个 `namespace shop` 组件：

| 组件 | 职责 |
| --- | --- |
| `Score` | 全局积分钱包、本局获得/击杀、`spend()` |
| `Inventory` | 地雷库存、`takeMine()` |

`Shop` 只做组合与持久化：`load()/save()` 读写配置存储的 `score/total` 和 `inventory/mines`（Windows 注册表 / 其他平台 ini，见 `Data/Settings.h`），`buyMine()` 负责扣款 + 入库。持久化生命周期由 `main()` 管：启动时 `ShopService.load()`，退出前 `ShopService.save()`；`Menu` / `Games` / `Arena` / `menu::Depot` 都直接使用 `ShopService`，不再由某个模块持有并向下传递引用。

### Arena

`Arena` 是全局模块类，以**私有继承**拥有 5 个 `namespace arena` 组件（组件是它的实现细节，不是公共 API）：

| 组件 | 职责 |
| --- | --- |
| `Bullets` | 子弹池：持有 `Bullet*`（纯虚接口），统一推进、回收、绘制；按阵营分两组 |
| `Combatants` | 玩家、敌人容器、Boss 指针；推进/绘制战斗者 |
| `Entities` | 地雷、传送门、掉落、粒子、飘字、冲击波 |
| `Events` | 碰撞、接触伤害、敌人分离、击杀 |
| `State` | Boss 阶段、Boss 击败、玩家失败 |

**封装**：私有继承让外部看不到任何组件接口（`arena.enemies()`、`arena.submit()`、`arena.player()` 都不可见），`Arena` 只暴露自己声明的模块接口（`beginWave / update / draw / placeMine / playerState / bossInfo / score / kills / clear* / celebrate* / takeFeedback` 等）。

**组件之间**不再用 `friend`，也不通过 `Arena&`：`Arena` 调用组件时构造 `arena::Context`（`Host&` + 5 个组件引用），把兄弟组件的引用显式注入进去；组件只依赖 `Context`，头文件里连 `Arena` 的前置声明都不需要。`Context` 是模块内部的类型，外部拿不到。

**组件 → 模块的反馈**走 `arena::Host` 的虚函数：`Arena` 实现 `addFeedback()`，组件通过 `ctx.host` 回调，既不持有回指指针也不用 `std::function`。

测试（`-DDEBUG` 的 `SelfTest`）是 `Arena` 的 friend，可以直接摆弄内部组件；这是唯一保留的 friend。

`Arena` 自己只负责编排调用顺序、把 `Feedback` 汇总成一个快照，并提供少量跨组件的门面（`beginWave / update / draw / spawnPortal / placeMine / clearForVictory / celebrate*`）。

地雷是 `Entities` 里的实体：局内右键 → `Games::mousePressRight()` 扣 `Inventory` → `Arena::placeMine()`（位置夹到战场内）；`Events::resolveMines()` 在敌人进入感应半径后引爆，对爆炸半径内的敌人结算 `DamageSource::Shock`，并播放爆炸音效/冲击波。

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

- 发射方只做 `arena.submit(std::make_unique<StandardBullet>(...))`（`Arena` 继承了 `arena::Bullets`；组件接口不对外，见下面的封装说明），不自持子弹；
- `arena::Bullets` 按 `standpoint()` 分流到玩家/敌方两组，用 `reserve` + 交换删除（swap-pop）回收，稳态不搬移整段数组；
- 碰撞仍由 `arena::Events` 做几何判定（`segmentCircle(bullet.prev(), bullet.position(), target, targetR + bullet.radius)`），命中后调 `bullet.hit(...)`，伤害、暴击、穿透全部由弹种自己结算；
- 新弹种 = 在 `Arena/Bullets/` 加一个 `Bullet` 实现，提交即可，`Events`/`Renderer` 不用改。

### Renderer

`games::Renderer` 是 `Games` 的**局内组件**（一局一个实例，随 `Games` 构造/销毁），只读 `Games` / `Arena` / `games::Waves` / `games::Upgrades`，随机抖动取自全局 `RandomService`，负责：

- 局内背景、星空、发光合成；
- 局内 HUD、Boss 血条、波次横幅；
- 强化卡布局与命中测试；
- 暂停界面；
- 震屏、闪屏、横幅状态。

标题、失败/胜利结算、对局统计不在 Renderer 中，由 `Menu` 绘制；跨屏幕的鼠标指针在 `BaseService` 的 `base::Pointer` 组件里。`Renderer` 每局重建（`initialize()` 重新生成背景图和星空），所以切局时视觉动画从头开始。

### lib / include / tests

- `src/lib/Math.h/.cpp`：向量、clamp、ease、线段圆碰撞。
- `src/lib/Random.h/.cpp`：可注入随机引擎，并提供全局单例 `RandomService`。
- `src/include/Fighter.h`：Player / Enemy 的共同虚接口。
- `src/include/Bullet.h`：子弹的全局纯虚接口（`advance / draw / hit / alive / prev / position / radius / standpoint`）。
- `src/tests/SelfTest.h`：只在 `-DDEBUG` 构建中由 `main.cpp` 包含。

## 数值数据（src/json）

所有可调数值放在 `src/json/`，**编译期**展开，运行时不读文件、不解析 JSON：

```text
src/json/*.json
  └─ tools/embed_json.cpp（构建期）→ build/generated/GameDataJson.h（原样 raw string）
       └─ src/app/Data/GameData.h：inline constexpr auto doc = json::parse(...)
            └─ data::kPlayer / kEnemies / kCampaign / kWaveTiming / kUpgradeRanges / k*Bullet
                 └─ 游戏代码直接读常量（零运行时解析开销）
```

- 解析器：`src/lib/Json.h`，`constexpr` 递归下降，支持对象、数组、字符串、数字、true/false/null；
- `build/generated/GameDataJson.h` 由 `tools/embed_json.cpp` 生成，Makefile 自动重建；JSON 改动会通过 `.d` 依赖触发包含它的 TU 重新编译；
- 数据约束：字符串不使用转义、数字不使用指数；固定容量由 `GameData.h` 的 `Max*` 常量决定（超出会在编译期报错或截断）；
- `GameData.h` 里每个文件都有 `static_assert`，JSON 写错会直接编译失败；
- 修改数值只改 `src/json/*.json` 后重新 `make`，不需要手工生成或提交任何中间文件。

## Fighter 接口

`Player` / `Enemy` 都实现 `arena::Fighter`：

```cpp
namespace arena {

enum class DamageSource { Bullet, Shock, Contact };
struct Damage {
    float amount = 0.0f;
    bool critical = false;
    DamageSource source = DamageSource::Bullet;
    bool shared = false;   // linker 转来的伤害，不再二次分担
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

## 敌人与支援光环

`Enemy` 是抽象基类：只保留共享状态、通用移动积分、受击/死亡/绘制骨架。`Enemies/EnemyTypes.h` 定义六个虚基类，`Enemies/<Kind>Enemy.cpp` 是每个 kind 的具体实现：

| 虚基类 | 实现 |
| --- | --- |
| `OffensiveEnemy` | drone / mini / rusher |
| `RangedEnemy` | spitter / summoner |
| `DefensiveEnemy` | tank / linker |
| `SupportEnemy` | amplifier / mender / necromancer |
| `SelfDestructEnemy` | splitter（死亡分裂） |
| `BossEnemy` | boss |

目前有 11 个 kind 字符串（**没有 Kind 枚举**），具体类型由 vtable 区分、测试/外部判断用 `dynamic_cast`；数值（含光环参数）全部来自 `src/json/enemies.json`。击杀触发在 `Events::handleKill()`，由各敌人虚函数 `onKilled()` / `onAllyKilled()` 接管：

| kind | 机制 |
| --- | --- |
| `amplifier` | 150 范围内友军伤害 +30%，每帧刷新 0.35s buff；敌方子弹伤害和接触伤害都吃加成 |
| `linker` | 170 范围内友军受到的 45% 伤害转给自己；只转一层，多个 linker 均分 |
| `mender` | 每秒为 150 范围内友军恢复 7 HP |
| `rusher` | 300 速度直冲，每 1.6s 以 2.3 倍速度突进 |
| `necromancer` | 220 范围内友军阵亡时 30% 概率半血复活；复活体不再连锁复活，分数减半 |
| `summoner` | 每个常规友军阵亡积累 +1 进度（mini 不计入），满 3 时在自身周围召唤 3 只 mini |

支援型敌人的光环半径画在 `Enemy::draw()`，被增伤的敌人有一圈琥珀色提示；伤害分担通过 `Damage.shared` 防止递归。

### 寻路角色

`Enemy::Role` 字段决定基础走位；类别归属直接用虚基类（`dynamic_cast<SupportEnemy*>` 等）判断：

| Role | 成员 | 走位 |
| --- | --- | --- |
| `Melee` | drone / tank / splitter / mini / rusher | 直冲玩家，以接触为目标（rusher 额外周期突进） |
| `Ranged` | spitter / boss / summoner | 保持 205–340 的距离并绕圈；检测 140 距离内、横向 34 以内的玩家弹幕，触发 0.12s 侧闪，冷却 1.6s |
| `Support` | amplifier / linker / mender / necromancer | 扫描友军，选 150 半径内邻居最多的目标点靠拢；没有友军时退回中距离走位 |

辅助函数都在 `Enemy` 保护区：`avoidPlayerBullets()` 只读 `Bullets::playerBullets()`，`allyClusterDirection()` 只读 `Combatants::enemies()`，不引入新的组件依赖。

### 注册与新增敌人

注册走 CRTP：

```cpp
namespace {
const EnemyRegistrar<DroneEnemy> kRegistration("drone", Base::Coral);
}
```

- 每个 `*Enemy.cpp` 定义自己的静态注册器，构造时写入 `enemyRegistry()`（`Enemy.cpp` 里的函数内 `static std::unordered_map`）；
- `Enemy::create(kind, pos, wave)` 运行时查表，未注册返回 `nullptr`；
- `Enemy.cpp` 不再有按 kind 的走位/绘制 `switch`，只保留基类逻辑、`statsFor()` 和注册表；
- 新增敌人：`enemies.json` 加数值 → 在 `Enemies/` 新建 `XEnemy.h/.cpp`，继承对应虚基类并写走位/绘制 → 在 cpp 里放 `EnemyRegistrar` → Makefile 的 `ARENA_COMPONENTS` 加对象 → 编辑器敌人库（`Menu/Editor/Editor.cpp` 的 `kKeys/kLabels/kindColor`）加一项 → 波次里使用。

## 依赖方向

```text
main() ──► Base::initialize()（字体） + ShopService.load()
GameWindow ──► BaseService.pointer*（指针位置/时钟/绘制）
            │
            ├─ BaseService ────────► base::Input / Audio / Music / Pointer / Paint（全局单例）
            ├─ RandomService ──────► 随机引擎（全局单例）
            ├─ ShopService ────────► shop::Score / Inventory（全局单例，注册表 / ini）
            └─ Menu（持有商店外的关卡表，组合 7 个界面组件）
               ├─ menu::Main ─────────► 关卡卡 / 商店与设置入口（数据由 Menu 提供）
├─ menu::Map ──────────► 星区地图节点（levels.json），Start → 开局
               ├─ menu::Match ────────► Games(Level)   ← 一局一个实例
               │  └─ Games
               │     ├─ Arena ────► 私有继承 arena::Bullets / Combatants / Entities / Events / State
               │     │              （组件回调模块反馈走 arena::Host 虚接口）
               │     ├─ games::Waves ─────► Arena, RandomService
               │     ├─ games::Upgrades ──► Arena, RandomService（音效走 BaseService）
               │     └─ games::Renderer ──► Games/Arena/Waves/Upgrades(只读), RandomService
               ├─ menu::Result ───────► 只读 Summary
               ├─ menu::Editor ───────► games::Script（自己持有）
               ├─ menu::Depot ────────► ShopService（buyMine）+ 只读钱包/库存
               └─ menu::Settings ─────► BaseService 音量接口 + 配置存储
```

依赖是单向的：

- `GameWindow` 持有 `Menu`，负责把画面和 `BaseService` 的指针合成到窗口上。
- `BaseService` / `RandomService` / `ShopService` 是叶子服务：只被调用，不反向依赖任何业务模块；需要它们的模块直接引用全局符号。
- `base::Pointer` 只依赖 `base::Paint` 的准星画法，不知道游戏业务。
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
│  ├─ Map:    menu::Map 自己推进时间/悬停
│  ├─ Editor: 输入自处理（menu::Editor），Menu 不更新
│  ├─ Shop:   输入自处理（menu::Depot），Menu 不更新
│  ├─ Match:  menu::Match → Games::update(dt)
│  │          ├─ Renderer::update(dt)
│  │          ├─ Playing: Arena→Waves→Feedback→胜负/波次判断
│  │          ├─ Upgrade: games::Upgrades 候选卡
│  │          ├─ Paused: 冻结
│  │          └─ Victory/Defeat: 只推进视觉
│  └─ Result: menu::Match 继续 Games::update(dt) 退场表现
├─ BaseService.pointerUpdate(dt)   ← 指针独立时钟，暂停时也推进
├─ Menu::render()
│  ├─ Main:    menu::Main   绘制菜单
│  ├─ Map:     menu::Map    绘制星区地图
│  ├─ Editor:  menu::Editor 绘制台本表格
│  ├─ Shop:    menu::Depot  绘制补给站
│  ├─ Settings: menu::Settings 绘制设置页
│  ├─ Match:  menu::Match → Games::render() → games::Renderer
│  └─ Result: menu::Match 画面 + menu::Result 结算覆盖层
└─ QWidget::update()

GameWindow::paintEvent()
├─ drawImage(Menu::frame())              ← 当前画面
└─ BaseService.pointerDraw()             ← 全局指针叠在最上层
```

## 加机制应该改哪里

### 改数值（平衡）

| 想改什么 | 只改这里 |
| --- | --- |
| 玩家血量 / 移速 / 伤害 / 射速 / 弹速 / 弹体半径 | `src/json/player.json` |
| 敌人血量 / 速度 / 半径 / 接触伤害 / 分数、Boss 基础值 | `src/json/enemies.json` |
| 战役波次组成、出怪节奏、波次成长系数 | `src/json/waves/level-01.json` |
| 强化每轮的随机区间 | `src/json/upgrades.json` |
| 敌方子弹速度 / 伤害 / 半径 | `src/json/bullets.json` |
| 地雷价格 / 伤害 / 感应与爆炸半径 / 引爆延迟 / 寿命 | `src/json/items.json` |
| 星区地图节点位置 / 名称 / 是否解锁 | `src/json/levels.json` |

改完直接 `make`：JSON 会重新嵌成 raw string，包含数值表的 TU 自动重编译；写错格式会在编译期由 `static_assert` 报错。

### 加强化

数值区间在 `src/json/upgrades.json`（按 key 的 `min`/`max`），其余在 `src/app/Games/Upgrades/`：

1. `src/json/upgrades.json` 加 `"key": {"min": ..., "max": ...}`（`min = max` 表示固定值，如 +1 穿透）；
2. `Upgrades::catalog()` 加名称/标签/颜色（数值会自动从 `data::findUpgrade()` 填）；
3. `Upgrades::eligible()` 加限制；
4. `Upgrades::describe()/valueLabel()` 加文案（卡面显示的数值就是实际生效值）；
5. `Upgrades::applyEffect()` 加效果，直接用 `offer.value`；
6. 新字段加到 `arena::PlayerState`。

`Renderer` 自动画卡片和右上角的 `offer.valueText`，`Games` 不用改。

### 加 / 改波次

只改 `src/json/waves/level-01.json` 的 `campaign`：每一波是 `[{"kind": "...", "count": N}, ...]`，最后一波带 `boss` 即为 Boss 波；`Menu::standardScript()` 把它转成 `games::Script` 提交。每个关卡一个文件（`src/json/waves/level-0N.json`），`levels.json` 的节点用 `waves` 字段登记文件 id；新增文件后同步 Makefile 的嵌入参数和 `GameData` 里的文档常量。

- 台本是数据，开局时作为 `Games::Level.script` 提交，`Waves` 只负责按它排队出生，不再有内置台本；
- 星区地图节点在 `src/json/levels.json`；把节点 `locked` 设为 `false` 前，需要先在关卡表里补上对应的标准关卡（目前只有第一个区域可用）；
- 出怪节奏（首波/后续延迟、间隔区间）在同一个文件的 `spawn` 段；
- 总波数 = 台本长度，HUD 通过 `Waves::totalWaves()` 读取，结算通过 `Games` 的快照读取；
- `boss` 出现在某一波里，那一波就是 Boss 波（`Waves::isBossWave()`），并且 Boss 永远第一个出场；击杀 Boss 不会结束对局，只有最后一波清空才胜利。

### 加关卡

改 `src/app/Menu/Menu.cpp` 的 `levels_`：

```cpp
{name, subtitle, startWave, script}
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

- 敌人：数值在 `src/json/enemies.json`，颜色与行为在 `src/app/Arena/Combatants/Enemy.*`，出生在 `Waves`；支援/特殊敌人的光环参数也在 `enemies.json`。
- 状态：`src/app/Arena/State.*`。
- 碰撞/击杀：`src/app/Arena/Events.*`。
- 实体：`src/app/Arena/Entities/*`。

### 加弹种

1. 在 `src/include/Bullet.h` 之外、`src/app/Arena/Bullets/` 里新建实现（参考 `StandardBullet`）；
2. 实现 `advance`（推进/寿命/出屏）、`draw`（样式）、`hit`（命中结算并返回是否继续存活）；
3. 发射处调用 `arena.submit(std::make_unique<YourBullet>(...))`，池会按 `standpoint()` 分流；
4. `Events` 的几何判定、`Arena` 的批量清理和绘制都不需要改。

### 加 IO / 音频 / 绘制原语

- 输入：`src/app/Base/Input/`。
- 音效：`src/app/Base/Audio/Audio.*`；背景音乐：`src/app/Base/Audio/Music.*`。
- 字体/颜色/绘制：`src/app/Base/Paint/`。
- 鼠标指针：`src/app/Base/Pointer/`（位置 + 时钟 + 可见性，`BaseService` 转发）；准星画法在 `base::Paint::reticle()`。
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
- 一局一个 `Games`：`Base`（输入/音频/字体）、`Random`、`Shop` 都是进程级单例，否则切局会重复初始化音频/随机源/钱包。
- 一局的安排只经构造参数进入 `Games`；`Games` 不提供任何读回安排的接口，`Menu` 只保留展示用的关卡表。
- `Arena` 的玩家状态对 `games::Upgrades` 开放写权限（`Arena::playerState()`），不再需要模块间 friend。
- `Arena` 的 5 个组件是它的私有基类：外部只看到 `Arena` 的公共接口；组件之间通过 `arena::Context` 注入的兄弟引用协作，组件 → 模块的反馈走 `arena::Host` 虚函数。
- `Arena` 只保留 `friend class SelfTest`（`-DDEBUG` 测试用来直接摆局面），没有业务 friend。
- 没有 ECS。当前 5 个 Arena 组件 + 2 个 Games 组件的粒度更适合这个规模。
- `Menu` 只做界面/关卡创建，不读取 `Arena` 内部实体；它通过 `Games` 的 getter 获取对局信息。
