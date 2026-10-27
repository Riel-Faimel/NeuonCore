#include "Mine.h"

#include "Base/Base.h"

#include <cmath>

namespace arena {

void Mine::update(float dt)
{
    age += dt;
    life -= dt;
    if (life <= 0.0f) {
        alive = false;
    }
}

void Mine::draw(QPainter &world, QPainter &glow) const
{
    if (!alive) {
        return;
    }
    const bool armed = age >= armTime;
    const float pulse = 0.5f + 0.5f * std::sin(age * 7.0f);
    const QColor color = armed ? Base::Amber : Base::Muted;

    QPolygonF diamond;
    diamond << QPointF(pos.x, pos.y - radius) << QPointF(pos.x + radius, pos.y)
            << QPointF(pos.x, pos.y + radius) << QPointF(pos.x - radius, pos.y);
    Base::polygon(world, diamond, Base::withAlpha(color, armed ? 75 : 30), color, 1.6f);
    Base::strokeCircle(world, pos, radius + 4.0f + pulse * 2.0f,
                       Base::withAlpha(color, armed ? 160 : 70), 1.0f);
    Base::fillCircle(world, pos, 3.0f, Base::White);
    Base::glow(glow, pos, radius * 1.8f, color, armed ? 0.8f : 0.3f);
}

} // namespace arena
