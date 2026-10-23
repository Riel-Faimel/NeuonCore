#pragma once

#include "Math.h"

#include <QSet>

namespace base {

// Input 组件：只保存原始键鼠状态并提供归一化查询。
class Input {
public:
    Input();

    void keyPress(int key);
    void keyRelease(int key);
    void mouseMove(const Math::Vec2 &position);
    void mousePressLeft(const Math::Vec2 &position);
    void mouseReleaseLeft();
    void clear();

    Math::Vec2 movementAxis() const;
    bool shootingHeld() const;
    bool mouseDown() const;
    bool mouseActive() const;
    Math::Vec2 mousePosition() const;
    void setMouseActive(bool active);

private:
    QSet<int> keys_;
    Math::Vec2 mousePosition_;
    bool mouseActive_ = false;
    bool mouseDown_ = false;
};

} // namespace base
