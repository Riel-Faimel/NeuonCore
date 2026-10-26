#pragma once

#include "Math.h"

#include <QRectF>

class QPainter;

namespace menu {

// Map 组件：标准协议的星区地图。节点数据来自 src/json/levels.json，
// 当前关卡作为第一个节点；未解锁节点只展示、不可进入。
class Map {
public:
    enum class Action {
        None,
        Start,
        Back,
    };

    struct Command {
        Action action = Action::None;
        int level = 0;
    };

    void reset();
    void update(float dt, const Math::Vec2 &mouse, bool mouseActive);
    void render(QPainter &painter) const;

    Command keyPress(int key);
    Command mousePress(const Math::Vec2 &position);

    QRectF nodeRect(int index) const;
    QRectF backRect() const;
    int selected() const { return selected_; }

private:
    int selected_ = 0;
    float time_ = 0.0f;
    float denied_ = 0.0f;
};

} // namespace menu
