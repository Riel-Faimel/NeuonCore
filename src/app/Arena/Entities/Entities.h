#pragma once

#include "Arena/Entities/Bullet.h"
#include "Arena/Entities/FloatingText.h"
#include "Arena/Entities/Particle.h"
#include "Arena/Entities/Pickup.h"
#include "Arena/Entities/Portal.h"
#include "Arena/Entities/Shockwave.h"

#include <QColor>
#include <QPainter>

#include <vector>

class Arena;

namespace arena {

// 实体组件：子弹、传送门、掉落、粒子、飘字、冲击波。
class Entities {
public:
    void reset();
    void clearEnemyBullets();
    void clearProjectiles();

    void updatePortals(Arena &arena, float dt);
    void updatePickups(Arena &arena, float dt);
    void updateEffects(Arena &arena, float dt);
    void cleanup();
    void cleanupProjectiles();

    void drawBackground(QPainter &world, QPainter &glow) const;
    void drawForeground(QPainter &world, QPainter &glow) const;

    void spawnPortal(Arena &arena, const QString &kind, int wave);
    void spawnPickup(Arena &arena, const Math::Vec2 &pos, int amount);
    void spawnBurst(Arena &arena, const Math::Vec2 &pos, const QColor &color,
                    int count, float speed, float life);
    void addShockwave(const Math::Vec2 &pos, float radius, const QColor &color,
                      float duration = 0.4f, int width = 2);

    int portalCount() const { return int(portals_.size()); }
    float &effectsTimer() { return effectsTimer_; }

    std::vector<Bullet> &playerBullets() { return playerBullets_; }
    std::vector<Bullet> &enemyBullets() { return enemyBullets_; }
    std::vector<Portal> &portals() { return portals_; }
    std::vector<Pickup> &pickups() { return pickups_; }
    std::vector<Particle> &particles() { return particles_; }
    std::vector<FloatingText> &floatingTexts() { return floatingTexts_; }
    std::vector<Shockwave> &shockwaves() { return shockwaves_; }

private:
    Math::Vec2 randomSpawnPosition(const Arena &arena) const;

    std::vector<Bullet> playerBullets_;
    std::vector<Bullet> enemyBullets_;
    std::vector<Portal> portals_;
    std::vector<Pickup> pickups_;
    std::vector<Particle> particles_;
    std::vector<FloatingText> floatingTexts_;
    std::vector<Shockwave> shockwaves_;
    float effectsTimer_ = 0.0f;
};

} // namespace arena
