#include "Kill.h"

#include "Arena/Arena.h"
#include "Arena/Enemy.h"
#include "Random.h"
#include "base/Base.h"

#include <memory>

namespace arena {

void Kill::handle(Arena &arena, Enemy &enemy)
{
    if (!enemy.alive) {
        return;
    }
    enemy.alive = false;
    arena.score_.addKill(enemy.score);
    arena.spawnBurst(enemy.pos, enemy.color, enemy.boss ? 32 : 14,
                     enemy.boss ? 240.0f : 150.0f, 0.65f);
    arena.addShockwave(enemy.pos, enemy.boss ? 170.0f : 70.0f, enemy.color, 0.45f, 2);
    arena.base_.playSound(QStringLiteral("kill"));

    if (enemy.kind == QStringLiteral("splitter")) {
        for (int side = -1; side <= 1; side += 2) {
            const Math::Vec2 offset(float(side) * 18.0f,
                                    arena.random_.real(-12.0f, 12.0f));
            auto child = std::make_unique<Enemy>(QStringLiteral("mini"),
                                                 enemy.pos + offset, enemy.wave,
                                                 arena.random_);
            arena.spawnBurst(child->pos, child->color, 6, 90.0f, 0.3f);
            arena.enemies_.push_back(std::move(child));
        }
    }

    if (enemy.boss) {
        arena.base_.playSound(QStringLiteral("boss"));
        for (int i = 0; i < 4; ++i) {
            arena.spawnPickup(enemy.pos + Math::Vec2(arena.random_.real(-55.0f, 55.0f),
                                                     arena.random_.real(-55.0f, 55.0f)), 20);
        }
        arena.bossState_.markDefeated();
        return;
    }

    float dropChance = 0.06f;
    if (enemy.kind == QStringLiteral("drone")) {
        dropChance = 0.055f;
    } else if (enemy.kind == QStringLiteral("mini")) {
        dropChance = 0.03f;
    } else if (enemy.kind == QStringLiteral("spitter")) {
        dropChance = 0.10f;
    } else if (enemy.kind == QStringLiteral("tank")) {
        dropChance = 0.18f;
    } else if (enemy.kind == QStringLiteral("splitter")) {
        dropChance = 0.14f;
    }
    if (arena.random_.real(0.0f, 1.0f) < dropChance) {
        arena.spawnPickup(enemy.pos, enemy.kind == QStringLiteral("tank") ? 22 : 16);
    }
}

} // namespace arena
