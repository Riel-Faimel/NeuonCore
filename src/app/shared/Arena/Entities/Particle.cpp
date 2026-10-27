#include "Particle.h"

#include "Base/Base.h"

#include <algorithm>
#include <cmath>

namespace arena {

void Particle::update(float dt)
{
    life -= dt;
    vel = vel * std::exp(-drag * dt);
    vel.y += gravity * dt;
    pos += vel * dt;
}

void Particle::draw(QPainter &world, QPainter &glow) const
{
    if (life <= 0.0f) {
        return;
    }
    const float t = Math::clamp(life / maxLife, 0.0f, 1.0f);
    const int alpha = int(210.0f * std::min(1.0f, t * 1.7f));
    const float drawRadius = std::max(1.0f, radius * (0.25f + 0.75f * t));
    if (layer == 1) {
        Base::fillCircle(world, pos, drawRadius, Base::withAlpha(color, alpha));
    }
    Base::glow(glow, pos, radius * (0.8f + 0.9f * t), color, 0.8f + 0.8f * t);
}

} // namespace arena
