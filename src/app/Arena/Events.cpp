#include "Events.h"

#include "Arena/Arena.h"
#include "Arena/Bullets/Bullets.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Combatants/Enemy.h"
#include "Arena/Combatants/Player.h"
#include "Arena/State.h"
#include "Bullet.h"
#include "GameConfig.h"
#include "Random.h"
#include "Base/Base.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace arena {

void Events::resolvePlayerBullets(Arena &arena)
{
    for (auto &owned : arena.bullets_.player()) {
        Bullet &bullet = *owned;
        if (!bullet.alive()) {
            continue;
        }
        const size_t count = arena.combatants_.enemies().size();
        for (size_t i = 0; i < count; ++i) {
            if (i >= arena.combatants_.enemies().size()) {
                break;
            }
            Enemy *enemy = arena.combatants_.enemies()[i].get();
            Fighter &fighter = *enemy;
            if (!fighter.isAlive()) {
                continue;
            }
            if (!Math::segmentCircle(bullet.prev(), bullet.position(),
                                     fighter.position(),
                                     fighter.collisionRadius() + bullet.radius())) {
                continue;
            }
            // 命中结算（伤害/暴击/穿透）由子弹自己处理；返回 false 表示这颗弹结束。
            if (!bullet.hit(arena, fighter)) {
                break;
            }
        }
        if (arena.state_.playerDefeated()) {
            break;
        }
    }
    arena.bullets_.cleanup();
}

void Events::resolveEnemyBullets(Arena &arena)
{
    Player &player = arena.combatants_.player();
    if (!player.isAlive()) {
        arena.bullets_.cleanup();
        return;
    }
    for (auto &owned : arena.bullets_.enemy()) {
        Bullet &bullet = *owned;
        if (!bullet.alive()) {
            continue;
        }
        if (!Math::segmentCircle(bullet.prev(), bullet.position(),
                                 player.position(),
                                 player.state.radius + bullet.radius())) {
            continue;
        }
        if (player.state.invuln > 0.0f) {
            arena.entities_.spawnBurst(arena, bullet.position(), Base::Cyan, 5, 80.0f,
                                       0.25f);
        }
        bullet.hit(arena, player);
        if (arena.state_.playerDefeated()) {
            break;
        }
    }
    arena.bullets_.cleanup();
}

void Events::separateEnemies(Arena &arena)
{
    std::vector<Enemy *> alive;
    alive.reserve(arena.combatants_.enemies().size());
    for (const auto &enemy : arena.combatants_.enemies()) {
        if (enemy->isAlive()) {
            alive.push_back(enemy.get());
        }
    }
    for (size_t i = 0; i < alive.size(); ++i) {
        for (size_t j = i + 1; j < alive.size(); ++j) {
            Enemy *first = alive[i];
            Enemy *second = alive[j];
            const Math::Vec2 delta = first->pos - second->pos;
            const float minimum = first->radius + second->radius;
            const float distanceSquared = delta.lengthSquared();
            if (distanceSquared <= 0.001f || distanceSquared >= minimum * minimum) {
                continue;
            }
            const float distance = std::sqrt(distanceSquared);
            const Math::Vec2 push = delta / distance * ((minimum - distance) * 0.48f);
            if (!first->boss) {
                first->pos += push;
            }
            if (!second->boss) {
                second->pos -= push;
            }
            first->pos.x = Math::clamp(first->pos.x, GameConfig::FieldLeft + first->radius,
                                       GameConfig::FieldRight - first->radius);
            first->pos.y = Math::clamp(first->pos.y,
                                       GameConfig::FieldTop + first->radius + 4.0f,
                                       GameConfig::FieldBottom - first->radius - 4.0f);
            second->pos.x = Math::clamp(second->pos.x, GameConfig::FieldLeft + second->radius,
                                        GameConfig::FieldRight - second->radius);
            second->pos.y = Math::clamp(second->pos.y,
                                        GameConfig::FieldTop + second->radius + 4.0f,
                                        GameConfig::FieldBottom - second->radius - 4.0f);
        }
    }
}

void Events::contactDamage(Arena &arena)
{
    Player &player = arena.combatants_.player();
    if (!player.isAlive() || player.state.invuln > 0.0f) {
        return;
    }
    for (const auto &enemy : arena.combatants_.enemies()) {
        if (!enemy->isAlive()) {
            continue;
        }
        const Fighter &fighter = *enemy;
        const Math::Vec2 delta = player.position() - fighter.position();
        const float minimum = player.state.radius + fighter.collisionRadius();
        if (delta.lengthSquared() <= minimum * minimum) {
            player.takeDamage(arena, Damage{fighter.contactDamage(), false,
                                            DamageSource::Contact});
            if (delta.lengthSquared() > 0.001f) {
                const Math::Vec2 direction = delta.normalized();
                player.state.pos += direction * 22.0f;
                enemy->pos -= direction * 12.0f;
                player.state.pos.x = Math::clamp(player.state.pos.x,
                                                 GameConfig::FieldLeft + 2.0f,
                                                 GameConfig::FieldRight - 2.0f);
                player.state.pos.y = Math::clamp(player.state.pos.y,
                                                 GameConfig::FieldTop + 2.0f,
                                                 GameConfig::FieldBottom - 2.0f);
            }
            break;
        }
    }
}

void Events::handleKill(Arena &arena, Enemy &enemy)
{
    if (!enemy.alive) {
        return;
    }
    enemy.alive = false;
    arena.state_.addKill(enemy.score);
    arena.entities_.spawnBurst(arena, enemy.pos, enemy.color,
                               enemy.boss ? 32 : 14,
                               enemy.boss ? 240.0f : 150.0f, 0.65f);
    arena.entities_.addShockwave(enemy.pos, enemy.boss ? 170.0f : 70.0f,
                                 enemy.color, 0.45f, 2);
    arena.base_.playSound(QStringLiteral("kill"));

    if (enemy.kind == QStringLiteral("splitter")) {
        for (int side = -1; side <= 1; side += 2) {
            const Math::Vec2 offset(float(side) * 18.0f,
                                    arena.random_.real(-12.0f, 12.0f));
            auto child = std::make_unique<Enemy>(QStringLiteral("mini"),
                                                 enemy.pos + offset,
                                                 enemy.wave, arena.random_);
            arena.entities_.spawnBurst(arena, child->pos, child->color, 6, 90.0f, 0.3f);
            arena.combatants_.enemies().push_back(std::move(child));
        }
    }

    if (enemy.boss) {
        arena.base_.playSound(QStringLiteral("boss"));
        for (int i = 0; i < 4; ++i) {
            arena.entities_.spawnPickup(
                arena,
                enemy.pos + Math::Vec2(arena.random_.real(-55.0f, 55.0f),
                                       arena.random_.real(-55.0f, 55.0f)),
                20);
        }
        arena.state_.markBossDefeated();
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
        arena.entities_.spawnPickup(arena, enemy.pos,
                                    enemy.kind == QStringLiteral("tank") ? 22 : 16);
    }
}

} // namespace arena
