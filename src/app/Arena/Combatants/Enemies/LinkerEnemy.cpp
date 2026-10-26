#include "LinkerEnemy.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Arena/Context.h"
#include "Base/Base.h"

#include <QPolygonF>

#include <algorithm>
#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<LinkerEnemy> kRegistration(
    "linker", QColor(150, 160, 255));
}

LinkerEnemy::LinkerEnemy(const Math::Vec2 &position, int wave)
    : DefensiveEnemy("linker", Role::Support, position, wave)
{
    const EnemyStats &stats = statsFor(kind);
    supportRadius = stats.shareRadius;
    supportValue = stats.shareRatio;
}

Math::Vec2 LinkerEnemy::desiredVelocity(const Context &ctx, float dt)
{
    const Math::Vec2 cluster = allyClusterDirection(ctx);
    if (cluster.lengthSquared() > 1.0f) {
        return cluster.normalized() * speed;
    }
    return DefensiveEnemy::desiredVelocity(ctx, dt);
}

void LinkerEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    const Math::Vec2 forward(std::cos(facing), std::sin(facing));
    const Math::Vec2 side(-forward.y, forward.x);
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    QPolygonF body;
    const float rotation = age * 0.4f;
    for (int i = 0; i < 4; ++i) {
        const float angle = rotation + Math::Tau * float(i) / 4.0f;
        body << QPointF(pos.x + std::cos(angle) * (r * 1.15f),
                        pos.y + std::sin(angle) * (r * 1.15f));
    }
    Base::polygon(world, body, Base::shade(color, 0.18f), color, 2.0f);
    Base::fillCircle(world, pos, r * 0.34f, Base::shade(color, 1.4f));
    Base::strokeLine(world, pos - side * (r + 5.0f), pos + side * (r + 5.0f),
                     Base::withAlpha(color, 150), 1.0f);
}

} // namespace arena
