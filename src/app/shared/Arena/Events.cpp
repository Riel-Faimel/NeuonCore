#include "Events.h"

#include "Arena/Context.h"
#include "Arena/Host.h"
#include "Arena/Bullets/Bullets.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Combatants/Enemy.h"
#include "Arena/Combatants/Enemies/DroneEnemy.h"
#include "Arena/Combatants/Enemies/MiniEnemy.h"
#include "Arena/Combatants/Enemies/SpitterEnemy.h"
#include "Arena/Combatants/Enemies/SplitterEnemy.h"
#include "Arena/Combatants/Enemies/TankEnemy.h"
#include "Arena/Combatants/Player.h"
#include "Arena/Entities/FloatingText.h"
#include "Arena/Entities/Mine.h"
#include "Arena/State.h"
#include "Bullet.h"
#include "Data/GameData.h"
#include "GameConfig.h"
#include "Random.h"
#include "Shop/Shop.h"
#include "Base/Base.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace arena {

void Events::resolvePlayerBullets(const Context &ctx)
{
    const auto &enemies = ctx.combatants.enemies();
    for (auto &owned : ctx.bullets.playerBullets()) {
        Bullet &bullet = *owned;
        if (!bullet.alive()) {
            continue;
        }
        const size_t count = enemies.size();
        for (size_t i = 0; i < count; ++i) {
            if (i >= enemies.size()) {
                break;
            }
            Enemy *enemy = enemies[i].get();
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
            if (!bullet.hit(ctx, fighter)) {
                break;
            }
        }
        if (ctx.state.playerDefeated()) {
            break;
        }
    }
    ctx.bullets.cleanup();
}

void Events::resolveEnemyBullets(const Context &ctx)
{
    if (ctx.combatants.livingPlayerCount() == 0) {
        ctx.bullets.cleanup();
        return;
    }
    for (auto &owned : ctx.bullets.enemyBullets()) {
        Bullet &bullet = *owned;
        if (!bullet.alive()) {
            continue;
        }
        // 敌方子弹可以命中任何一架活着的玩家飞机（按 id 结算到被击中的那一架）。
        for (Player &player : ctx.combatants.players()) {
            if (!player.state.active || !player.isAlive()) {
                continue;
            }
            if (!Math::segmentCircle(bullet.prev(), bullet.position(),
                                     player.position(),
                                     player.state.radius + bullet.radius())) {
                continue;
            }
            if (player.state.invuln > 0.0f) {
                ctx.entities.spawnBurst(ctx, bullet.position(), Base::Cyan, 5,
                                        80.0f, 0.25f);
            }
            bullet.hit(ctx, player);
            break;
        }
        if (ctx.combatants.livingPlayerCount() == 0) {
            break;
        }
    }
    ctx.bullets.cleanup();
}

void Events::resolveMines(const Context &ctx)
{
    for (Mine &mine : ctx.entities.mines()) {
        if (!mine.alive || mine.age < mine.armTime) {
            continue;
        }
        for (const auto &enemy : ctx.combatants.enemies()) {
            if (!enemy->alive) {
                continue;
            }
            if (mine.pos.distanceTo(enemy->pos) > mine.triggerRadius + enemy->radius) {
                continue;
            }
            detonateMine(ctx, mine);
            break;
        }
    }
    ctx.entities.cleanup();
}

void Events::detonateMine(const Context &ctx, Mine &mine)
{
    mine.alive = false;
    for (const auto &enemy : ctx.combatants.enemies()) {
        if (!enemy->alive) {
            continue;
        }
        if (mine.pos.distanceTo(enemy->pos) > mine.blastRadius + enemy->radius) {
            continue;
        }
        enemy->takeDamage(ctx, Damage{data::kMine.damage, false, DamageSource::Shock});
    }
    ctx.entities.spawnBurst(ctx, mine.pos, Base::Amber, 30, 230.0f, 0.7f);
    ctx.entities.addShockwave(mine.pos, mine.blastRadius, Base::Amber, 0.5f, 3);
    ctx.host.addFeedback(Base::Amber, 42, 8.0f);
    BaseService.playSound(QStringLiteral("explode"));
}

void Events::separateEnemies(const Context &ctx)
{
    auto &enemies = ctx.combatants.enemies();
    for (size_t i = 0; i < enemies.size(); ++i) {
        Enemy *first = enemies[i].get();
        if (!first->isAlive()) {
            continue;
        }
        for (size_t j = i + 1; j < enemies.size(); ++j) {
            Enemy *second = enemies[j].get();
            if (!second->isAlive()) {
                continue;
            }
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

void Events::contactDamage(const Context &ctx)
{
    for (Player &player : ctx.combatants.players()) {
        if (!player.state.active || !player.isAlive()
            || player.state.invuln > 0.0f) {
            continue;
        }
        for (const auto &enemy : ctx.combatants.enemies()) {
            if (!enemy->isAlive()) {
                continue;
            }
            const Fighter &fighter = *enemy;
            const Math::Vec2 delta = player.position() - fighter.position();
            const float minimum = player.state.radius + fighter.collisionRadius();
            if (delta.lengthSquared() <= minimum * minimum) {
                player.takeDamage(ctx, Damage{fighter.contactDamage(), false,
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
}

void Events::handleKill(const Context &ctx, Enemy &enemy)
{
    if (!enemy.alive) {
        return;
    }
    enemy.alive = false;
    ShopService.score().addKill(enemy.score);
    ctx.entities.spawnBurst(ctx, enemy.pos, enemy.color,
                            enemy.boss ? 32 : 14,
                            enemy.boss ? 240.0f : 150.0f, 0.65f);
    ctx.entities.addShockwave(enemy.pos, enemy.boss ? 170.0f : 70.0f,
                              enemy.color, 0.45f, 2);
    BaseService.playSound(QStringLiteral("kill"));

    enemy.onKilled(ctx);

    if (!enemy.boss) {
        // 通知所有存活友军：有人阵亡；复活/召唤进度由具体敌人类实现。
        std::vector<Enemy *> listeners;
        for (const auto &other : ctx.combatants.enemies()) {
            if (other->alive && other.get() != &enemy) {
                listeners.push_back(other.get());
            }
        }
        for (Enemy *listener : listeners) {
            listener->onAllyKilled(ctx, enemy);
        }
    }

    if (enemy.boss) {
        BaseService.playSound(QStringLiteral("boss"));
        for (int i = 0; i < 4; ++i) {
            ctx.entities.spawnPickup(
                ctx,
                enemy.pos + Math::Vec2(RandomService.real(-55.0f, 55.0f),
                                       RandomService.real(-55.0f, 55.0f)),
                20);
        }
        ctx.state.markBossDefeated();
        return;
    }

    float dropChance = 0.06f;
    if (dynamic_cast<const DroneEnemy *>(&enemy) != nullptr) {
        dropChance = 0.055f;
    } else if (dynamic_cast<const MiniEnemy *>(&enemy) != nullptr) {
        dropChance = 0.03f;
    } else if (dynamic_cast<const SpitterEnemy *>(&enemy) != nullptr) {
        dropChance = 0.10f;
    } else if (dynamic_cast<const TankEnemy *>(&enemy) != nullptr) {
        dropChance = 0.18f;
    } else if (dynamic_cast<const SplitterEnemy *>(&enemy) != nullptr) {
        dropChance = 0.14f;
    }
    if (RandomService.real(0.0f, 1.0f) < dropChance) {
        ctx.entities.spawnPickup(
            ctx, enemy.pos,
            dynamic_cast<const TankEnemy *>(&enemy) != nullptr ? 22 : 16);
    }
}

} // namespace arena
