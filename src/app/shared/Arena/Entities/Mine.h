#pragma once

#include "Math.h"

#include <QColor>
#include <QPainter>

namespace arena {

// 地雷实体：局外商店购买、局内右键布置的一次性爆炸物。
// 布下后先经过 armTime 进入待发状态，敌人进入 triggerRadius 即引爆。
struct Mine {
    Math::Vec2 pos;
    float radius = 14.0f;
    float triggerRadius = 26.0f;
    float blastRadius = 110.0f;
    float damage = 240.0f;
    float armTime = 0.35f;
    float life = 22.0f;
    float age = 0.0f;
    bool alive = true;

    void update(float dt);
    void draw(QPainter &world, QPainter &glow) const;
};

} // namespace arena
