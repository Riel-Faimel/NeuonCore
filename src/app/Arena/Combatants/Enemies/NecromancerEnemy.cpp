#include "NecromancerEnemy.h"

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
const EnemyRegistrar<NecromancerEnemy> kRegistration(
    "necromancer", QColor(190, 120, 255));
}

NecromancerEnemy::NecromancerEnemy(const Math::Vec2 &position, int wave)
    : SupportEnemy(typeid(NecromancerEnemy), Role::Support, position, wave)
{
    const EnemyStats &stats = statsFor(type());
    supportRadius = stats.reviveRadius;
    supportValue = stats.reviveChance;
}

void NecromancerEnemy::onAllyKilled(const Context &ctx, Enemy &dead)
{
    if (!alive || dead.boss || dead.revived) {
        return;
    }
    if (pos.distanceTo(dead.pos) > supportRadius) {
        return;
    }
    if (RandomService.real(0.0f, 1.0f) >= supportValue) {
        return;
    }
    auto revived = Enemy::create(dead.type(), dead.pos, dead.wave);
    if (!revived) {
        return;
    }
    revived->hp = revived->maxHp * 0.5f;
    revived->hpDisplay = revived->hp;
    revived->revived = true;
    revived->score = std::max(1, dead.score / 2);
    ctx.entities.spawnBurst(ctx, dead.pos, Base::Violet, 16, 150.0f, 0.5f);
    ctx.entities.floatingTexts().push_back(FloatingText{
        dead.pos + Math::Vec2(0.0f, -20.0f), QStringLiteral("复生"),
        Base::Violet, 0.9f, 0.9f, 15, false});
    ctx.combatants.enemies().push_back(std::move(revived));
}

void NecromancerEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    (void)facing;
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    Base::fillCircle(world, pos, r, Base::shade(color, 0.14f));
    Base::strokeCircle(world, pos, r, color, 2.0f);
    for (int i = 0; i < 3; ++i) {
        const float angle = -age * 1.4f + Math::Tau * float(i) / 3.0f;
        const Math::Vec2 dot = pos + Math::Vec2(std::cos(angle),
                                                std::sin(angle)) * (r + 6.0f);
        Base::fillCircle(world, dot, 3.0f, Base::shade(color, 1.25f));
    }
    Base::fillCircle(world, pos, r * 0.28f, QColor(245, 235, 255));
    Base::strokeCircle(world, pos, r + 12.0f, Base::withAlpha(color, 55), 1.0f);
}

} // namespace arena
