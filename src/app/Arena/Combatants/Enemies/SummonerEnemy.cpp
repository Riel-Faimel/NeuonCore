#include "SummonerEnemy.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Base/Base.h"

#include <QPolygonF>

#include <algorithm>
#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<SummonerEnemy> kRegistration(
    Enemy::Kind::Summoner, "summoner", Enemy::Role::Ranged, QColor(255, 200, 90));
}

SummonerEnemy::SummonerEnemy(const Math::Vec2 &position, int wave)
    : RangedEnemy(Kind::Summoner, "summoner", Role::Ranged, position, wave)
{
    const EnemyStats &stats = statsFor(kind);
    chargeRequired = std::max(1, stats.chargeRequired);
    summonCount = std::max(1, stats.summonCount);
}

void SummonerEnemy::onAllyKilled(const Context &ctx, Enemy &dead)
{
    // 召唤物不计入进度，否则会“杀 mini → 再召唤”无限循环。
    if (!alive || dead.kind == QStringLiteral("mini")) {
        return;
    }
    ++charge;
    if (charge < chargeRequired) {
        return;
    }
    charge -= chargeRequired;
    ctx.combatants.spawnMinions(ctx, pos, summonCount, wave);
    ctx.entities.spawnBurst(ctx, pos, color, 18, 160.0f, 0.5f);
    ctx.entities.floatingTexts().push_back(FloatingText{
        pos + Math::Vec2(0.0f, -28.0f), QStringLiteral("召唤"),
        color, 0.9f, 0.9f, 15, false});
}

void SummonerEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    (void)facing;
    const float r = radius;
    Base::glow(glow, pos, r * 1.25f, color, 0.9f);

    QPolygonF body;
    for (int i = 0; i < 6; ++i) {
        const float angle = Math::Tau * float(i) / 6.0f + Math::Pi / 6.0f;
        body << QPointF(pos.x + std::cos(angle) * r, pos.y + std::sin(angle) * r);
    }
    Base::polygon(world, body, Base::shade(color, 0.16f), color, 2.0f);
    Base::fillCircle(world, pos, r * 0.36f, Base::shade(color, 0.55f));
    const float progress = chargeRequired > 0
                               ? Math::clamp(float(charge) / float(chargeRequired),
                                             0.0f, 1.0f)
                               : 0.0f;
    Base::arcRadians(world, pos, r + 7.0f, -Math::Pi / 2.0f,
                     Math::Tau * progress, Base::shade(color, 1.2f), 3.0f);
}

} // namespace arena
