# 客户端 / 服务端拆分计划

目标：业务逻辑（Arena / Waves / Upgrades / 计分 / 敌人 AI）只运行在服务端；渲染、菜单、输入、音频、窗口留在客户端；两边通过 TCP 端口通信，为后续联机留出权威服务器和状态同步的接口。

这篇文档先定边界和迁移顺序，不直接改运行代码。当前 `local` 单机路径保持可玩，迁移按阶段进行，每阶段都能构建、能跑 smoke。

> 状态：阶段 0–3 已完成，并补上了房间/大厅生命周期与真实多玩家会话（协议 v2）。阶段 3 的实现是 `src/app/server/Net/Server.*`（`server::GameServer`，headless `GameSimulation` + 60Hz tick + `QTcpServer`）和 `src/app/client/Net/Client.*`（`client::GameClient`，`QTcpSocket` + 握手/输入/快照/`GameView`/`Renderer`），协议/快照在 `src/app/shared/Net/`，消息在 `include/Net/Input.h`、`Messages.h`、`Snapshot.h`。`menu::Match` 默认走网络：单机在 127.0.0.1 上启动自动端口的内嵌服务端并立即开局，`--connect host:port` 连远端并进入 `Room`。自定义协议的“局域网主机”启动内嵌服务端后只开房，房主在 `Room` 按 ENTER 才本地 `startMatch()`；按 X 解散会关闭服务端并让加入者看到专用失败页。多玩家：Hello 分配稳定 playerId/slot（1–4），startMatch 把当前 welcomed 会话组成 roster，生成 1–4 架玩家飞机；每个会话输入只作用于自己的飞机，快照携带 `players[]` 和每个客户端的 `localPlayerId`，中途加入为观战、掉线只移除该飞机。与本文原计划的差异：内嵌服务端与客户端在同一 Qt 事件循环（真实 TCP 回环，不额外起线程）；本地 `Games` 路径保留为 `--local`/自动化测试回退，等阶段 4 再删除。

## 1. 当前耦合点

- `Games` 同时拥有 `Arena`、`Waves`、`Upgrades`、`Renderer`，并在 `update()` 里同时推进模拟和表现；
- `games::Renderer` 直接读 `Arena / Waves / Upgrades / Games` 的只读状态，绘制世界层、HUD、Boss 条、强化卡、横幅；
- `Arena::draw()` 直接调用各实体/敌人的 `draw()`，表现和实体类绑定；
- `Player::update()` 直接读全局 `BaseService` 的键鼠状态；
- 钱包/库存/战备通过 `ShopService` / `LoadoutService` 读写本地配置；
- `Menu` 通过 `Main / Map / Match / Result` 管理屏幕状态机，`Match` 直接持有 `Games`。

## 2. 目标结构

```text
src/
├─ include/Net/
│  ├─ Protocol.h        消息头、消息 id、版本号、长度前缀
│  ├─ Snapshot.h        服务端 -> 客户端的只读世界快照
│  └─ Input.h           客户端 -> 服务端的输入命令
├─ server/
│  ├─ GameSimulation.*  只含 Arena / Waves / Upgrades / 计分，不依赖 Renderer
│  ├─ GameServer.*      TCP 监听、会话、固定 tick、广播快照
│  └─ main_server.cpp   独立服务器入口
├─ client/
│  ├─ GameView.*        快照 -> 可渲染的视图状态（Arena 的只读镜像）
│  ├─ Renderer.*        从 Games 移到 client，只读 GameView
│  ├─ GameClient.*      TCP 连接、输入发送、快照插值
│  └─ Match.*           客户端对局控制器（替代现在 Menu::Match）
└─ app/                 菜单、商店、战备、编辑器等纯客户端界面
```

## 3. 协议

TCP，默认端口 `7777`。消息统一：

```text
[u32 length][u16 type][u16 version][payload]
```

消息类型：

| id | 方向 | 内容 |
| --- | --- | --- |
| `Hello` | C→S | 协议版本、客户端版本 |
| `Welcome` | S→C | 会话 id、tick rate、是否接受加入 |
| `StartMatch` | C→S | 关卡 id / 自定义台本（类型用数据 key，不用 `type_index`） |
| `Input` | C→S | 按键、鼠标位置、鼠标键、冲刺、布雷 |
| `Snapshot` | S→C | 世界快照（见下） |
| `Event` | S→C | 音效、横幅、震屏/闪屏等表现事件 |
| `MatchEnd` | S→C | 胜负、统计、可重试 |

快照内容（只读，不含业务对象指针）：

- 玩家 `PlayerState`；
- 敌人：类型数据 key、wave、hidden level、位置/速度/半径/血量/年龄/支援光环/充能等绘制所需字段；
- 子弹：位置/速度/颜色/半径/阵营；
- 实体：传送门、地雷、掉落、粒子、飘字、冲击波；
- 波次：当前/总数/剩余威胁/Boss 波；
- 强化：本轮三张卡（展示字段）；
- 计分：服务端钱包、本局获得、击杀、运行时间；
- 对局状态：Playing/Upgrade/Paused/Victory/Defeat。

客户端不向服务端回传世界状态；服务端不接受客户端位置。

## 4. 迁移阶段

