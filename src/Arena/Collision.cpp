#include "Collision.h"

#include "Arena/Arena.h"
#include "Arena/Bullet.h"
#include "Arena/Enemy.h"
#include "Arena/Player.h"
#include "GameConfig.h"
#include "Random.h"
#include "base/Base.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace arena {

void Collision::resolvePlayerBullets(Arena &arena, float dt)
{
    for (Bullet &bullet : arena.playerBullets_) {
        bullet.update(dt);
        if (!bullet.alive) {
            continue;
        }
        const size_t count = arena.enemies_.size();
        for (size_t i = 0; i < count; ++i) {
            if (i >= arena.enemies_.size()) {
                break;
            }
            Enemy *enemy = arena.enemies_[i].get();
            Fighter &fighter = *enemy;
            if (!fighter.isAlive() || bullet.hits.count(fighter.id())) {
                continue;
            }
            if (!Math::segmentCircle(bullet.prev, bullet.pos, fighter.position(),
                                     fighter.collisionRadius() + bullet.radius)) {
                continue;
            }
            const bool critical = arena.random_.real(0.0f, 1.0f) < bullet.critChance;
            const float damage = bullet.damage * (critical ? 2.0f : 1.0f);
            const int enemyId = fighter.id();
            fighter.takeDamage(arena, Damage{damage, critical, DamageSource::Bullet});
            bullet.hits.insert(enemyId);
            if (arena.bossState_.defeated()) {
                bullet.alive = false;
                break;
            }
            if (bullet.pierce <= 0) {
                bullet.alive = false;
                break;
            }
            --bullet.pierce;
        }
        if (arena.bossState_.defeated() || arena.playerDefeated_) {
            break;
        }
    }
    std::erase_if(arena.playerBullets_, [](const Bullet &bullet) {
        return !bullet.alive;
    });
}

void Collision::resolveEnemyBullets(Arena &arena, float dt)
{
    for (Bullet &bullet : arena.enemyBullets_) {
        bullet.update(dt);
        if (!bullet.alive || !arena.player_.state.alive) {
            continue;
        }
        if (!Math::segmentCircle(bullet.prev, bullet.pos, arena.player_.state.pos,
                                 arena.player_.state.radius + bullet.radius)) {
            continue;
        }
        if (arena.player_.state.invuln > 0.0f) {
            arena.spawnBurst(bullet.pos, Base::Cyan, 5, 80.0f, 0.25f);
        } else {
            arena.player_.takeDamage(
                arena, Damage{bullet.damage, false, DamageSource::Bullet});
        }
        bullet.alive = false;
        if (arena.playerDefeated_) {
            break;
        }
    }
    std::erase_if(arena.enemyBullets_, [](const Bullet &bullet) {
        return !bullet.alive;
    });
}

void Collision::separateEnemies(Arena &arena)
{
    std::vector<Enemy *> alive;
    alive.reserve(arena.enemies_.size());
    for (const auto &enemy : arena.enemies_) {
        if (enemy->alive) {
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

void Collision::contactDamage(Arena &arena)
{
    if (!arena.player_.state.alive || arena.player_.state.invuln > 0.0f) {
        return;
    }
    for (const auto &enemy : arena.enemies_) {
        if (!enemy->alive) {
            continue;
        }
        const Fighter &fighter = *enemy;
        const Math::Vec2 delta = arena.player_.state.pos - fighter.position();
        const float minimum =
            arena.player_.state.radius + fighter.collisionRadius();
        if (delta.lengthSquared() <= minimum * minimum) {
            arena.player_.takeDamage(
                arena, Damage{fighter.contactDamage(), false, DamageSource::Contact});
            if (delta.lengthSquared() > 0.001f) {
                const Math::Vec2 direction = delta.normalized();
                arena.player_.state.pos += direction * 22.0f;
                enemy->pos -= direction * 12.0f;
                arena.player_.state.pos.x =
                    Math::clamp(arena.player_.state.pos.x,
                                GameConfig::FieldLeft + 2.0f,
                                GameConfig::FieldRight - 2.0f);
                arena.player_.state.pos.y =
                    Math::clamp(arena.player_.state.pos.y,
                                GameConfig::FieldTop + 2.0f,
                                GameConfig::FieldBottom - 2.0f);
            }
            break;
        }
    }
}

} // namespace arena
