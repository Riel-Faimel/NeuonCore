#include "BossEnemy.h"

#include "Arena/Bullets/Bullets.h"
#include "Arena/Bullets/StandardBullet.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Base/Base.h"
#include "Data/GameData.h"

#include <QPolygonF>

#include <array>
#include <cmath>
#include <memory>

namespace arena {

namespace {
const EnemyRegistrar<BossEnemy> kRegistration(
    "boss", QColor(255, 74, 160));
}

BossEnemy::BossEnemy(const Math::Vec2 &position, int wave)
    : Enemy("boss", Role::Ranged, Archetype::Boss, position, wave)
{
    // Boss 不吃波次成长：数值直接用 JSON 基础值。
    boss = true;
    const EnemyStats &stats = statsFor(kind);
    maxHp = stats.hp;
    speed = stats.speed;
    damage = stats.damage;
    hp = maxHp;
    hpDisplay = hp;
}

Math::Vec2 BossEnemy::desiredVelocity(const Context &ctx, float dt)
{
    (void)dt;
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    const Math::Vec2 direction = toPlayer / distance;
    const Math::Vec2 perpendicular(-direction.y, direction.x);
    if (distance > 265.0f) {
        return direction * speed;
    }
    if (distance < 185.0f) {
        return -direction * speed * 0.9f;
    }
    return perpendicular * speed * 0.55f * float(strafe);
}

void BossEnemy::updateSpecial(const Context &ctx, float dt)
{
    (void)dt;
    if (age <= 1.45f) {
        return;
    }
    const bool enraged = hp < maxHp * 0.48f;
    if (attackTimer <= 0.0f) {
        attackTimer = enraged ? 0.95f : 1.25f;
        shoot(ctx);
    }
    if (radialTimer <= 0.0f) {
        radialTimer = enraged ? 2.15f : 2.65f;
        const int count = enraged ? 18 : 14;
        const float offset = age * 0.55f;
        for (int i = 0; i < count; ++i) {
            const float angle = offset + Math::Tau * float(i) / float(count);
            const Math::Vec2 bulletDirection(std::cos(angle), std::sin(angle));
            ctx.bullets.spawnStandard(
                pos + bulletDirection * 42.0f,
                bulletDirection * data::kBossRadialBullet.speed,
                data::kBossRadialBullet.damage * damageMultiplier(), Base::Magenta,
                Bullet::Standpoint::Enemy, data::kBossRadialBullet.radius);
        }
        ctx.entities.addShockwave(pos, 92.0f, Base::Magenta, 0.36f, 2);
    }
}

void BossEnemy::shoot(const Context &ctx)
{
    const bool enraged = hp < maxHp * 0.48f;
    fireAimed(ctx, enraged ? 3 : 2, 0.11f);
    BaseService.playSound(QStringLiteral("shoot_enemy"));
}

void BossEnemy::fireAimed(const Context &ctx, int count, float spread)
{
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float base = std::atan2(toPlayer.y, toPlayer.x);
    for (int i = 0; i < count; ++i) {
        const float offset = (float(i) - (count - 1) / 2.0f) * spread;
        const Math::Vec2 direction(std::cos(base + offset), std::sin(base + offset));
        ctx.bullets.spawnStandard(
            pos + direction * 46.0f, direction * data::kBossAimedBullet.speed,
            data::kBossAimedBullet.damage * damageMultiplier(),
            QColor(255, 112, 190), Bullet::Standpoint::Enemy,
            data::kBossAimedBullet.radius);
    }
    ctx.entities.spawnBurst(ctx, pos, Base::Magenta, 8, 100.0f, 0.35f);
}

void BossEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    drawBoss(world, glow, facing);
}

void BossEnemy::drawBoss(QPainter &world, QPainter &glow, float facing) const
{
    const float r = radius;
    Base::glow(glow, pos, 100.0f, Base::Magenta, 1.3f);

    const std::array<std::array<float, 5>, 3> rings = {{
        {r + 14.0f, 0.55f, age * 0.9f, 1.8f, 2.0f},
        {r + 28.0f, -0.35f, -age * 0.7f, 1.2f, 1.0f},
        {r + 42.0f, 0.22f, age * 0.4f + 2.4f, 0.8f, 1.0f},
    }};
    for (const auto &ring : rings) {
        Base::arcRadians(world, pos, ring[0], ring[2], ring[3],
                         Base::withAlpha(Base::Magenta, 95), ring[4]);
        Base::arcRadians(glow, pos, ring[0], ring[2], ring[3],
                         Base::shade(Base::Magenta, 0.25f), ring[4] + 1.0f);
    }

    const float rotation = -age * 0.5f;
    QPolygonF body;
    for (int i = 0; i < 6; ++i) {
        const float angle = rotation + Math::Tau * float(i) / 6.0f;
        body << QPointF(pos.x + std::cos(angle) * r, pos.y + std::sin(angle) * r);
    }
    Base::polygon(world, body, QColor(43, 12, 40), QColor(255, 122, 186), 3.0f);

    QPolygonF inner;
    for (int i = 0; i < 6; ++i) {
        const float angle = rotation + Math::Pi / 6.0f + Math::Tau * float(i) / 6.0f;
        inner << QPointF(pos.x + std::cos(angle) * (r * 0.66f),
                         pos.y + std::sin(angle) * (r * 0.66f));
    }
    Base::polygon(world, inner, Base::shade(Base::Magenta, 0.22f), Base::Magenta, 1.0f);

    for (int i = 0; i < 6; ++i) {
        const float angle = rotation + Math::Tau * float(i) / 6.0f;
        const Math::Vec2 pod = pos
            + Math::Vec2(std::cos(angle), std::sin(angle)) * (r + 8.0f);
        Base::fillCircle(world, pod, 5.0f, QColor(255, 148, 200));
        Base::strokeCircle(world, pod, 8.0f, Base::Magenta, 1.0f);
    }

    const Math::Vec2 eyeCenter = pos
        + Math::Vec2(std::cos(facing), std::sin(facing)) * 6.0f;
    Base::fillCircle(world, eyeCenter, 13.0f, QColor(255, 228, 241));
    const Math::Vec2 pupil = eyeCenter
        + Math::Vec2(std::cos(facing), std::sin(facing)) * 4.0f;
    Base::fillCircle(world, pupil, 6.0f, QColor(91, 7, 43));
    Base::fillCircle(world, pupil + Math::Vec2(-2.0f, -2.0f), 2.0f,
                     QColor(255, 245, 250));
    Base::strokeCircle(world, pos, r + 54.0f, Base::withAlpha(Base::Magenta, 70), 2.0f);
}

} // namespace arena
