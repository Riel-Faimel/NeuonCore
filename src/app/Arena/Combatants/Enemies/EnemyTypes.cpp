#include "EnemyTypes.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"

#include <algorithm>

namespace arena {

Math::Vec2 OffensiveEnemy::desiredVelocity(const Context &ctx, float dt)
{
    (void)dt;
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    const Math::Vec2 direction = toPlayer / distance;
    return direction.rotated(weave()) * speed;
}

Math::Vec2 RangedEnemy::desiredVelocity(const Context &ctx, float dt)
{
    (void)dt;
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    const Math::Vec2 direction = toPlayer / distance;
    const Math::Vec2 perpendicular(-direction.y, direction.x);
    if (distance > maxRange()) {
        return direction * speed;
    }
    if (distance < minRange()) {
        return -direction * speed;
    }
    return perpendicular * speed * strafeFactor() * float(strafe);
}

Math::Vec2 DefensiveEnemy::desiredVelocity(const Context &ctx, float dt)
{
    (void)dt;
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    return toPlayer / distance * speed;
}

Math::Vec2 SupportEnemy::desiredVelocity(const Context &ctx, float dt)
{
    (void)dt;
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    const Math::Vec2 direction = toPlayer / distance;
    const Math::Vec2 perpendicular(-direction.y, direction.x);
    Math::Vec2 desired;
    const Math::Vec2 cluster = allyClusterDirection(ctx);
    if (cluster.lengthSquared() > 1.0f) {
        desired = cluster.normalized() * speed;
    } else if (distance > 300.0f) {
        desired = direction * speed;
    } else if (distance < 210.0f) {
        desired = -direction * speed;
    } else {
        desired = perpendicular * speed * 0.55f * float(strafe);
    }
    if (distance < 160.0f) {
        desired -= direction * speed * 0.8f;
    }
    return desired;
}

void SupportEnemy::applyDamageBuff(const Context &ctx)
{
    for (const auto &other : ctx.combatants.enemies()) {
        Enemy *ally = other.get();
        if (ally == this || !ally->alive) {
            continue;
        }
        if (pos.distanceTo(ally->pos) > supportRadius) {
            continue;
        }
        ally->damageBuffBonus = std::max(ally->damageBuffBonus, supportValue);
        ally->damageBuffTimer = 0.35f;
    }
}

void SupportEnemy::healAllies(const Context &ctx, float dt)
{
    bool healed = false;
    for (const auto &other : ctx.combatants.enemies()) {
        Enemy *ally = other.get();
        if (ally == this || !ally->alive || ally->hp >= ally->maxHp) {
            continue;
        }
        if (pos.distanceTo(ally->pos) > supportRadius + ally->radius) {
            continue;
        }
        ally->hp = std::min(ally->maxHp, ally->hp + supportValue * dt);
        healed = true;
    }
    healFxTimer -= dt;
    if (healed && healFxTimer <= 0.0f) {
        healFxTimer = 0.45f;
        ctx.entities.spawnBurst(ctx, pos, color, 4, 60.0f, 0.3f);
    }
}

Math::Vec2 SelfDestructEnemy::desiredVelocity(const Context &ctx, float dt)
{
    (void)dt;
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    return toPlayer / distance * speed;
}

} // namespace arena
