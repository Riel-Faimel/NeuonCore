#include "RusherEnemy.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Base/Base.h"
#include "Random.h"

#include <QPolygonF>

#include <algorithm>
#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<RusherEnemy> kRegistration(
    Enemy::Kind::Rusher, "rusher", Enemy::Role::Melee, QColor(255, 140, 80));
}

RusherEnemy::RusherEnemy(const Math::Vec2 &position, int wave)
    : OffensiveEnemy(Kind::Rusher, "rusher", Role::Melee, position, wave)
{
    const EnemyStats &stats = statsFor(kind);
    lungeInterval = std::max(0.4f, stats.lungeInterval);
    lungeTimer = RandomService.real(0.4f, 1.1f);
}

Math::Vec2 RusherEnemy::desiredVelocity(const Context &ctx, float dt)
{
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = std::max(1.0f, toPlayer.length());
    const Math::Vec2 direction = toPlayer / distance;
    lungeTimer -= dt;
    if (lungeTimer <= 0.0f) {
        lungeTimer = lungeInterval;
        vel = direction * speed * 2.4f;
        ctx.entities.spawnBurst(ctx, pos, color, 5, 90.0f, 0.25f);
    }
    return direction * speed;
}

void RusherEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    const Math::Vec2 forward(std::cos(facing), std::sin(facing));
    const Math::Vec2 side(-forward.y, forward.x);
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    const Math::Vec2 tip = pos + forward * (r * 1.7f);
    const Math::Vec2 left = pos - forward * (r * 0.8f) + side * (r * 0.85f);
    const Math::Vec2 right = pos - forward * (r * 0.8f) - side * (r * 0.85f);
    QPolygonF body;
    body << QPointF(tip.x, tip.y) << QPointF(left.x, left.y)
         << QPointF(right.x, right.y);
    Base::polygon(world, body, Base::shade(color, 0.2f), color, 1.5f);
    for (int i = 1; i <= 3; ++i) {
        const Math::Vec2 back = pos - forward * (r * (1.0f + i * 0.8f));
        Base::strokeLine(world, back - side * (r * 0.4f),
                         back + side * (r * 0.4f),
                         Base::withAlpha(color, 110 - i * 25), 1.0f);
    }
}

} // namespace arena
