# 星核协议 C++ 架构

这份文档只描述 **C++ / Qt 6 版** 的结构。目标是让后续加机制时，知道“这件事应该由谁决定”，而不是继续往中心类里堆分支。

## 设计原则

1. **一个工作只做一次**
   - 波次台本由 `Menu` 定义成数据（`games::Script`），开局时提交给 `Games`；`games::Waves` 只按台本生成出生队列。
   - 强化池、可选性和强化效果由 `games::Upgrades` 决定。
   - 敌人 AI、碰撞、Boss 阶段、分数由 `Arena` 及其组件决定。
   - 一局的模拟（`Arena / Waves / Upgrades`、状态、计时、输入）由 `games::GameSimulation` 决定。
   - 模拟通过 `net::Snapshot` 交给表现层：`games::GameSimulation::snapshot()` 提取只读数据，`client::GameView` 按快照重建可绘制世界；`Renderer` 只读 `GameView`，不读模拟对象。
   - 画面布局和局内表现反馈由 `Games` 的 `Renderer` 组件决定。
   - 主菜单、关卡选择和对局结算由 `Menu` 决定。
   - 输入、音频、字体和绘制原语由 `Base` 及其组件提供；`Base` 是进程级单例 `BaseService`，任何模块直接使用，不通过构造参数传递。
   - `Games` 是单机适配器：把本地 `BaseService` 输入转成 `arena::InputState`，并把模拟事件接到 `Renderer` / 音效。

2. **模块是全局类，组件是 namespace 中的类**
   - 除 `main` / `WinMain` 外，没有自由函数。
   - 全局模块类：`GameConfig / Math / Random / Base / Arena / Games / Shop / Menu / GameWindow / SelfTest`。
   - 进程级单例服务（`extern` 全局对象，不挂在任何模块上）：`BaseService`（输入/音频/字体）、`RandomService`（随机引擎）、`ShopService`（钱包/库存）。业务模块直接引用它们，不通过构造参数传递。
   - `Base` 的组件在 `namespace base`：`Input / Audio / Music / Paint`。
   - `Games` 的组件在 `namespace games`：`GameSimulation / Waves / Upgrades / Renderer`。
   - 网络层类在 `namespace server` / `namespace client`：`GameServer / GameClient / GameView`；`net` 命名空间只放协议、输入、消息和快照数据。
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
│  ├─ shared/
│  │  ├─ Arena/                    Arena + Bullets / Combatants / Enemies / Entities / Events / State
│  │  └─ Net/                      Protocol.cpp / Input.cpp / Messages.cpp / Snapshot.cpp
│  ├─ server/
│  │  ├─ Games/
│  │  │  ├─ Simulation/GameSimulation.h/.cpp
│  │  │  ├─ Waves/Waves.h/.cpp
│  │  │  └─ Upgrades/Upgrades.h/.cpp
│  │  ├─ Net/Server.h/.cpp         QTcpServer：headless 模拟、60Hz tick、广播、房间生命周期
│  │  ├─ Room/Room.h/.cpp           服务端权威房间：名字/人数/房主/等待或对局中
│  │  ├─ Shop/                      Score / Inventory（服务端权威）
│  │  ├─ Loadout/                   发射器选择（服务端权威）
│  │  └─ main.cpp                   独立服务端入口（--port，stdin 控制）
│  └─ client/
│     ├─ Base/                     窗口/输入/音频/绘制原语/线程池
│     ├─ Games/
│     │  ├─ Games.h / Games.cpp    单机适配器：GameSimulation + Renderer
│     │  └─ Renderer/Renderer.h/.cpp
│     ├─ Client/GameView.h/.cpp    快照 → 可绘制的只读 Arena 镜像
│     ├─ Net/Client.h/.cpp         QTcpSocket：握手、输入、快照
│     ├─ Menu/
│     │  ├─ Menu.h / Menu.cpp
│     │  ├─ Main/ Map/ Match/ Room/ Failure/ Result/
│     │  ├─ Editor/ Depot/ Armory/
│     │  └─ Settings/
│     ├─ app.rc
│     └─ main.cpp                  客户端入口/窗口；服务端模式已移到 NeonCoreServer
├─ lib/
│  ├─ Math.h / Math.cpp
│  ├─ Json.h                        constexpr JSON 解析器
│  └─ Random.h / Random.cpp
├─ include/
│  ├─ Data/GameData.h              编译期数值表（展开自 src/json）
│  ├─ Data/Settings.h              配置存储（注册表 / ini）
│  ├─ GameConfig.h
│  ├─ Bullet.h
│  ├─ Fighter.h
│  └─ Net/
│     ├─ Protocol.h                协议版本、消息 id、长度前缀与帧解码、FrameBuffer
│     ├─ Input.h                   客户端 → 服务端输入命令模型与序列化
│     ├─ Messages.h                Hello / Welcome / StartMatch / Event / MatchEnd
│     └─ Snapshot.h                服务端 → 客户端的只读世界快照 + QDataStream 序列化
├─ json/
│  ├─ player.json                   玩家基础数值
│  ├─ enemies.json                  敌人基础数值
│  ├─ waves/level-01..04.json       四个关卡的台本 + 出怪节奏
│  ├─ loadout.json                  战备发射器数值
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

`Base` 是全局模块类，组合 6 个 `namespace base` 组件（`Input / Audio / Music / Pointer / ThreadPool` 由 `Impl` 持有，`Paint` 是全静态）：

| 组件 | 职责 |
| --- | --- |
| `Input` | 键鼠状态、移动轴、射击状态 |
| `Audio` | 程序化 WAV 音效、QSoundEffect 池、设备重试、静音 |
| `Music` | 背景音乐管理：固定目录轮转、设备等待、静音；播放器常驻，不随界面重建 |
| `Pointer` | 全局鼠标指针：位置、独立时钟、可见性；准星画法转发 `Paint::reticle` |
| `Paint` | 颜色、字体、文字排版、发光、条形/面板等绘制原语；`Pass` 控制世界/辉光通道 |
| `ThreadPool` | 帧内并行任务池：主线程也参与执行，用于世界层/辉光层并行渲染 |

`Base` 是**进程级单例**：唯一实例是 `BaseService`（`extern Base BaseService;`，定义在 `Base.cpp`），所有模块直接调用 `BaseService.playSound(...)` / `BaseService.movementAxis()`，不再通过构造函数把 `Base&` 一层层传下去。`Base::Impl` 持有 `base::Input`、`base::Audio`、`base::Music`、`base::Pointer`、`base::ThreadPool`；静态绘制/颜色/字体/准星接口转发给 `base::Paint`。`Base.h` 不暴露组件头，指针的对外接口是 `pointerMoveTo / pointerSetVisible / pointerUpdate / pointerDraw`，并行接口是 `parallelEnabled / parallelWorkers / parallelFor`。

