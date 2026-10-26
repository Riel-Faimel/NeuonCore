#include "AmplifierEnemy.h"

#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Base/Base.h"

#include <QPolygonF>

#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<AmplifierEnemy> kRegistration(
    "amplifier", QColor(120, 220, 255));
}

AmplifierEnemy::AmplifierEnemy(const Math::Vec2 &position, int wave)
    : SupportEnemy(typeid(AmplifierEnemy), Role::Support, position, wave)
{
    const EnemyStats &stats = statsFor(type());
    supportRadius = stats.auraRadius;
    supportValue = stats.auraValue;
}

void AmplifierEnemy::updateSpecial(const Context &ctx, float dt)
{
    (void)dt;
    applyDamageBuff(ctx);
}

void AmplifierEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    const Math::Vec2 forward(std::cos(facing), std::sin(facing));
    const Math::Vec2 side(-forward.y, forward.x);
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    QPolygonF body;
    for (int i = 0; i < 6; ++i) {
        const float angle = age * 0.5f + Math::Tau * float(i) / 6.0f;
        body << QPointF(pos.x + std::cos(angle) * r, pos.y + std::sin(angle) * r);
    }
    Base::polygon(world, body, Base::shade(color, 0.16f), color, 2.0f);
    Base::strokeLine(world, pos - side * (r * 0.5f), pos + side * (r * 0.5f),
                     Base::shade(color, 0.9f), 2.0f);
    Base::strokeLine(world, pos - forward * (r * 0.5f),
                     pos + forward * (r * 0.5f), Base::shade(color, 0.9f), 2.0f);
    Base::strokeCircle(world, pos, r + 5.0f, Base::withAlpha(color, 80), 1.0f);
}

} // namespace arena
