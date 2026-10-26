#include "Pickup.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Entities/FloatingText.h"
#include "Arena/Combatants/Player.h"
#include "Base/Base.h"

#include <algorithm>
#include <cmath>

namespace arena {

Pickup::Pickup(const Math::Vec2 &position, int healAmount)
    : pos(position), amount(healAmount)
{
}

void Pickup::update(const Context &ctx, float dt)
{
    age += dt;
    life -= dt;
    if (life <= 0.0f) {
        alive = false;
        return;
    }
    const Math::Vec2 toPlayer = ctx.combatants.player().state.pos - pos;
    const float distance = toPlayer.length();
    if (distance < 155.0f && distance > 0.1f) {
        const Math::Vec2 direction = toPlayer / distance;
        const float pull = (1.0f - distance / 155.0f) * 980.0f;
        vel += direction * pull * dt;
    }
    vel = vel * std::exp(-2.4f * dt);
    pos += vel * dt;
    if (distance < 22.0f) {
        const float before = ctx.combatants.player().state.hp;
        ctx.combatants.player().state.hp =
            std::min(ctx.combatants.player().state.maxHp,
                     ctx.combatants.player().state.hp + float(amount));
        const int gained = int(ctx.combatants.player().state.hp - before);
        if (gained > 0) {
            ctx.entities.floatingTexts().push_back(FloatingText{
                ctx.combatants.player().state.pos + Math::Vec2(0.0f, -28.0f),
                QStringLiteral("+%1").arg(gained), Base::Green,
                0.9f, 0.9f, 16, false});
        }
        alive = false;
        BaseService.playSound(QStringLiteral("pickup"));
        ctx.entities.spawnBurst(ctx, pos, Base::Green, 12, 120.0f, 0.45f);
        ctx.entities.addShockwave(pos, 36.0f, Base::Green, 0.3f, 2);
    }
}

void Pickup::draw(QPainter &world, QPainter &glow) const
{
    if (!alive) {
        return;
    }
    const float bob = std::sin(age * 4.0f + phase) * 3.0f;
    const Math::Vec2 drawPos = pos + Math::Vec2(0.0f, bob);
    const bool visible = life > 3.0f || int(life * 8.0f) % 2 == 0;
    if (!visible) {
        return;
    }
    Base::glow(glow, drawPos, 22.0f, Base::Green, 1.1f);
    Base::fillCircle(world, drawPos, 11.0f, QColor(18, 61, 48));
    Base::strokeCircle(world, drawPos, 11.0f, Base::Green, 2.0f);
    world.fillRect(QRectF(drawPos.x - 5.0f, drawPos.y - 1.0f, 10.0f, 3.0f),
                   Base::Green);
    world.fillRect(QRectF(drawPos.x - 1.0f, drawPos.y - 5.0f, 3.0f, 10.0f),
                   Base::Green);
    Base::fillCircle(world, drawPos, 2.0f, QColor(220, 255, 236));
}

} // namespace arena