单例的构造只分配组件对象（不创建 `QSoundEffect` / `QMediaPlayer`，也不要求 `QApplication` 已存在，不启动线程）；字体和工作线程在 `main()` 里 `QApplication` 之后用 `Base::initialize()` 初始化；音频仍由 `BaseService.initAudio()` 按需初始化；`main()` 在退出前调用 `BaseService.shutdown()`，先停线程池，再释放 Qt 音频对象。

音频是进程级服务，拆成两个组件：`Audio` 在 `%TEMP%\neon_core_cpp_audio\<pid>` 下生成自己的 WAV（同时开两个实例不会互相覆盖），启动时设备还没就绪的话会在 `play()` 里节流重试初始化；`Music` 是独立的背景音乐管理器，只初始化一次、由主线程的定时器推进曲目，任何音频重试、界面切换、开局/结算都不会重建播放器。`QMediaPlayer` 自带解码线程，`Music` 只负责在主线程上做状态管理。

指针是全局元素，不属于任何一局、也不属于任何画面，现在是 `Base` 的 `Pointer` 组件：`BaseService` 持有指针位置和独立时钟，`GameWindow` 每帧在 `paintEvent` 里通过 `BaseService.pointerDraw()` 把它叠在 `Menu::frame()` 之上（`pointerMoveTo` / `pointerSetVisible` 由窗口事件驱动）。所以主菜单、局内、强化、暂停、结算共用同一个指针，暂停时外弧也照常旋转。准星的画法只有一份，在 `base::Paint::reticle()`；窗口内隐藏系统光标，指针离开窗口时恢复系统箭头。

### Games

`Games` 是单机适配器，公开继承 `games::GameSimulation`，并拥有 `games::Renderer`。模拟和表现的分工：

| 组件 | 职责 |
| --- | --- |
| `GameSimulation` | 持有 `Arena / Waves / Upgrades`、一局状态、运行时间、胜负/清波计时；只推进模拟，不依赖 Renderer，可提取 `net::Snapshot` |
| `Waves` | 按运行时提交的台本生成出生队列、Boss 波判定、出生间隔 |
| `Upgrades` | 强化定义、可选性、三选一、叠加、效果应用 |
| `Renderer` | 局内画面与特效：背景/星空/发光合成、震屏闪屏、HUD、Boss 条、强化卡、暂停遮罩；只读 `client::GameView` |

`GameSimulation` 构造时只接收 `Level`（台本在里面），负责：

- 一局内的 `Playing / Upgrade / Paused / Victory / Defeat` 状态；
- 通过 `setInput()` 接收 `arena::InputState`（移动轴、鼠标、射击、冲刺请求），`Player::update()` 不再直接读 `BaseService`；
- 推进 `Arena` 和 `games::Waves`；
- 波次清空后打开 `games::Upgrades`；
- 把 `Arena::Feedback`、胜负、Boss 击破、清波等事件通过 `on*` 虚函数钩子交给适配器。

`Games` 只负责单机表现：每帧从 `BaseService` 采集 `arena::InputState` 再调用 `GameSimulation::update()`，实现钩子调用 `renderer_.shake/flash/showBanner/burstConfetti` 和 `BaseService.playSound()`，并继续把胜负结果交给 `Menu`。`Games::render()` 不再把模拟对象直接交给 `Renderer`：先 `writeSnapshot()` 提取 `net::Snapshot`，再用 `client::GameView::apply()` 重建视图，最后 `Renderer` 只读这份视图。单机和将来的 TCP 客户端因此共用同一条表现路径；`GameSimulation` 可以脱离 `Renderer` 单独构造和 update。

积分由全局 `ShopService.score()`（`shop::Score`，`src/app/server/Shop/Score/`）统一持有：`Games` 构造时调用 `beginRun()`（只重置本局统计），`arena::Events::handleKill()` 击杀时调用 `addKill(enemy.score)`。全局余额跨局累计并写入配置存储（Windows 注册表 `HKCU\Software\NeonCore\NeonCore`，其他平台 exe 同级 `settings.ini`；键 `score/total`），HUD 显示钱包，结算显示本局获得（`Games::runScore()`）；`Score::spend()` 供商店使用。

一局的结束条件只由台本决定：清空最后一波即胜利。`Boss` 只是一个带阶段的强敌，击杀它不会结束对局，只触发表现反馈（目标摧毁横幅、震屏、音效），所以它可以被放在任意一波。

关卡使用 `waves/level-01..04.json`，长度 8/10/12/12 波；`Games::Level.enemyLevel` 为 1–4，通过 `Waves` → `Portal` → `Combatants::spawnEnemy()` 传到 `Enemy::setLevel()`：每级约 +20% 血量、+12% 伤害、+3.5% 速度，只影响数值，不显示等级数字，也不进入 HUD。召唤/分裂产生的单位沿用来源的等级。

### Menu

`Menu` 是顶层界面模块入口，结构和 `Arena` 一样：自己只持有进程级的关卡表、当前屏幕和输入路由（`Base`/`Shop` 都是全局单例，不在 `Menu` 里），把十个界面拆成 `namespace menu` 组件：

| 组件 | 职责 |
| --- | --- |
| `Main` | 标题页：标题、关卡卡、商店/战备/设置入口；选中状态和命中测试 |
| `Map` | 星区地图：关卡节点、锁定状态、选择与进入 |
| `Match` | 默认持有网络 `GameClient`（内嵌/远端 `GameServer`）+ `GameView`/`Renderer`；`--local`/测试时持有本地 `Games`。`Mode::Local` 直接开局、`Mode::Network` 单机立即开局/加入远端、`Mode::Host` 只开房等待 `startRoomMatch()`。负责开局、房间生命周期、输入转发、推进、渲染和失败信息 |
| `Room` | 房间/大厅：显示服务端权威的房间名、人数、房主/你标记和等待状态；主机的 `ENTER` 开始、`X` 解散、`ESC` 离开，加入者只能 `ESC` 离开 |
| `Failure` | 连接层失败页：无法连接服务器 / 房间已解散，提供 `ENTER` 重试与 `ESC` 返回；不读取战绩，不冒充对局失败 |
| `Result` | 结算覆盖层：只接收一份只读 `Summary`，不读取 `Games` |
| `Editor` | 自定义协议编辑：顶部单机/局域网主机/加入主机选项卡；单机与主机编辑台本，主机在波次下方追加端口，加入主机显示地址和端口 |
| `Depot` | 补给站界面：展示钱包/库存，把购买意图转给全局 `Shop` |
| `Armory` | 战备界面：展示/选择发射器，把装备意图转给全局 `Loadout` |
| `Settings` | 音效 / 背景音乐音量，读写配置存储 |

`Menu` 自己只负责：

