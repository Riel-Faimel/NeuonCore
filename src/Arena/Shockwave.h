#pragma once

#include "Math.h"

#include <QColor>
#include <QPainter>

namespace arena {

// 冲击波效果实体。
class Shockwave {
public:
    Math::Vec2 pos;
    float maxRadius = 50.0f;
    QColor color;
    float duration = 0.38f;
    float life = 0.38f;
    int width = 2;

    Shockwave() = default;
    Shockwave(const Math::Vec2 &position, float radius, const QColor &waveColor,
              float durationValue = 0.38f, int lineWidth = 2);
    void update(float dt);
    void draw(QPainter &world, QPainter &glow) const;
};

} // namespace arena
