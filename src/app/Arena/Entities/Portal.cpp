#include "Portal.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Combatants/Enemy.h"
#include "Base/Base.h"

#include <algorithm>
#include <cmath>

namespace arena {

Portal::Portal(const QString &portalKind, const Math::Vec2 &position, int waveNumber)
    : kind(portalKind), pos(position), wave(waveNumber)
{
    color = Enemy::statsFor(kind).color;
    duration = kind == QStringLiteral("boss") ? 1.25f : 0.66f;
    timer = duration;
    radius = kind == QStringLiteral("boss") ? 54.0f : 23.0f;
}

void Portal::update(const Context &ctx, float dt)
{
    timer -= dt;
    if (timer <= 0.0f) {
        alive = false;
        ctx.combatants.spawnEnemy(ctx, kind, pos, wave);
        ctx.entities.spawnBurst(ctx, pos, color,
                                 kind == QStringLiteral("boss") ? 16 : 8,
                                 160.0f, 0.5f);
    }
}

void Portal::draw(QPainter &world, QPainter &glow) const
{
    if (!alive) {
        return;
    }
    const float progress = 1.0f - timer / duration;
    const float pulse = 0.5f + 0.5f * std::sin(progress * 22.0f);
    const float drawRadius = radius * (1.15f - 0.25f * progress);
    Base::glow(glow, pos, radius * 1.2f, color, 0.8f + pulse * 0.8f);
    Base::strokeCircle(world, pos, drawRadius,
                       Base::withAlpha(color, int(45.0f + 80.0f * pulse)), 2.0f);
    Base::strokeCircle(world, pos, std::max(2.0f, drawRadius * 0.5f),
                       Base::withAlpha(color, 70), 1.0f);
    for (int i = 0; i < 4; ++i) {
        const float angle = progress * 7.0f + Math::Tau * float(i) / 4.0f;
        const Math::Vec2 direction(std::cos(angle), std::sin(angle));
        Base::strokeLine(world, pos + direction * (drawRadius + 4.0f),
                         pos + direction * (drawRadius + 16.0f), color, 2.0f);
    }
    Base::fillCircle(world, pos, 2.0f, QColor(255, 245, 250));
}

} // namespace arena