- 屏幕状态机（`Main / Map / Room / Match / Failure / Result / Editor / Shop / Armory / Settings`）和输入路由；
- 把 `Main` / `Map` / `Editor` / `Room` / `Failure` / `Depot` / `Armory` / `Settings` 的意图（`OpenMap / Start / OpenEditor / StartRoom / DissolveRoom / Retry / OpenShop / OpenArmory / Deploy / Back` 等 `Action`）翻译成进地图、开局、进房间、进编辑器、进商店/战备、回主菜单；
- 开局时把 `Games::Level`（展示信息 + 台本 + 起始波）交给 `Match`；回主菜单时让 `Match` 销毁这一局与房间；
- 给 `Result` 组装 `Summary`（胜负、得分、击杀、波次、时间）；
- 管理关卡表 `campaignLevels_`（四个关卡）与 `levels_`（标准/自定义卡）：地图节点按 campaign 索引开局，自定义协议在 `Editor` 顶部的三个选项卡里选择单机/主机/加入，按当前选项卡的配置开始，并用 `Editor::script()` 覆盖台本、固定从第 1 波开始；标准协议始终使用默认服务器配置；
- 启动时按需初始化音频（`BaseService.initAudio()`），并按配置存储恢复音量（`Shop` 的加载/保存在 `main()`）；

`Games` 一旦构造就由 `Match` 持有，`Menu` 不再直接访问；`Menu` 不参与战斗、波次或强化计算，也不直接画任何屏幕（组件的 `render()` 才画）。

自定义协议的流程是 `Main → Editor（三个选项卡）→ Room / Match`。`Single` 使用本地直连，不监听端口，立即开局；`Host` 在编辑页波次下方显示端口，`Match` 在 `0.0.0.0` 上启动内嵌服务端并进入 `Room`，只有房主在房间页按 `ENTER` 才调用本地 `GameServer::startMatch()`；`Join` 只显示地址和端口，只创建 `GameClient`，连接后进入同一个 `Room`，不能创建本地服务端或发送 TCP `StartMatch`。房主按 `X`/离开时 `GameServer::dissolveRoom()` 先广播 `MatchEnd("房间已解散")` 再关闭监听，客户端进入 `Failure`。标准协议的 `startCampaign()` 每次都会重新套用 `Menu` 构造时的默认 host/port（无参数时是自动端口回环），不会继承上一次自定义联机设置。

### Shop

`Shop` 是进程级单例 `ShopService`（`extern Shop ShopService;`，定义在 `Shop.cpp`），组合 2 个 `namespace shop` 组件：

| 组件 | 职责 |
| --- | --- |
| `Score` | 全局积分钱包、本局获得/击杀、`spend()` |
| `Inventory` | 地雷库存、`takeMine()` |

`Shop` 只做组合与持久化：`load()/save()` 读写配置存储的 `score/total` 和 `inventory/mines`（Windows 注册表 / 其他平台 ini，见 `Data/Settings.h`），`buyMine()` 负责扣款 + 入库。持久化生命周期由 `main()` 管：启动时 `ShopService.load()`，退出前 `ShopService.save()`；`Menu` / `Games` / `Arena` / `menu::Depot` 都直接使用 `ShopService`，不再由某个模块持有并向下传递引用。

### Loadout

`Loadout` 位于 `src/app/server/Loadout/`，是进程级单例 `LoadoutService`：从 `loadout.json` 读 `LauncherDef`（伤害/射速/弹速倍率、散射倍率、额外弹丸、穿透），把选择持久化到配置存储的 `loadout/launcher`。`Player::reset()` 应用倍率到 `PlayerState`，`Player::shoot()` 应用散射/额外弹丸/穿透；默认 `standard` 发射器所有倍率为 1，保持原弹道。`menu::Armory` 只展示与选择，不直接改子弹数据。

### Arena

`Arena` 是全局模块类，以**私有继承**拥有 5 个 `namespace arena` 组件（组件是它的实现细节，不是公共 API）：

| 组件 | 职责 |
| --- | --- |
| `Bullets` | 子弹池：持有 `Bullet*`（纯虚接口），统一推进、回收、绘制；按阵营分两组 |
| `Combatants` | 玩家、敌人容器、Boss 指针；推进/绘制战斗者 |
| `Entities` | 地雷、传送门、掉落、粒子、飘字、冲击波 |
| `Events` | 碰撞、接触伤害、敌人分离、击杀 |
| `State` | Boss 阶段、Boss 击败、玩家失败 |

**多玩家（阶段 3.5）**：`Combatants` 持有 1–4 架 `arena::Player`，开局时由服务端调用 `configurePlayers(playerIds)` 生成。只有一架时沿用原来的中央出生点/Cyan 和“阵亡即失败”；2–4 架时按 playerId 分配固定出生点与识别色，敌人/敌方子弹/掉落/接触伤害都以“最近的存活玩家”为目标，玩家的瞄准和子弹只来自自己的 `InputState`。单架阵亡进入 3.2s 重新部署（满血回到出生点并短暂无敌），所有飞机同时倒下才判负；会话离开时 `removePlayer()` 只把对应飞机标记为非 active，快照不再包含它。`Arena::playerState()` 返回本机视角的飞机（客户端按 `Snapshot.localPlayerId`，服务端/单机回退第一架），`playerStateAt()` 供 HUD/强化遍历全部飞机；`Upgrades` 对全队生效。

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

- 发射方只做 `ctx.bullets.spawnStandard(...)`（`Arena` 继承了 `arena::Bullets`；组件接口不对外），不自持子弹；
- `arena::Bullets` 按 `standpoint()` 分流到玩家/敌方两组，用 `reserve` + 交换删除（swap-pop）回收；死亡的标准弹回到空闲池复用（`StandardBullet::reset`），稳态射击**不再产生堆分配**；
- 碰撞仍由 `arena::Events` 做几何判定（`segmentCircle(bullet.prev(), bullet.position(), target, targetR + bullet.radius)`），命中后调 `bullet.hit(...)`，伤害、暴击、穿透全部由弹种自己结算；
- 新弹种 = 在 `Arena/Bullets/` 加一个 `Bullet` 实现，提交即可，`Events`/`Renderer` 不用改；对象池目前只复用标准弹。

### Renderer

`games::Renderer` 是 `Games` 的**局内组件**（一局一个实例，随 `Games` 构造/销毁），只读 `client::GameView`（快照重建的玩家/敌人/子弹/实体 + HUD 状态），随机抖动取自全局 `RandomService`，负责：

- 局内背景、星空、发光合成；
- 局内 HUD、Boss 血条、波次横幅；
- 强化卡布局与命中测试；
- 暂停界面；
- 震屏、闪屏、横幅状态。

**并行渲染管线**（帧内两通道）：

| 通道 | 线程 | 写入 | 内容 |
| --- | --- | --- | --- |
| 世界 | `ThreadPool` 工作线程 | `frame_` | 背景、星空、实体、子弹、粒子、冲击波（全部是图形，无字体） |
| 辉光 | 主线程 | `glow_` | 同一批绘制调用里的 `Paint::glow`（加法光晕） |
| 字体 | 主线程（并行结束后） | `frame_` | 飘字 + HUD/横幅/卡片/暂停 |

