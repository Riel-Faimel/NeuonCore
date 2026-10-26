#pragma once

#include "Arena/Bullets/Bullets.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Events.h"
#include "Arena/State.h"

#include <QColor>
#include <QPainter>

class Base;
class Random;
class Score;
class SelfTest;

namespace games {
class Upgrades;
}

// Arena 是全局模块类，只组合 5 个组件：
// arena::Bullets / Combatants / Entities / Events / State。
class Arena {
public:
    using PlayerState = arena::PlayerState;
    using BossInfo = arena::BossInfo;

    struct Feedback {
        bool hasFlash = false;
        QColor color;
        int alpha = 0;
        float shake = 0.0f;
    };

    Arena(Random &random, Base &base, Score &score);
    ~Arena();

    Arena(const Arena &) = delete;
    Arena &operator=(const Arena &) = delete;

    void reset();
    void beginWave();
    void update(float dt);
    void draw(QPainter &world, QPainter &glow) const;

    void spawnPortal(const QString &kind, int wave);
    void placeMine(const Math::Vec2 &position);
    int mineCount() const { return entities_.mineCount(); }

    const PlayerState &player() const { return combatants_.player().state; }
    int score() const;
    int kills() const;
    int aliveEnemyCount() const { return combatants_.aliveEnemyCount(); }
    int portalCount() const { return entities_.portalCount(); }
    bool playerDefeated() const { return state_.playerDefeated(); }
    bool bossDefeated() const { return state_.bossDefeated(); }
    BossInfo bossInfo() const { return combatants_.bossInfo(); }

    void updateVisuals(float dt);
    void clearEnemyBullets();
    void clearProjectiles();
    void clearForVictory();
    void requestDash();
    void celebrateUpgrade();
    void celebrateVictory();
    Feedback takeFeedback();

    // 子弹命中时要用同一个随机源（暴击判定）。
    Random &random() { return random_; }

private:
    friend class SelfTest;
    friend class games::Upgrades;
    friend class arena::Player;
    friend class arena::Enemy;
    friend class arena::Combatants;
    friend class arena::Entities;
    friend class arena::Events;
    friend class arena::State;
    friend class arena::Portal;
    friend class arena::Pickup;

    PlayerState &editablePlayer() { return combatants_.player().state; }

    void addFeedback(const QColor &color, int alpha, float shake);
    void drawBorder(QPainter &world) const;

    Random &random_;
    Base &base_;
    Score &score_;

    arena::Bullets bullets_;
    arena::Combatants combatants_;
    arena::Entities entities_;
    arena::Events events_;
    arena::State state_;
    Feedback feedback_;
};
