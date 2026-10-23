#pragma once

#include "Arena/BossState.h"
#include "Arena/Bullet.h"
#include "Arena/Collision.h"
#include "Arena/Enemy.h"
#include "Arena/FloatingText.h"
#include "Arena/Kill.h"
#include "Arena/Particle.h"
#include "Arena/Pickup.h"
#include "Arena/Player.h"
#include "Arena/Portal.h"
#include "Arena/Score.h"
#include "Arena/Shockwave.h"

#include <QColor>
#include <QPainter>

#include <memory>
#include <vector>

class Base;
class Random;
class SelfTest;
class Upgrades;

// Arena 是全局模块类，组合 namespace arena 中的战斗者、实体、事件和状态组件。
// 外部模块只看到 Arena 的公共接口；组件类型不会从 Game / Renderer 直接接触。
class Arena {
public:
    using PlayerState = arena::PlayerState;

    struct BossInfo {
        bool active = false;
        float hp = 0.0f;
        float maxHp = 0.0f;
        float hpDisplay = 0.0f;
    };

    struct Feedback {
        bool hasFlash = false;
        QColor color;
        int alpha = 0;
        float shake = 0.0f;
    };

    explicit Arena(Random &random, Base &base);
    ~Arena();

    Arena(const Arena &) = delete;
    Arena &operator=(const Arena &) = delete;

    void reset();
    void beginWave();
    void update(float dt);
    void draw(QPainter &world, QPainter &glow) const;

    void spawnPortal(const QString &kind, int wave);

    const PlayerState &player() const { return player_.state; }
    int score() const { return score_.score(); }
    int kills() const { return score_.kills(); }
    int aliveEnemyCount() const;
    int portalCount() const;
    bool playerDefeated() const { return playerDefeated_; }
    bool bossDefeated() const { return bossState_.defeated(); }
    BossInfo bossInfo() const;

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
    friend class Upgrades;
    friend class arena::Player;
    friend class arena::Enemy;
    friend class arena::Portal;
    friend class arena::Pickup;
    friend class arena::Collision;
    friend class arena::Kill;
    friend class arena::BossState;

    PlayerState &editablePlayer() { return player_.state; }

    void updatePortals(float dt);
    void updateEnemies(float dt);
    void updatePickups(float dt);
    void updateEffects(float dt);
    void cleanup();

    void spawnEnemy(const QString &kind, const Math::Vec2 &pos, int wave);
    void spawnMinions(const Math::Vec2 &pos, int count, int wave);
    void spawnPickup(const Math::Vec2 &pos, int amount);
    void spawnBurst(const Math::Vec2 &pos, const QColor &color, int count,
                    float speed, float life);
    void addShockwave(const Math::Vec2 &pos, float radius, const QColor &color,
                      float duration = 0.4f, int width = 2);
    void damageEnemiesInRadius(const Math::Vec2 &center, float radius, float damage,
                               arena::DamageSource source = arena::DamageSource::Shock);
    Math::Vec2 randomSpawnPosition();
    void addFeedback(const QColor &color, int alpha, float shake);

    void drawBorder(QPainter &world) const;

    arena::Player player_;
    std::vector<std::unique_ptr<arena::Enemy>> enemies_;
    std::vector<arena::Bullet> playerBullets_;
    std::vector<arena::Bullet> enemyBullets_;
    std::vector<arena::Portal> portals_;
    std::vector<arena::Pickup> pickups_;
    std::vector<arena::Particle> particles_;
    std::vector<arena::FloatingText> floatingTexts_;
    std::vector<arena::Shockwave> shockwaves_;

    arena::Collision collision_;
    arena::Kill kills_;
    arena::BossState bossState_;
    arena::Score score_;

    arena::Enemy *boss_ = nullptr;
    bool playerDefeated_ = false;
    float effectsTimer_ = 0.0f;
    Feedback feedback_;

    Random &random_;
    Base &base_;
};
