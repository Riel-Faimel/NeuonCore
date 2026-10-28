#pragma once

#include "Debug.h"

#include "Arena/Context.h"
#include "Arena/Bullets/Bullets.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Events.h"
#include "Arena/Host.h"
#include "Arena/State.h"

#include <QColor>
#include <QPainter>
#include <QString>

#include <vector>

namespace net {
struct Snapshot;
}

// Arena 是全局模块类，由 5 个组件构成——但用的是**私有继承**：
//   arena::Host / Bullets / Combatants / Entities / Events / State
//
// 封装规则：
// - 外部只能看到下面 public 区声明的模块接口；组件的任何方法都不对外暴露。
// - 组件之间通过 arena::Context 拿兄弟组件的引用（Arena 构造 Context 时注入），
//   既不需要 friend，也不持有回指指针。
// - 组件需要模块级反馈（闪屏/震屏）时，通过 arena::Host 的虚函数回调 Arena。
// 测试（-DDEBUG 的 SelfTest）通过 friend 直接摆弄内部组件。
class Arena : private arena::Host,
              private arena::Bullets,
              private arena::Combatants,
              private arena::Entities,
              private arena::Events,
              private arena::State {
public:
    using PlayerState = arena::PlayerState;
    using Player = arena::Player;
    using BossInfo = arena::BossInfo;

    struct Feedback {
        bool hasFlash = false;
        QColor color;
        int alpha = 0;
        float shake = 0.0f;
    };

    Arena();
    ~Arena();

    Arena(const Arena &) = delete;
    Arena &operator=(const Arena &) = delete;

    // ----- 输入 / 局内流程 -----
    // 一帧的玩家输入由 GameSimulation 通过这里注入；Player 不再直接读 BaseService。
    // setInput(input) 是单机路径（第一架飞机）；网络路径按 playerId 定向。
    void setInput(const arena::InputState &input);
    void setInput(int playerId, const arena::InputState &input);
    // 按服务端分配的一局玩家 id 重建玩家飞机；单机传入 {1}。
    void configurePlayers(const std::vector<int> &playerIds);
    void reset();
    void beginWave();
    void update(float dt);
    void draw(QPainter &world, QPainter &glow) const;
    // 飘字层（字体渲染）由 Renderer 在主线程单独绘制。
    void drawTexts(QPainter &world) const { Entities::drawTexts(world); }

    void spawnPortal(arena::EnemyType type, int wave, int enemyLevel = 1);
    void placeMine(const Math::Vec2 &position);

    // ----- 只读查询 -----
    // 本地视角的飞机（单机=唯一一架；网络客户端=localPlayerId 对应的飞机）。
    const PlayerState &playerState() const;
    PlayerState &playerState();
    const PlayerState &playerStateAt(int index) const;
    PlayerState &playerStateAt(int index);
    int playerIdAt(int index) const;
    int playerCount() const;
    int livingPlayerCount() const;
    bool hasPlayer(int playerId) const;
    int localPlayerId() const;
    void setLocalPlayerId(int playerId);
    // 会话离开：移除对应飞机（快照不再包含它），其他玩家继续。
    bool removePlayer(int playerId);
    BossInfo bossInfo() const { return Combatants::bossInfo(); }
    int score() const;
    int kills() const;
    int aliveEnemyCount() const { return Combatants::aliveEnemyCount(); }
    int portalCount() const { return Entities::portalCount(); }
    bool playerDefeated() const { return State::playerDefeated(); }
    bool bossDefeated() const { return State::bossDefeated(); }

    // ----- 表现 / 清理 -----
    void updateVisuals(float dt);
    void clearEnemyBullets();
    void clearProjectiles();
    void clearForVictory();
    void requestDash(int playerId = 0);
    void celebrateUpgrade();
    void celebrateVictory();
    Feedback takeFeedback();

    // ----- 网络快照（阶段 2） -----
    // 提取只读世界数据；应用把快照重建成可绘制的世界（不推进模拟）。
    void writeSnapshot(net::Snapshot &out) const;
    void applySnapshot(const net::Snapshot &snapshot);

private:
    DebugInfo(friend class SelfTest;)

    // arena::Host：组件请求模块级画面反馈。
    void addFeedback(const QColor &color, int alpha, float shake) override;

    // 模块内部：组件之间互相访问用的上下文。只有 Arena 能构造。
    arena::Context context() const;

    // 多玩家：推进阵亡飞机的重新部署倒计时；所有 active 飞机都倒下时判负。
    void updateRespawns(const arena::Context &ctx, float dt);
    bool allPlayersDefeated() const;
    const Player &localPlayer() const;

    void drawBorder(QPainter &world) const;

    Feedback feedback_;
};
