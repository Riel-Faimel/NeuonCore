#pragma once

#include "Math.h"

#include <QColor>
#include <QPainter>

class Arena;

namespace arena {

// 掉落实体：靠近玩家时被吸附并治疗。
class Pickup {
public:
    Math::Vec2 pos;
    Math::Vec2 vel;
    int amount = 16;
    float age = 0.0f;
    float life = 14.0f;
    float phase = 0.0f;
    bool alive = true;
    float radius = 10.0f;

    Pickup(const Math::Vec2 &position, int healAmount);
    void update(Arena &arena, float dt);
    void draw(QPainter &world, QPainter &glow) const;
};

} // namespace arena
