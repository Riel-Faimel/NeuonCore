#include "TankEnemy.h"

#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Base/Base.h"

#include <QPolygonF>

#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<TankEnemy> kRegistration(
    "tank", Base::Amber);
}

TankEnemy::TankEnemy(const Math::Vec2 &position, int wave)
    : DefensiveEnemy("tank", Role::Melee, position, wave)
{
}

void TankEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    (void)facing;
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    QPolygonF body;
    const float rotation = age * 0.25f;
    for (int i = 0; i < 8; ++i) {
        const float angle = rotation + Math::Tau * float(i) / 8.0f;
        body << QPointF(pos.x + std::cos(angle) * r, pos.y + std::sin(angle) * r);
    }
    Base::polygon(world, body, Base::shade(color, 0.16f), color, 2.0f);
    QPolygonF inner;
    for (int i = 0; i < 4; ++i) {
        const float angle = -rotation + Math::Tau * float(i) / 4.0f + Math::Pi / 4.0f;
        inner << QPointF(pos.x + std::cos(angle) * (r * 0.56f),
                         pos.y + std::sin(angle) * (r * 0.56f));
    }
    Base::polygon(world, inner, Base::shade(color, 0.34f), color, 1.0f);
    Base::fillCircle(world, pos, 4.0f, QColor(255, 220, 160));
}

} // namespace arena
