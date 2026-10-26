#include "SpitterEnemy.h"

#include "Arena/Bullets/Bullets.h"
#include "Arena/Bullets/StandardBullet.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Base/Base.h"
#include "Data/GameData.h"
#include "Random.h"

#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <memory>

namespace arena {

namespace {
const EnemyRegistrar<SpitterEnemy> kRegistration(
    "spitter", Base::Magenta);
}

SpitterEnemy::SpitterEnemy(const Math::Vec2 &position, int wave)
    : RangedEnemy("spitter", Role::Ranged, position, wave)
{
}

void SpitterEnemy::updateSpecial(const Context &ctx, float dt)
{
    (void)dt;
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    if (age > 0.9f && attackTimer <= 0.0f && distance < 540.0f) {
        attackTimer = RandomService.real(1.8f, 2.5f);
        shoot(ctx);
    }
}

void SpitterEnemy::shoot(const Context &ctx)
{
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float base = std::atan2(toPlayer.y, toPlayer.x);
    const Math::Vec2 aim = Math::Vec2(std::cos(base), std::sin(base))
        .rotated(RandomService.real(-0.055f, 0.055f));
    ctx.bullets.submit(std::make_unique<StandardBullet>(
        pos + aim * 18.0f, aim * data::kSpitterBullet.speed,
        data::kSpitterBullet.damage * damageMultiplier(), Base::Magenta,
        Bullet::Standpoint::Enemy, data::kSpitterBullet.radius));
    ctx.entities.spawnBurst(ctx, pos, Base::Magenta, 5, 70.0f, 0.26f);
    BaseService.playSound(QStringLiteral("shoot_enemy"));
}

void SpitterEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    const Math::Vec2 forward(std::cos(facing), std::sin(facing));
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    QPolygonF body;
    const float rotation = age * 0.8f;
    for (int i = 0; i < 6; ++i) {
        const float angle = rotation + Math::Tau * float(i) / 6.0f;
        body << QPointF(pos.x + std::cos(angle) * r, pos.y + std::sin(angle) * r);
    }
    Base::polygon(world, body, Base::shade(color, 0.16f), color, 2.0f);
    QPolygonF inner;
    for (int i = 0; i < 3; ++i) {
        const float angle = -rotation * 1.3f + Math::Tau * float(i) / 3.0f;
        inner << QPointF(pos.x + std::cos(angle) * (r * 0.48f),
                         pos.y + std::sin(angle) * (r * 0.48f));
    }
    Base::polygon(world, inner, QColor(0, 0, 0, 0), color, 1.0f);
    Base::fillCircle(world, pos, 3.0f, Base::White);
    Base::strokeCircle(world, pos, 7.0f, color, 1.0f);
    Base::strokeLine(world, pos + forward * 8.0f, pos + forward * (r + 6.0f),
                     Base::shade(color, 0.8f), 2.0f);
}

} // namespace arena