两条通道遍历同一批绘制调用，`base::Paint::Pass`（`World` / `Glow`）让每条通道只执行属于自己的原语：世界通道忽略 `glow`，辉光通道忽略世界图形。通道写不同 `QImage`，字体永远在主线程，因此不需要锁。并行结束后主线程把 `glow_` 以 `Plus` 叠到 `frame_`，再画暗角/闪屏/UI。单线程环境（或 `NEON_SINGLE_THREAD=1`）会自动退回一遍合并绘制。

标题、失败/胜利结算、对局统计不在 Renderer 中，由 `Menu` 绘制；跨屏幕的鼠标指针在 `BaseService` 的 `base::Pointer` 组件里。`Renderer` 每局重建（`initialize()` 重新生成背景图和星空），所以切局时视觉动画从头开始。

### Client / Net

阶段 3 把单机进程内的 `GameSimulation -> Snapshot -> GameView` 路径搬到了真实 TCP 上：

| 角色 | 位置 | 职责 |
| --- | --- | --- |
| `net::Protocol` | `src/include/Net/Protocol.h` | 协议版本（v2）、消息 id、`[u32 length][u16 type][u16 version][payload]` 帧的打包/拆包、TCP 粘包缓冲 `FrameBuffer` |
| `net::Input` | `src/include/Net/Input.h` | `InputCommand`：移动轴、鼠标、射击、冲刺、暂停、布雷、强化选择；`playerId` 只用于 stdio 宿主选择目标，TCP 客户端必须为 0 |
| `net::Messages` | `src/include/Net/Messages.h` | `Hello / Welcome（sessionId + playerId/slot）/ StartMatch / Input / Snapshot / Event / MatchEnd / RoomList（含会话槽位）/ JoinRoom` 的 payload 与带帧头的编码/解码 |
| `net::Snapshot` | `src/include/Net/Snapshot.h` | `players[]`（每个玩家 id/颜色/生命/冲刺/瞄准/复活倒计时）、`localPlayerId`（每个接收者自己的飞机）、敌人（dataKey + 绘制字段）、子弹、传送门/掉落/地雷/粒子/飘字/冲击波、波次 HUD、强化卡、状态/时间/分数/击杀/Boss |
| 提取 | `GameSimulation::snapshot()` | `Arena / Combatants / Bullets / Entities` 各自把只读数据写进快照，RTTI 在边界转 `Enemy::dataKeyFor()` |
| `server::GameServer` | `src/app/server/Net/Server.*` | `QTcpServer`、会话（稳定 playerId/slot）、固定 60Hz tick、headless `GameSimulation`、按会话填写 `localPlayerId` 的 Snapshot 广播、权威输入按 playerId 路由、Event/MatchEnd/RoomList 广播、玩家离开时移除飞机 |
| `server::RoomManager` | `src/app/server/Room/Room.*` | 服务端权威房间：`open/reset` 生命周期、名字、玩家数上限、房主会话、等待/对局中 |
| `client::GameClient` | `src/app/client/Net/Client.*` | `QTcpSocket`、Hello/Welcome（记录 playerId）、输入发送、快照接收、`client::GameView`、重连与错误状态 |
| `client::GameView` | `src/app/client/Client/GameView.*` | 把快照应用回一个只读 `Arena` 镜像：重建全部玩家/敌人/传送门，按 `localPlayerId` 选择本机飞机；不推进模拟 |
| `games::Renderer` | `src/app/client/Games/Renderer/*` | 只读 `GameView` 画世界层/HUD/强化卡/暂停；HUD 的生命/冲刺/瞄准来自本机飞机，远程飞机有队伍生命列表 |

服务端只拥有模拟，不链接/使用 Renderer、Menu 或 `BaseService` 的键鼠状态；`GameSimulation::Level` 里的台本由**本地宿主**通过 `GameServer::startMatch()` 或 `NeonCoreServer` 的 stdin 控制帧提交，TCP 客户端不能发 `StartMatch`。`GameServer` 的房间控制面也是本地宿主专用：`openRoom(name)` 建立等待房间，`startMatch(level)` 才创建并启动 `GameSimulation`，`dissolveRoom(reason)` 广播 `MatchEnd`、关闭监听并断开会话。服务端用 `QElapsedTimer` 累加固定 1/60s 步长，每个 tick 后 `writeSnapshot()` 并广播；没有对局时 `RoomManager` 广播 `RoomList`（人数来自服务端实际 welcomed 会话数，`members` 给出每个会话的 playerId/slot 与房主标记），客户端进入房间/大厅而不是渲染对局；`ShopService.score().beginRun()` 在开局时重置本局统计，击杀仍由 `Arena::Events` 写回，因此比分/波次/胜负以服务端为准。表现事件（波次横幅、Boss 击破、震屏/闪屏、胜利/失败）通过 `Event` 消息发送，由客户端 `GameClient` 调用本地 `Renderer` 和音效；客户端不回传世界状态。

**会话与玩家（阶段 3.5）**：Hello 时服务端给每个 welcomed 会话分配稳定的 `playerId`/slot（1–4，房间满时拒绝第 5 个连接），Welcome 把它回给客户端。`startMatch()` 把当时所有 welcomed 会话的 playerId 组成这一局的 roster，调用 `GameSimulation::setPlayers()` 生成同样数量的玩家飞机（1 架时保持原单机行为，2–4 架时按 slot 使用不同出生点/颜色）。每个 TCP 会话的 `Input` 只应用到自己的飞机：`handleInput()` 先校验 `session.playerId`、`inMatch` 和 `simulation->hasPlayer()`，观战者/未知会话直接忽略；`InputCommand.playerId` 只供 stdio 宿主选择目标，TCP 客户端传什么都不会改变目标飞机。暂停/选强化是整局共享动作，只接受 leader（最小 active playerId）。`broadcastSnapshot()` 每帧只提取一次世界，编码前按会话把 `Snapshot.localPlayerId` 覆盖成各自的 playerId（观战者为 0），避免每个客户端复制整份世界。会话在对局中掉线时 `onDisconnected()` 调用 `GameSimulation::removePlayer()`，只把那一架飞机标记为非 active 并从后续快照移除，其他玩家继续；全部玩家都离开时模拟判负并结束对局。开局后才加入的会话保持自己的 playerId，但这一局没有飞机，是观战者；单机内嵌路径在握手完成前就 startMatch（roster 回退为 {1}），因此握手完成后允许尚未被占用的 slot 接管对应飞机。

