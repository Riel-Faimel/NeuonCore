#include "DroneEnemy.h"

#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Base/Base.h"

#include <QPolygonF>

#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<DroneEnemy> kRegistration(
    Enemy::Kind::Drone, "drone", Enemy::Role::Melee, Base::Coral);
}

DroneEnemy::DroneEnemy(const Math::Vec2 &position, int wave)
    : OffensiveEnemy(Kind::Drone, "drone", Role::Melee, position, wave)
{
}

float DroneEnemy::weave() const
{
    return std::sin(age * 3.1f + phase) * 0.32f;
}

void DroneEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    const Math::Vec2 forward(std::cos(facing), std::sin(facing));
    const Math::Vec2 side(-forward.y, forward.x);
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    const Math::Vec2 tip = pos + forward * (r * 1.2f);
    const Math::Vec2 left = pos - forward * (r * 0.78f) + side * (r * 0.72f);
    const Math::Vec2 notch = pos - forward * (r * 0.32f);
    const Math::Vec2 right = pos - forward * (r * 0.78f) - side * (r * 0.72f);
    QPolygonF body;
    body << QPointF(tip.x, tip.y) << QPointF(left.x, left.y)
         << QPointF(notch.x, notch.y) << QPointF(right.x, right.y);
    Base::polygon(world, body, Base::shade(color, 0.18f), color, 2.0f);
    Base::fillCircle(world, pos, 3.0f, color);
    Base::strokeLine(world, left, right, Base::shade(color, 0.55f), 1.0f);
}

} // namespace arena
