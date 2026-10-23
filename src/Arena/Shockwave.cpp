#include "Shockwave.h"

#include "base/Base.h"

#include <algorithm>
#include <cmath>

namespace arena {

Shockwave::Shockwave(const Math::Vec2 &position, float radius,
                     const QColor &waveColor, float durationValue, int lineWidth)
    : pos(position), maxRadius(radius), color(waveColor), duration(durationValue),
      life(durationValue), width(lineWidth)
{
}

void Shockwave::update(float dt)
{
    life -= dt;
}

void Shockwave::draw(QPainter &world, QPainter &glow) const
{
    if (life <= 0.0f) {
        return;
    }
    const float t = 1.0f - life / duration;
    const float radius = std::max(2.0f, maxRadius * Math::easeOutCubic(t));
    const int alpha = int(190.0f * std::pow(1.0f - t, 1.5f));
    if (alpha <= 0) {
        return;
    }
    Base::strokeCircle(world, pos, radius, Base::withAlpha(color, alpha),
                       float(width));
    Base::strokeCircle(world, pos, std::max(1.0f, radius - 5.0f),
                       Base::withAlpha(color, int(alpha * 0.4f)), 1.0f);
    Base::strokeCircle(glow, pos, radius, Base::shade(color, 0.25f),
                       float(std::max(1, width)));
}

} // namespace arena
