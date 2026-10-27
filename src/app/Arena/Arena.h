#pragma once

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
    void setInput(const arena::InputState &input);
    void reset();
    void beginWave();
    void update(float dt);
    void draw(QPainter &world, QPainter &glow) const;
    // 飘字层（字体渲染）由 Renderer 在主线程单独绘制。
    void drawTexts(QPainter &world) const { Entities::drawTexts(world); }

    void spawnPortal(arena::EnemyType type, int wave, int enemyLevel = 1);
    void placeMine(const Math::Vec2 &position);

    // ----- 只读查询 -----
    const PlayerState &playerState() const { return player().state; }
    PlayerState &playerState() { return player().state; }
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
    void requestDash();
    void celebrateUpgrade();
    void celebrateVictory();
    Feedback takeFeedback();

private:
    friend class SelfTest;

    // arena::Host：组件请求模块级画面反馈。
    void addFeedback(const QColor &color, int alpha, float shake) override;

    // 模块内部：组件之间互相访问用的上下文。只有 Arena 能构造。
    arena::Context context() const;

    void drawBorder(QPainter &world) const;

    Feedback feedback_;
};