`menu::Match` 默认是 `Mode::Network`：host 为空时在 127.0.0.1 上 `listen(0)` 启动内嵌 `GameServer` 并立即 `startMatch()`（单机/标准路径），`--connect host:port` 则连接远端并进入房间、等待房主开始；同一房间的 2–4 个会话各自控制自己的飞机（颜色/出生点不同），开局后加入的会话是观战者。`Mode::Host` 是自定义“局域网主机”：在 `0.0.0.0:<port>` 上开房、只创建 `GameClient` 回环连接，不调用 `startMatch()`；房主在 `Room` 按 `ENTER` 时 `Match::startRoomMatch()` 直接调用本地 `GameServer::startMatch()`（把当前所有 welcomed 会话组成 roster），按 `X` 时 `Match::dissolveRoom()` 关闭服务端并通知客户端。`Mode::Local` 保留本地直连 `Games` 路径，供 `SelfTest`、`--local` 回退和自定义单机直接开局。`--server --port N` 在 `QCoreApplication` 里只启动 `GameServer`，没有窗口/字体/音频。

`net::Snapshot` 的全部结构默认生成 `operator==`，roundtrip 测试逐字段比较；`serializeSnapshot / encodeSnapshotMessage` 用 `QDataStream`（小端、Qt 6 版本），长度字段和版本字段都由 `Protocol.h` 的辅助函数校验。TCP 集成测试覆盖 Hello/Welcome、宿主本地 StartMatch、输入命令、权威位移、快照重建、双客户端广播、端口占用、协议版本不匹配、断线与重连调度；TCP StartMatch 被拒绝。多玩家测试覆盖：两个会话在开局前各自拿到不同 playerId/slot，开局后快照同时包含两架飞机且每个客户端的 `localPlayerId`/`GameView::playerState()` 指向自己的飞机；只有发送者的飞机移动，另一方不动；开局后加入的会话 `localPlayerId==0`，其输入被忽略；一个玩家对局中掉线只移除那一架飞机、其余玩家继续，服务器不崩溃；直接调用 `GameSimulation` 的测试覆盖不同出生点/颜色、未知 playerId 忽略、序列化后的多玩家快照、阵亡等待复活、全灭判负和单机路径不变。房间测试另外覆盖：主机 `ENTER` 只进房不开局、服务端权威人数与成员槽位、加入者显示同一房间、房主 `ENTER` 本地开局、双方进入对局、解散关闭服务端并让加入者进入带重试/返回的失败页，以及连接不存在的服务器时显示“无法连接服务器”。

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
       └─ src/include/Data/GameData.h：inline constexpr auto doc = json::parse(...)
            └─ data::kPlayer / kEnemies / kCampaign / kWaveTiming / kUpgradeRanges / k*Bullet
                 └─ 游戏代码直接读常量（零运行时解析开销）
```

- 解析器：`src/lib/Json.h`，`constexpr` 递归下降，支持对象、数组、字符串、数字、true/false/null；
- `build/generated/GameDataJson.h` 由 `tools/embed_json.cpp` 生成，Makefile 自动重建；JSON 改动会通过 `.d` 依赖触发包含它的 TU 重新编译；
- 数据约束：字符串不使用转义、数字不使用指数；固定容量由 `GameData.h` 的 `Max*` 常量决定（超出会在编译期报错或截断）；
- `GameData.h` 提供 `data::validationError()`：启动时由 `main` 显式检查，数据不合法会打印具体错误并以非零码退出；
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

`Enemy` 是抽象基类：只保留共享状态、通用移动积分、受击/死亡/绘制骨架。类型身份用 RTTI：`arena::EnemyType = std::type_index`，每个具体敌人在构造时把 `typeid(Self)` 传给基类，`Enemy` 里没有分类字符串字段。`Enemies/EnemyTypes.h` 定义六个虚基类，`Enemies/<Kind>Enemy.cpp` 是每个具体类型的实现：

| 虚基类 | 实现 |
| --- | --- |
| `OffensiveEnemy` | drone / mini / rusher |
| `RangedEnemy` | spitter / summoner |
| `DefensiveEnemy` | tank / linker |
| `SupportEnemy` | amplifier / mender / necromancer |
| `SelfDestructEnemy` | splitter（死亡分裂） |
| `BossEnemy` | boss |

数值（含光环参数）全部来自 `src/json/enemies.json`，按 `EnemyType` 取；JSON 里的类型键只在注册和数据边界使用。击杀触发在 `Events::handleKill()`，由各敌人虚函数 `onKilled()` / `onAllyKilled()` 接管；掉落类型等分支用 `dynamic_cast` 判断具体类，不比较字符串：

| 类型（JSON 键） | 机制 |
| --- | --- |
| `amplifier` | 150 范围内友军伤害 +30%，每帧刷新 0.35s buff；敌方子弹伤害和接触伤害都吃加成 |
| `linker` | 170 范围内友军受到的 45% 伤害转给自己；只转一层，多个 linker 均分 |
| `mender` | 每秒为 150 范围内友军恢复 7 HP |
| `rusher` | 300 速度直冲，每 1.6s 以 2.3 倍速度突进 |
| `necromancer` | 220 范围内友军阵亡时 30% 概率半血复活；复活体不再连锁复活，分数减半 |
| `summoner` | 每个常规友军阵亡积累 +1 进度（mini 不计入），满 3 时在自身周围召唤 3 只 mini |

支援型敌人的光环半径画在 `Enemy::draw()`，被增伤的敌人有一圈琥珀色提示；伤害分担通过 `Damage.shared` 防止递归。

### 寻路与阵型

类别归属用 `Enemy::Archetype` 缓存（`Offensive / Ranged / Defensive / Support / SelfDestruct / Boss`），由虚基类构造时写入，避免每帧 RTTI；`Enemy::Role` 只决定是否需要弹幕闪避：

| 类型 | 成员 | 走位 |
| --- | --- | --- |
| 进攻型 | drone / mini / rusher | 直冲玩家，以接触为目标（rusher 额外周期突进） |
| 自毁型 | splitter | 直冲玩家，死亡时分裂（后续自毁敌人也在 `SelfDestructEnemy` 下直冲） |
| 远程型 | spitter / summoner | 在没有防御线时保持射程；有防御者且防御者更靠前时，移动到防御者背后（玩家反方向 42px）；同时保留 0.12s 弹幕侧闪 |
| 防御型 | tank / linker | 计算远程/辅助友军的质心，站到“质心与玩家之间”（45% 处），不主动贴玩家 |
| 辅助型 | amplifier / mender / necromancer | 优先躲到最近防御者背后；朝“非自毁型”友军最密集点靠拢（150 半径邻居数），并只考虑防御线后方的友军；贴玩家过近时后退 |
| Boss | boss | 独立走位：保持 185–265 距离、绕圈、定向弹幕 + 环形弹幕 |

辅助函数：`avoidPlayerBullets()` 只读 `Bullets::playerBullets()`；`nearestDefender()` / `densestAlly()` / `behindDefender()` 在 `EnemyTypes.cpp` 内按 `Archetype` 读 `Combatants::enemies()`，不引入新的组件依赖。

### 注册与新增敌人

注册走 CRTP，键是 RTTI 类型：

```cpp
namespace {
const EnemyRegistrar<DroneEnemy> kRegistration("drone", Base::Coral);
}
```

- 每个 `*Enemy.cpp` 定义自己的静态注册器，构造时写入 `enemyRegistry()`（`std::unordered_map<EnemyType, EnemyDef>`）；`EnemyDef` 保存颜色、工厂和仅用于 JSON/台本的 `dataKey`；
- `Enemy::create(EnemyType, pos, wave)` 按类型查表，未注册返回 `nullptr`；`Enemy::create(QString)` 只在数据/测试边界把 `dataKey` 解析成类型；
- 台本在数据边界完成解析：`Menu::standardScript()` 和 `Editor` 把 JSON/编辑器键解析成 `EnemyType`，`games::EnemySpawn` 只存类型；`Waves::buildQueue()` 直接用类型排队，Boss 判定用 `Enemy::isBoss(type)`，传送门也不再保存 kind 字符串；
- `Enemy.cpp` 不再有按 kind 的走位/绘制 `switch`，只保留基类逻辑、`statsFor(EnemyType)` 和注册表；
- 新增敌人：`enemies.json` 加数值 → 在 `Enemies/` 新建 `XEnemy.h/.cpp`，继承对应虚基类并写走位/绘制 → 在 cpp 里放 `EnemyRegistrar` → Makefile 的 `ARENA_COMPONENTS` 加对象 → 编辑器敌人库（`Menu/Editor/Editor.cpp` 的 `kKeys/kLabels/kindColor`）加一项 → 波次里使用。

## 依赖方向

```text
main() ──► Base::initialize()（字体） + ShopService.load()
GameWindow ──► BaseService.pointer*（指针位置/时钟/绘制）
            │
            ├─ BaseService ────────► base::Input / Audio / Music / Pointer / Paint / ThreadPool（全局单例）
            ├─ RandomService ──────► 随机引擎（全局单例）
            ├─ ShopService ────────► shop::Score / Inventory（全局单例，注册表 / ini）
            └─ Menu（持有四个关卡表，组合 10 个界面组件）
               ├─ menu::Main ─────────► 关卡卡 / 商店与设置入口（数据由 Menu 提供）
