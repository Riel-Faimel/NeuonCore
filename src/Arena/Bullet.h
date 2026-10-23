#pragma once

#include "Math.h"

#include <QColor>
#include <QPainter>

#include <unordered_set>

namespace arena {

// 子弹实体。
class Bullet {
public:
    Math::Vec2 pos;
    Math::Vec2 prev;
    Math::Vec2 vel;
    float damage = 10.0f;
    QColor color;
    bool friendly = true;
    bool alive = true;
    float radius = 3.0f;
    int pierce = 0;
    float critChance = 0.0f;
    float life = 1.6f;
    std::unordered_set<int> hits;

    Bullet() = default;
    Bullet(const Math::Vec2 &position, const Math::Vec2 &velocity, float damageValue,
           const QColor &bulletColor, bool isFriendly, float radiusValue = 3.0f,
           int pierceValue = 0, float critChanceValue = 0.0f);
    void update(float dt);
    void draw(QPainter &world, QPainter &glow) const;
};

} // namespace arena
