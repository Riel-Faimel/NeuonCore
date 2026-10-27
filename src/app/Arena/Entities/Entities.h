#pragma once

#include "Arena/Entities/FloatingText.h"
#include "Arena/Entities/Mine.h"
#include "Arena/Entities/Particle.h"
#include "Arena/Entities/Pickup.h"
#include "Arena/Entities/Portal.h"
#include "Arena/Entities/Shockwave.h"

#include <QColor>
#include <QPainter>

#include <vector>

namespace net {
struct Snapshot;
}

namespace arena {

struct Context;

// 实体组件：地雷、传送门、掉落、粒子、飘字、冲击波（子弹在 arena::Bullets）。
class Entities {
public:
    void reset();

    void updatePortals(const Context &ctx, float dt);
    void updatePickups(const Context &ctx, float dt);
    void updateMines(float dt);
    void updateEffects(const Context &ctx, float dt);
    void cleanup();

    void drawBackground(QPainter &world, QPainter &glow) const;
    void drawForeground(QPainter &world, QPainter &glow) const;
    // 飘字单独一层：字体渲染留在主线程，世界层可以放到工作线程。
    void drawTexts(QPainter &world) const;

    void spawnPortal(const Context &ctx, EnemyType type, int wave,
                     int enemyLevel = 1);
    void spawnPickup(const Context &ctx, const Math::Vec2 &pos, int amount);
    void spawnMine(const Context &ctx, const Math::Vec2 &pos);
    void clearMines();
    void spawnBurst(const Context &ctx, const Math::Vec2 &pos, const QColor &color,
                    int count, float speed, float life);
    void addShockwave(const Math::Vec2 &pos, float radius, const QColor &color,
                      float duration = 0.4f, int width = 2);

    // ----- 网络快照（阶段 2） -----
    void writeSnapshot(net::Snapshot &out) const;
    void applySnapshot(const net::Snapshot &snapshot);

    int portalCount() const { return int(portals_.size()); }
    int mineCount() const { return int(mines_.size()); }
    float &effectsTimer() { return effectsTimer_; }

    std::vector<Portal> &portals() { return portals_; }
    std::vector<Pickup> &pickups() { return pickups_; }
    std::vector<Mine> &mines() { return mines_; }
    std::vector<Particle> &particles() { return particles_; }
    std::vector<FloatingText> &floatingTexts() { return floatingTexts_; }
    std::vector<Shockwave> &shockwaves() { return shockwaves_; }

    const std::vector<Portal> &portals() const { return portals_; }
    const std::vector<Pickup> &pickups() const { return pickups_; }
    const std::vector<Mine> &mines() const { return mines_; }
    const std::vector<Particle> &particles() const { return particles_; }
    const std::vector<FloatingText> &floatingTexts() const { return floatingTexts_; }
    const std::vector<Shockwave> &shockwaves() const { return shockwaves_; }

private:
    Math::Vec2 randomSpawnPosition(const Context &ctx) const;

    std::vector<Portal> portals_;
    std::vector<Pickup> pickups_;
    std::vector<Mine> mines_;
    std::vector<Particle> particles_;
    std::vector<FloatingText> floatingTexts_;
    std::vector<Shockwave> shockwaves_;
    float effectsTimer_ = 0.0f;
};

} // namespace arena
