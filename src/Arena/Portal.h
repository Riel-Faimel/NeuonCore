#pragma once

#include "Math.h"

#include <QColor>
#include <QPainter>
#include <QString>

class Arena;

namespace arena {

// 传送门实体：倒计时结束后生成敌人。
class Portal {
public:
    QString kind;
    Math::Vec2 pos;
    int wave = 1;
    QColor color;
    float duration = 0.66f;
    float timer = 0.66f;
    bool alive = true;
    float radius = 23.0f;

    Portal(const QString &portalKind, const Math::Vec2 &position, int waveNumber);
    void update(Arena &arena, float dt);
    void draw(QPainter &world, QPainter &glow) const;
};

} // namespace arena
