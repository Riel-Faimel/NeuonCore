#pragma once

#include "Math.h"

#include <QRectF>

class QPainter;

namespace menu {

// Armory 组件：局外“战备”界面，选择玩家使用的发射器。
// 只负责展示与选择；数值在 src/json/loadout.json，选择由全局 Loadout 持久化。
class Armory {
public:
    enum class Action {
        None,
        Back,
    };

    void open();
    Action keyPress(int key);
    Action mousePress(const Math::Vec2 &position);
    void render(QPainter &painter) const;

    QRectF cardRect(int index) const;
    QRectF backRect() const;
    int selected() const { return selected_; }

private:
    int selected_ = 0;
};

} // namespace menu