### 阶段 0：共享协议和快照

- 新增 `include/Net/Protocol.h`、`Snapshot.h`、`Input.h`；
- 定义 `QDataStream` 读写；所有 `type_index` 在数据边界转成 `EnemyDef::dataKey`；
- 加快照 roundtrip 测试：构造 `Snapshot` → 序列化 → 反序列化 → 字段相等。

验收：不改游戏行为，构建通过，roundtrip 测试通过。

### 阶段 1：把模拟从 Games 里抽出来

- 新增 `server::GameSimulation`：持有 `Arena / Waves / Upgrades / score`，提供 `startLevel / update / applyInput / snapshot / state`；
- `Games` 退回为“单机适配器”：内部持有 `GameSimulation + Renderer`，行为保持不变；
- 输入通过 `InputCommand` 进入 `GameSimulation`，不再由 `Player` 直接读 `BaseService`；
  - `Player` 增加 `InputState`（move axis、mouse active/position、keys）；
  - `BaseService` 只在客户端使用。

验收：单机 smoke 全过；`GameSimulation` 可以无 Renderer 更新。

### 阶段 2：GameView + 客户端 Renderer

- 新增 `client::GameView`：`Arena` 的只读镜像，可由 `Snapshot` 重建；
  - 复用现有 `Enemy / Bullet / Entity` 的绘制代码：按数据 key 重建对象，再填充绘制字段；
  - `GameView` 不推进模拟，只更新表现动画（横幅、星光、插值）；
- `Renderer` 从 `Games` 改为读 `GameView`；
- 单机仍走本地 `GameSimulation -> Snapshot -> GameView`，先验证视觉一致，再切网络。

验收：本地快照渲染截图与当前 `screenshots/` 对比无明显差异；smoke 可改为“模拟 + 视图”两步。

### 阶段 3：TCP 服务端和客户端

- `server::GameServer`：`QTcpServer`、固定 tick（60Hz）、每个连接一个会话、广播快照；
- `client::GameClient`：`QTcpSocket`、握手、输入发送、快照接收、断线重连；
- 单机模式：客户端启动时在后台线程起一个本地服务端（端口 0 自动分配），客户端连 `127.0.0.1`；这样单机和联机走同一条代码路径；
- 联机模式：`--connect host:7777`；服务端可独立启动 `--server --port 7777`。

验收：本地端口回环下完整打通一局；两个客户端可同时连接一个服务端。

### 阶段 4：权威性、表现和存档

- 钱包/库存/战备改为服务端权威：客户端只发购买/装备意图，服务端校验后回执；
- 音效/横幅/震屏改为 `Event` 消息，由客户端播放；
- 快照压缩和插值：tick 30/60Hz、客户端渲染插值、丢包/延迟处理；
- 断线、暂停、观战、房间/会话列表。

验收：客户端无法通过改本地内存获得资源；延迟 100ms 下可玩。

### 阶段 5：多人

- 多会话、玩家 id、准备/开始、队伍（**已完成基础版**：稳定 playerId/slot、1–4 人 roster、每会话一架飞机、输入按 playerId 隔离、快照 players[]/localPlayerId、离开移除飞机、全灭判负；准备/队伍/昵称仍未实现）；
- 输入缓冲、服务器 tick 与客户端 tick 对齐；
- 反作弊边界（服务器只接受输入命令）。

## 5. 关键决策

- 服务端不链接 `Renderer`、`Menu`、`BaseService`（除音频事件外）；
- 快照只含数据，不含 Qt 绘制对象；
- 敌人/子弹/实体类型在协议里用 `dataKey` 字符串，客户端查注册表重建；
- 服务端固定 tick，客户端表现层可以插值，不把渲染帧率绑到模拟 tick；
- 单机 = 本地回环服务端，避免“单机一套、联机一套”两条逻辑；
- 当前 `local` 路径在阶段 1–2 保留，作为回退和测试基线，阶段 3 完成后删除。

## 6. 测试清单

- 快照 roundtrip（字段逐项相等）；
- `GameSimulation` 无渲染运行 8 波，结果与当前 `autoplay` 一致；
- 快照重建后的 `GameView` 能画出玩家/敌人/子弹/实体；
- 本地回环一局完整跑通（开始、暂停、强化、胜负、重试）；
- 两个客户端连接同一服务端，输入互不串线（已覆盖：两个开局前会话各自移动自己的飞机；中途加入为观战）；
- 快照多玩家 roundtrip / 每客户端 localPlayerId / 掉线只移除对应飞机（已覆盖）；
- 断线重连、服务端关闭、客户端退出不崩溃；
- 端口占用、握手版本不匹配的错误路径。

## 7. 风险

- 现有 `Renderer` 和 `Arena` 耦合很深，阶段 2 是最大工作量；
- 表现事件（粒子/震屏/横幅）如果全部进快照会膨胀，应只发事件、客户端本地生成；
- 随机数必须服务端权威，客户端不能用 `RandomService` 决定伤害/掉落；
- 输入若继续走 `BaseService` 会阻碍远程客户端，阶段 1 必须先抽 `InputState`；
- 单机本地回环会引入一帧延迟，需要插值/预测或保留直连快路径（但两套路径会分叉，优先统一）。
