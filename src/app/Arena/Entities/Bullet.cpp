#include "Bullet.h"

#include "GameConfig.h"
#include "Base/Base.h"

namespace arena {

Bullet::Bullet(const Math::Vec2 &position, const Math::Vec2 &velocity,
               float damageValue, const QColor &bulletColor, bool isFriendly,
               float radiusValue, int pierceValue, float critChanceValue)
    : pos(position), prev(position), vel(velocity), damage(damageValue),
      color(bulletColor), friendly(isFriendly), radius(radiusValue),
      pierce(pierceValue), critChance(critChanceValue),
      life(isFriendly ? 1.6f : 4.5f)
{
}

void Bullet::update(float dt)
{
    prev = pos;
    pos += vel * dt;
    life -= dt;
    if (life <= 0.0f) {
        alive = false;
    }
    if (pos.x < -50.0f || pos.x > GameConfig::ScreenWidth + 50.0f
        || pos.y < -50.0f || pos.y > GameConfig::ScreenHeight + 50.0f) {
        alive = false;
    }
}

void Bullet::draw(QPainter &world, QPainter &glow) const
{
    if (!alive) {
        return;
    }
    const Math::Vec2 direction = vel.lengthSquared() > 0.0f
                                     ? vel.normalized()
                                     : Math::Vec2(1.0f, 0.0f);
    const Math::Vec2 tail = pos - direction * (friendly ? 8.0f : 6.0f);
    Base::strokeLine(world, tail, pos, Base::withAlpha(color, 105), 2.0f);
    Base::fillCircle(world, pos, radius, color);
    Base::fillCircle(world, pos, std::max(1.0f, radius * 0.42f), Base::White);
    Base::glow(glow, pos, radius * 3.0f, color, 1.0f);
}

} // namespace arena
