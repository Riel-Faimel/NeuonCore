#include "MenderEnemy.h"

#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Base/Base.h"

#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<MenderEnemy> kRegistration(
    "mender", QColor(120, 255, 178));
}

MenderEnemy::MenderEnemy(const Math::Vec2 &position, int wave)
    : SupportEnemy("mender", Role::Support, position, wave)
{
    const EnemyStats &stats = statsFor(kind);
    supportRadius = stats.healRadius;
    supportValue = stats.healRate;
}

void MenderEnemy::updateSpecial(const Context &ctx, float dt)
{
    healAllies(ctx, dt);
}

void MenderEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    (void)facing;
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    Base::fillCircle(world, pos, r, Base::shade(color, 0.16f));
    Base::strokeCircle(world, pos, r, color, 2.0f);
    const float arm = r * 0.62f;
    Base::strokeLine(world, {pos.x - arm, pos.y}, {pos.x + arm, pos.y},
                     Base::shade(color, 1.1f), 4.0f);
    Base::strokeLine(world, {pos.x, pos.y - arm}, {pos.x, pos.y + arm},
                     Base::shade(color, 1.1f), 4.0f);
    Base::strokeCircle(world, pos, r + 5.0f, Base::withAlpha(color, 70), 1.0f);
}

} // namespace arena