├─ menu::Map ──────────► 星区地图节点（levels.json），Start → 开局
               ├─ menu::Room ────────► 服务端权威房间（房主开始/解散、加入者等待）
               ├─ menu::Failure ─────► 无法连接服务器 / 房间已解散（重试/返回）
               ├─ menu::Match ────────► Network: client::GameClient ──► net::Snapshot / Event
               │  │                    │  ├─ client::GameView ──► net::Snapshot（重建）, Arena(只读镜像)
               │  │                    │  └─ games::Renderer ──► client::GameView(只读), RandomService
               │  │                    └─ 内嵌 server::GameServer ──► games::GameSimulation (headless)
               │  └─ Local: Games(Level)（测试/--local 回退）
               │     ├─ games::GameSimulation
               │     │  ├─ Arena ────► 私有继承 arena::Bullets / Combatants / Entities / Events / State
               │     │  │              （组件回调模块反馈走 arena::Host 虚接口）
               │     │  ├─ games::Waves ─────► Arena, RandomService
               │     │  └─ games::Upgrades ──► Arena, RandomService（音效走 BaseService）
               │     ├─ games::GameSimulation ──► net::Snapshot（提取只读世界）
               │     ├─ client::GameView ──────► net::Snapshot（重建）, Arena(只读镜像)
               │     └─ games::Renderer ──► client::GameView(只读), RandomService
               ├─ menu::Result ───────► 只读 Summary
               ├─ menu::Editor ───────► games::Script（自己持有）
               ├─ menu::Depot ────────► ShopService（buyMine）+ 只读钱包/库存
               └─ menu::Settings ─────► BaseService 音量接口 + 配置存储

--server 模式:
QCoreApplication ──► server::GameServer ──► games::GameSimulation ──► Arena / Waves / Upgrades
                                           ├─ net::Snapshot / Event / MatchEnd
                                           └─ ShopService.score()（本局统计）
