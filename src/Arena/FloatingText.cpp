#include "FloatingText.h"

#include "base/Base.h"

#include <algorithm>
#include <cmath>

namespace arena {

void FloatingText::update(float dt)
{
    life -= dt;
    pos.y -= 38.0f * dt;
    pos.x += std::sin((maxLife - life) * 12.0f) * 9.0f * dt;
}

void FloatingText::draw(QPainter &world) const
{
    if (life <= 0.0f) {
        return;
    }
    const float t = Math::clamp(life / maxLife, 0.0f, 1.0f);
    const int alpha = int(255.0f * std::min(1.0f, t * 1.8f));
    const QFont &font = crit ? Base::bodyFont() : Base::smallFont();
    const QString label = crit ? QStringLiteral("暴击 ") + text : text;
    Base::text(world, font, label, color, pos, QStringLiteral("center"), alpha);
}

} // namespace arena
