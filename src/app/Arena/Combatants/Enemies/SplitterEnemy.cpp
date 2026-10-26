#include "SplitterEnemy.h"

#include "MiniEnemy.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Base/Base.h"
#include "Random.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace arena {

namespace {
const EnemyRegistrar<SplitterEnemy> kRegistration(
    "splitter", Base::Violet);
}

SplitterEnemy::SplitterEnemy(const Math::Vec2 &position, int wave)
    : SelfDestructEnemy(typeid(SplitterEnemy), Role::Melee, position, wave)
{
}

void SplitterEnemy::onKilled(const Context &ctx)
{
    for (int side = -1; side <= 1; side += 2) {
        const Math::Vec2 offset(float(side) * 18.0f,
                                RandomService.real(-12.0f, 12.0f));
        auto child = Enemy::create(EnemyType(typeid(MiniEnemy)), pos + offset, wave);
        if (!child) {
            continue;
        }
        ctx.entities.spawnBurst(ctx, child->pos, child->color, 6, 90.0f, 0.3f);
        ctx.combatants.enemies().push_back(std::move(child));
    }
}

void SplitterEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    (void)facing;
    const Math::Vec2 forward(std::cos(age), std::sin(age));
    const Math::Vec2 side(-forward.y, forward.x);
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    Base::fillCircle(world, pos, r, Base::shade(color, 0.18f));
    Base::strokeCircle(world, pos, r, color, 2.0f);
    Base::strokeLine(world, pos - side * r, pos + side * r,
                     Base::shade(color, 0.75f), 1.0f);
    const Math::Vec2 offset = side * (3.0f + std::sin(age * 4.0f) * 2.0f);
    Base::fillCircle(world, pos + offset, 3.0f, color);
    Base::fillCircle(world, pos - offset, 3.0f, color);
    Base::strokeCircle(world, pos, r + 5.0f, Base::withAlpha(color, 70), 1.0f);
}

} // namespace arena
