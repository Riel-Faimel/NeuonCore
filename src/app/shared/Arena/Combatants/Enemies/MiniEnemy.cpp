#include "MiniEnemy.h"

#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Base/Base.h"

#include <QPolygonF>

#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<MiniEnemy> kRegistration(
    "mini", QColor(218, 154, 255));
}

MiniEnemy::MiniEnemy(const Math::Vec2 &position, int wave)
    : OffensiveEnemy(typeid(MiniEnemy), Role::Melee, position, wave)
{
    // 召唤物可能一次出现几十只：倾向更早简化外观（人少时仍画完整机身），
    // 具体门槛见 Enemy.cpp。
    prefersLowDetail = true;
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
