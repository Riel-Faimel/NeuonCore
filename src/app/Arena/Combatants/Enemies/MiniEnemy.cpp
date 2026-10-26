#include "MiniEnemy.h"

#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Base/Base.h"

#include <QPolygonF>

#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<MiniEnemy> kRegistration(
    Enemy::Kind::Mini, "mini", Enemy::Role::Melee, QColor(218, 154, 255));
}

MiniEnemy::MiniEnemy(const Math::Vec2 &position, int wave)
    : OffensiveEnemy(Kind::Mini, "mini", Role::Melee, position, wave)
{
}

float MiniEnemy::weave() const
{
    return std::sin(age * 3.1f + phase) * 0.32f;
}

void MiniEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    const Math::Vec2 forward(std::cos(facing), std::sin(facing));
    const Math::Vec2 side(-forward.y, forward.x);
    Base::glow(glow, pos, radius * 1.25f, color, 0.9f);

    QPolygonF body;
    body << QPointF(pos.x + forward.x * 9.0f, pos.y + forward.y * 9.0f)
         << QPointF(pos.x + side.x * 6.0f, pos.y + side.y * 6.0f)
         << QPointF(pos.x - forward.x * 7.0f, pos.y - forward.y * 7.0f)
         << QPointF(pos.x - side.x * 6.0f, pos.y - side.y * 6.0f);
    Base::polygon(world, body, Base::shade(color, 0.18f), color, 1.0f);
    Base::fillCircle(world, pos, 2.0f, Base::White);
}

} // namespace arena
