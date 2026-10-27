#pragma once

#include "Math.h"

#include <QRectF>
#include <QString>

class QPainter;

namespace menu {

// Result 组件：对局结算覆盖层。只接收一份只读 Summary，
// 不读取 Games，也不决定重开还是回菜单（由 Menu 执行）。
class Result {
public:
    struct Summary {
        bool victory = false;
        QString levelName;
        int score = 0;
        int kills = 0;
        int wave = 0;
        int totalWaves = 0;
        float runTime = 0.0f;
    };

    enum class Action {
        None,
        Retry,
        Back,
    };

    void render(QPainter &painter, const Summary &summary) const;
    Action keyPress(int key) const;
    Action mousePress(const Math::Vec2 &position) const;

    QRectF retryRect() const;
    QRectF menuRect() const;
};

} // namespace menu
