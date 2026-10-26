#include "EnemyTypes.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"

#include <algorithm>

namespace arena {

namespace {

bool isSelfDestruct(const Enemy *enemy)
{
    return dynamic_cast<const SelfDestructEnemy *>(enemy) != nullptr;
}

// 离自己最近的防御型友军（tank / linker）。
const Enemy *nearestDefender(const Context &ctx, const Enemy &self)
{
    const Enemy *best = nullptr;
    float bestDistance = 1e9f;
    for (const auto &owned : ctx.combatants.enemies()) {
        Enemy *other = owned.get();
        if (other == &self || !other->alive) {
            continue;
        }
        if (dynamic_cast<DefensiveEnemy *>(other) == nullptr) {
            continue;
        }
        const float distance = self.pos.distanceTo(other->pos);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = other;
        }
    }
    return best;
}

// 防御者背后（远离玩家一侧）的落脚点。
Math::Vec2 behindDefender(const Enemy &defender, const Math::Vec2 &player)
{
    Math::Vec2 away = defender.pos - player;
    if (away.lengthSquared() < 1.0f) {
        away = Math::Vec2(1.0f, 0.0f);
    }
    away = away.normalized();
    return defender.pos + away * (defender.radius + 42.0f);
}

// 在“可跟随”的友军里找邻居最多的点。
// 自毁型永远不计入；有防御线时只考虑防御线后方的友军。
const Enemy *densestAlly(const Context &ctx, const Enemy &self,
                         const Enemy *defender, const Math::Vec2 &forward)
{
    const auto &allies = ctx.combatants.enemies();
    const Enemy *best = nullptr;
    int bestNeighbors = -1;
    float bestDistance = 1e9f;
    for (const auto &owned : allies) {
        const Enemy *candidate = owned.get();
        if (candidate == &self || !candidate->alive || isSelfDestruct(candidate)) {
            continue;
        }
        if (defender != nullptr
            && (candidate->pos - defender->pos).dot(forward) < -10.0f) {
            continue;
        }
        int neighbors = 0;
        for (const auto &other : allies) {
            const Enemy *neighbor = other.get();
            if (neighbor == candidate || !neighbor->alive
                || isSelfDestruct(neighbor)) {
                continue;
            }
            if (defender != nullptr
                && (neighbor->pos - defender->pos).dot(forward) < -10.0f) {
                continue;
            }
            if (candidate->pos.distanceTo(neighbor->pos) <= 150.0f) {
                ++neighbors;
            }
        }
        const float distance = self.pos.distanceTo(candidate->pos);
        if (neighbors > bestNeighbors
            || (neighbors == bestNeighbors && distance < bestDistance)) {
            bestNeighbors = neighbors;
            bestDistance = distance;
            best = candidate;
        }
    }
    return best;
}

} // namespace

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
    const Math::Vec2 player = ctx.combatants.player().state.pos;
    const Math::Vec2 toPlayer = player - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    const Math::Vec2 direction = toPlayer / distance;

    // 有防御线就躲到最近防御者背后；没有才退回“保持射程”走位。
    const Enemy *defender = nearestDefender(ctx, *this);
    if (defender != nullptr && defender->pos.distanceTo(player) < distance) {
        const Math::Vec2 anchor = behindDefender(*defender, player);
        const Math::Vec2 delta = anchor - pos;
        if (delta.lengthSquared() > 64.0f) {
            return delta.normalized() * speed;
        }
        const Math::Vec2 perpendicular(-direction.y, direction.x);
        return perpendicular * speed * strafeFactor() * 0.5f * float(strafe);
    }

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
    const Math::Vec2 player = ctx.combatants.player().state.pos;

    // 站在“远程/辅助后排”和玩家之间。
    Math::Vec2 centroid;
    int count = 0;
    for (const auto &owned : ctx.combatants.enemies()) {
        const Enemy *ally = owned.get();
        if (ally == this || !ally->alive) {
            continue;
        }
        if (dynamic_cast<const RangedEnemy *>(ally) != nullptr
            || dynamic_cast<const SupportEnemy *>(ally) != nullptr) {
            centroid += ally->pos;
            ++count;
        }
    }
    if (count == 0) {
        const Math::Vec2 toPlayer = player - pos;
        return toPlayer.normalized() * speed;
    }
    centroid /= float(count);
    const Math::Vec2 anchor = centroid + (player - centroid) * 0.45f;
    const Math::Vec2 delta = anchor - pos;
    if (delta.lengthSquared() > 25.0f) {
        return delta.normalized() * speed;
    }
    const Math::Vec2 direction = (player - pos).normalized();
    return Math::Vec2(-direction.y, direction.x) * speed * 0.25f * float(strafe);
}

Math::Vec2 SupportEnemy::desiredVelocity(const Context &ctx, float dt)
{
    (void)dt;
    const Math::Vec2 player = ctx.combatants.player().state.pos;
    const Math::Vec2 toPlayer = player - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    const Math::Vec2 direction = toPlayer / distance;

    // 优先躲到防御者背后；同时朝“非自毁型”友军最密集的位置靠。
    const Enemy *defender = nearestDefender(ctx, *this);
    const Math::Vec2 forward = defender != nullptr
                                   ? (defender->pos - player).normalized()
                                   : Math::Vec2(1.0f, 0.0f);
    const Enemy *cluster = densestAlly(ctx, *this, defender, forward);
    Math::Vec2 target;
    bool hasTarget = false;
    if (cluster != nullptr) {
        target = cluster->pos + forward * 18.0f;
        hasTarget = true;
    } else if (defender != nullptr) {
        target = behindDefender(*defender, player);
        hasTarget = true;
    }

    Math::Vec2 desired;
    if (hasTarget && (target - pos).lengthSquared() > 25.0f) {
        desired = (target - pos).normalized() * speed;
    } else if (distance > 300.0f) {
        desired = direction * speed;
    } else if (distance < 210.0f) {
        desired = -direction * speed;
    } else {
        desired = Math::Vec2(-direction.y, direction.x) * speed * 0.55f
                  * float(strafe);
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
