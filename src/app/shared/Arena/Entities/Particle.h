#pragma once

#include "Math.h"

#include <QColor>
#include <QPainter>

namespace arena {

// 粒子实体。
class Particle {
public:
    Math::Vec2 pos;
    Math::Vec2 vel;
    float life = 0.0f;
    float maxLife = 1.0f;
    float radius = 2.0f;
    QColor color;
    float drag = 2.2f;
    float gravity = 0.0f;
    int layer = 1;

    void update(float dt);
    void draw(QPainter &world, QPainter &glow) const;
};

} // namespace arena
