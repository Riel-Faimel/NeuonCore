#pragma once

#include "Math.h"

#include <QColor>
#include <QPainter>
#include <QString>

namespace arena {

// 飘字实体。
class FloatingText {
public:
    Math::Vec2 pos;
    QString text;
    QColor color;
    float life = 0.8f;
    float maxLife = 0.8f;
    int size = 15;
    bool crit = false;

    void update(float dt);
    void draw(QPainter &world) const;
};

} // namespace arena