```

依赖是单向的：

- `GameWindow` 持有 `Menu`，负责把画面和 `BaseService` 的指针合成到窗口上。
- `BaseService` / `RandomService` / `ShopService` 是叶子服务：只被调用，不反向依赖任何业务模块；需要它们的模块直接引用全局符号。
- `base::Pointer` 只依赖 `base::Paint` 的准星画法，不知道游戏业务。
- `Menu` 只做状态机、输入路由和关卡表；界面在 `menu::*` 组件里。`menu::Match` 默认持有 `client::GameClient`（以及单机/主机时的内嵌 `server::GameServer`），`--local`/测试时持有本地 `Games`；`Room` / `Failure` 只消费 `Match` 的只读房间/失败状态并把意图交回 `Menu`。
- `Games` 是单机适配器：只负责输入采集与表现，模拟在 `GameSimulation`；它不知道 `Menu`，一局的安排由构造参数进入，之后不再外泄。
- `GameSimulation` 只通过 `Arena` 公共接口访问战斗世界，不依赖 `Renderer` / `BaseService` 的键鼠状态；它只向外提供 `net::Snapshot`。`server::GameServer` 直接拥有它，不依赖 Renderer。
- `client::GameView` 只消费 `net::Snapshot`，不推进模拟、不读输入；`games::Renderer` 只读 `GameView`，不再画标题和结算。
- `net::Protocol / Input / Messages / Snapshot` 只依赖 Qt 和 `Math`，不依赖 Arena / Games / Renderer；`Net/Server` 负责把模拟接到 socket，`Net/Client` 负责把 socket 接到 GameView/Renderer。
- `Waves` / `Upgrades` 不知道 UI。
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
│  ├─ Room:   GameClient::update(dt) 推进连接/RoomList；收到首份快照 → Match
│  ├─ Failure: 只处理重试/返回，不推进对局
│  ├─ Match:  Network → GameClient::update(dt)
│  │          ├─ Renderer::update(dt)（暂停时不推进）
│  │          ├─ BaseService → net::InputCommand → QTcpSocket（仅在 Welcome 后）
│  │          └─ 收到的 Snapshot → GameView::apply；Event → Renderer/音效
│  │          Local → Games::update(dt)（测试/--local 回退路径）
│  │          ├─ Renderer::update(dt)
│  │          ├─ BaseService → arena::InputState → GameSimulation::setInput()
│  │          └─ Playing/Upgrade/Paused/Victory/Defeat 同前
│  └─ Result: menu::Match 继续 update 退场表现
│
│  server::GameServer（主线程 QTimer 或独立 --server 进程）
│  └─ 固定 60Hz：GameSimulation::update(1/60) → Snapshot/Event 广播 → MatchEnd
├─ BaseService.pointerUpdate(dt)   ← 指针独立时钟，暂停时也推进
├─ Menu::render()
│  ├─ Main:    menu::Main   绘制菜单
│  ├─ Map:     menu::Map    绘制星区地图
│  ├─ Editor:  menu::Editor 绘制台本表格
│  ├─ Shop:    menu::Depot  绘制补给站
│  ├─ Settings: menu::Settings 绘制设置页
│  ├─ Room:   menu::Room 绘制服务端房间/等待状态
│  ├─ Failure: menu::Failure 绘制无法连接/房间已解散
│  ├─ Match:  Network → GameClient::render() → Renderer::render(GameView)
│  │          Local → GameSimulation::writeSnapshot → GameView::apply → Renderer
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
| 战役波次组成、出怪节奏、波次成长系数 | `src/json/waves/level-01..04.json` |
| 战备发射器：伤害/射速/弹速/散射/额外弹丸/穿透 | `src/json/loadout.json` |
| 强化每轮的随机区间 | `src/json/upgrades.json` |
| 敌方子弹速度 / 伤害 / 半径 | `src/json/bullets.json` |
| 地雷价格 / 伤害 / 感应与爆炸半径 / 引爆延迟 / 寿命 | `src/json/items.json` |
| 星区地图节点位置 / 名称 / 是否解锁 | `src/json/levels.json` |

改完直接 `make`：JSON 会重新嵌成 raw string，包含数值表的 TU 自动重编译；格式写错会退化成默认值/空表，启动时由 `data::validationError()` 返回错误，`main` 打印并退出。

### 加强化

数值区间在 `src/json/upgrades.json`（按 key 的 `min`/`max`），其余在 `src/app/server/Games/Upgrades/`：

1. `src/json/upgrades.json` 加 `"key": {"min": ..., "max": ...}`（`min = max` 表示固定值，如 +1 穿透）；
2. `Upgrades::catalog()` 加名称/标签/颜色（数值会自动从 `data::findUpgrade()` 填）；
3. `Upgrades::eligible()` 加限制；
4. `Upgrades::describe()/valueLabel()` 加文案（卡面显示的数值就是实际生效值）；
5. `Upgrades::applyEffect()` 加效果，直接用 `offer.value`；
6. 新字段加到 `arena::PlayerState`。

`Renderer` 自动画卡片和右上角的 `offer.valueText`，`Games` 不用改。

### 加 / 改波次

只改 `src/json/waves/level-0N.json` 的 `campaign`：每一波是 `[{"kind": "...", "count": N}, ...]`，最后一波带 `boss` 即为 Boss 波；`Menu::standardScript(index)` 把它转成 `games::Script` 提交。每个关卡一个文件，`levels.json` 的节点用 `waves` 字段登记文件 id；新增文件后同步 Makefile 的嵌入参数、`GameData` 的文档常量和 `Menu` 的 `campaignLevels_`。

- 台本是数据，开局时作为 `Games::Level.script` 提交，`Waves` 只负责按它排队出生，不再有内置台本；
- 星区地图节点在 `src/json/levels.json`；把节点 `locked` 设为 `false` 前，需要先在关卡表里补上对应的标准关卡（目前只有第一个区域可用）；
- 出怪节奏（首波/后续延迟、间隔区间）在同一个文件的 `spawn` 段；
- 总波数 = 台本长度，HUD 通过 `Waves::totalWaves()` 读取，结算通过 `Games` 的快照读取；
- `boss` 出现在某一波里，那一波就是 Boss 波（`Waves::isBossWave()`），并且 Boss 永远第一个出场；击杀 Boss 不会结束对局，只有最后一波清空才胜利。

### 加关卡

改 `src/app/client/Menu/Menu.cpp` 的 `levels_`：

```cpp
{name, subtitle, startWave, script}
```

`Menu::startLevel()` 把这条 `Level` 交给 `menu::Match`，由 `Match` 构造 `Games`；`Games` 接收后即开局，并只在内部使用它。

“自定义协议”是同一套机制的动态版本：`levels_` 里只放展示信息，真正的台本在 `menu::Editor` 的 `script_` 里，初始只有一波，玩家在编辑器里定义波数和每波组成；`Menu::startLevel()` 在交给 `Match` 前把它写进 `Level.script`，`startWave` 固定为 1。

### 加一个界面 / 界面字段

每个屏幕都是一个 `menu::*` 组件（`src/app/client/Menu/<名字>/`）：

1. 组件自己持有 UI 状态、绘制和命中测试，用返回的 `Action` 表达意图；
2. `Menu` 的 `Screen` 枚举加一项，`render()` 里调它的 `render()`，输入路由里处理它的 `Action`；
3. 需要的数据由 `Menu` 组装后传入（如 `Main::Card`、`Result::Summary`），组件不反向依赖 `Menu`。

### 加一个结算字段

只改 `menu::Result::Summary` 和 `menu::Result::render()`；数据由 `Menu::summary()` 从 `menu::Match` 的只读 getter 组装，`Renderer` 不参与结算。

### 加敌人 / 状态 / 事件

- 敌人：数值在 `src/json/enemies.json`，颜色与行为在 `src/app/shared/Arena/Combatants/Enemy.*`，出生在 `Waves`；支援/特殊敌人的光环参数也在 `enemies.json`。
- 状态：`src/app/shared/Arena/State.*`。
- 碰撞/击杀：`src/app/shared/Arena/Events.*`。
- 实体：`src/app/shared/Arena/Entities/*`。

### 加弹种

1. 在 `src/include/Bullet.h` 之外、`src/app/shared/Arena/Bullets/` 里新建实现（参考 `StandardBullet`）；
2. 实现 `advance`（推进/寿命/出屏）、`draw`（样式）、`hit`（命中结算并返回是否继续存活）；
3. 发射处调用 `arena.submit(std::make_unique<YourBullet>(...))`，池会按 `standpoint()` 分流；
4. `Events` 的几何判定、`Arena` 的批量清理和绘制都不需要改。

### 加 IO / 音频 / 绘制原语

- 输入：`src/app/client/Base/Input/`。
- 线程池：`src/app/client/Base/ThreadPool/`（帧内并行；`BaseService.parallelFor` 转发）。
- 音效：`src/app/client/Base/Audio/Audio.*`；背景音乐：`src/app/client/Base/Audio/Music.*`。
- 字体/颜色/绘制：`src/app/client/Base/Paint/`。
- 鼠标指针：`src/app/client/Base/Pointer/`（位置 + 时钟 + 可见性，`BaseService` 转发）；准星画法在 `base::Paint::reticle()`。
- `Base` 只保留转发接口。

## 性能与并行

帧内渲染拆成“世界 / 辉光”两通道并行（见 `### Renderer`）：世界通道在 `ThreadPool` 工作线程写 `frame_`，辉光通道在主线程写 `glow_`，字体统一留在主线程，然后主线程合成。`base::Paint::Pass` 让同一批绘制调用按通道裁剪，所以两条通道不需要复制绘制代码。

可复现的基准（`--bench`，固定场景 + 300 帧 `render` + 600 帧 `update`）：

| 版本 | render | update |
| --- | --- | --- |
| 优化前（HEAD） | 10.1–13.2 ms/帧（同机交替中位 12.4） | ~0.03 ms/帧 |
| 优化后（并行） | **6.6–7.3 ms/帧（中位 7.0）** | ~0.006 ms/帧 |
| 优化后（`NEON_SINGLE_THREAD=1`） | 12.0–12.4 ms/帧 | ~0.008 ms/帧 |

同机交替 A/B 下渲染耗时 **−35% ~ −43%**（绝对值随机器负载浮动，以中位数为准）。逻辑更新本身只有 0.03 ms/帧，所以优化重点在渲染而非 AI/碰撞。

优化项：

- 世界/辉光双通道并行（线程池，主线程参与执行）；
- 绘制原语去掉逐次 `save/restore`，`glow` 合并为一次画笔状态；`Paint::Pass` 提前跳过反通道原语；
- 去掉多余的整帧图层：世界直接画进 `frame_`，字体单独在主线程补画；`background_` 不透明，省掉 `world_` 清屏/回贴；
- 标准弹对象池复用（`Bullets::spawnStandard` + `StandardBullet::reset`），稳态射击零堆分配；
- 敌人 `Archetype` 缓存替代每帧 `dynamic_cast`；`statsFor` 从线性扫描改为 `QHash` 下标表；`takeDamage` 先按 `Archetype::Defensive` 过滤 linker 扫描；
- `Combatants::draw` 排序缓冲改为线程内 `thread_local`，两个渲染通道各自复用；
- `QLocale` 等每帧构造的对象改为 `static`。

并发调试开关：

```text
NEON_SINGLE_THREAD=1     关闭线程池，退回单遍合并绘制
NEON_RENDER_THREADS=N    工作线程数（0–8，默认 min(核数-1, 3)）
```

GPU 加速没有采用：当前是 `QImage` + `QPainter` 光栅管线，走 GPU 需要重写为 `QRhi` / `QOpenGLWidget` 渲染器，并且会与 offscreen 截图/冒烟测试冲突；在本项目里多线程通道已经拿到主要收益。

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
- `NeonCoreConsole.exe --bench` → 固定场景渲染/更新基准（只在 `-DDEBUG` 构建里）。

## 已知边界

- 台本是运行时提交的 `games::Script` 数据，但默认战役仍在 `Menu` 里用 C++ 写（还没有外部 JSON/文件格式）。
- 自定义台本编辑器只改内存里的 `customScript_`，没有存档；重启回到默认战役台本。
- 敌人/强化/音效仍用字符串 ID。
- 一局一个 `Games`：`Base`（输入/音频/字体）、`Random`、`Shop` 都是进程级单例，否则切局会重复初始化音频/随机源/钱包。
- 一局的安排只经构造参数进入 `GameSimulation`；`Games` 不提供任何读回安排的接口，`Menu` 只保留展示用的关卡表。
- 模拟阶段 3：`Games` 仍保留为单机适配器（测试/`--local` 回退）；`menu::Match` 默认用 `GameClient` + 内嵌 `GameServer`，远端用 `--connect`。单机内嵌服务端与客户端在同一主线程的 Qt 事件循环里跑，真实 TCP 回环但不额外起线程。
- `net::Snapshot` 只含数据：玩家列表（playerId/颜色/复活倒计时）与每个接收者自己的 `localPlayerId`、敌人/传送门类型用注册表的 `dataKey` 字符串，`std::type_index` 不出现在协议里；帧格式是 `[u32 length][u16 type][u16 version][payload]`，序列化用 `QDataStream`。协议版本与快照数据版本都是 2（players[]/localPlayerId/Welcome.playerId/RoomList.members）。
- 服务端固定 60Hz tick 广播完整快照（目前不压缩、不插值）；单机回环会引入一个 tick/一帧左右的延迟，远端高延迟下的插值/预测留到阶段 4。
- `Player` 只读 `InputState`（`moveAxis / mouseActive / mousePosition / shooting / dashRequested`）；客户端把同样的字段封成 `net::InputCommand`，服务端校验后按会话的 playerId 写回 `GameSimulation::setInput(playerId, ...)`，冲刺/暂停/布雷/强化选择都是服务端权威动作。TCP 客户端不能指定 playerId；stdio 宿主可以用 `InputCommand.playerId` 选择飞机。
- 多玩家目前是 1–4 人的合作对局：积分/击杀仍是全局 `ShopService`（不按玩家拆分），强化是全队共享，暂停/选强化只由 leader（最小 active playerId）决定；阵亡后 3.2s 重新部署（阵亡期间不切换成自由观战），全灭才判负。开局后加入的会话只能观战；重连会得到新的 session/playerId，不会自动接管原来的飞机（没有可恢复令牌）。
- 房间目前是单房间模型（`RoomManager` 的 `rooms()` 最多一项），没有准备/踢人/玩家昵称；`RoomList.members` 只是会话槽位（sessionId、playerId、是否房主、是否已进入对局）。人数统计所有 welcomed 会话（主机自己的回环客户端也算 1 人），第 5 个连接会被拒绝；对局开始后加入的会话仍计入房间人数，但没有飞机。`RoomManager` 在服务端记录房主会话，房主离开时会把 host 交给下一个 welcomed 会话。房主解散后 `Match` 直接销毁内嵌服务端，加入者靠 `MatchEnd("房间已解散")` 或断线兜底进入 `Failure`。
- `menu::Match` 的加入远端路径使用手动重试（失败页 `ENTER`），不在后台自动重连；`GameClient` 本身仍保留可配置的 `setAutoReconnect()`，供独立测试/后续观战重连使用。
- 钱包/库存仍是阶段 3 的过渡状态：击杀和本局比分由服务端模拟写 `ShopService`（单机同进程所以立即可见）；远端客户端的本地钱包/地雷库存没有同步，服务端只接受布雷意图，阶段 4 再改成服务端权威并回执。
- `Arena` 的玩家状态对 `games::Upgrades` 开放写权限（`Arena::playerState()`），不再需要模块间 friend。
- `Arena` 的 5 个组件是它的私有基类：外部只看到 `Arena` 的公共接口；组件之间通过 `arena::Context` 注入的兄弟引用协作，组件 → 模块的反馈走 `arena::Host` 虚函数。
- `Arena` 只保留 `friend class SelfTest`（`-DDEBUG` 测试用来直接摆局面），没有业务 friend。
- 没有 ECS。当前 5 个 Arena 组件 + `GameSimulation` + 2 个 Games 组件的粒度更适合这个规模。
- `Menu` 只做界面/关卡创建，不读取 `Arena` 内部实体；它通过 `menu::Match` 的只读 getter 获取对局信息，网络模式下这些值来自最新快照/`MatchEnd`。
