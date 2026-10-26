#pragma once

#include "Math.h"

#include <QRectF>

class QPainter;

namespace menu {

// Depot 组件：局外补给站界面。购买逻辑在全局 Shop 模块里，
// 这里只负责展示钱包/库存、命中测试和把“购买”意图转给 Shop。
class Depot {
public:
    enum class Action {
        None,
        Back,
    };

    void reset();
    Action keyPress(int key);
    Action mousePress(const Math::Vec2 &position);
    void render(QPainter &painter) const;

    QRectF itemRect() const;
    QRectF buyRect() const;
    QRectF backRect() const;

private:
    int purchases_ = 0;
};

} // namespace menu
