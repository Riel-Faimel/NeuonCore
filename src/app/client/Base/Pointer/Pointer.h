#pragma once

#include "GameConfig.h"
#include "Math.h"

class QPainter;

namespace base {

// Pointer 组件：全局鼠标指针。
// 它不属于某一局、也不属于某个画面，而是由窗口每帧最后通过 BaseService 叠加到画面上，
// 所以所有界面共享同一个指针和同一个动画相位（局内暂停时外弧也照常旋转）。
// 准星的画法只有一份，在 base::Paint::reticle()。
class Pointer {
public:
    void moveTo(const Math::Vec2 &position);
    void setVisible(bool visible) { visible_ = visible; }

    void update(float dt) { time_ += dt; }
    void draw(QPainter &painter) const;
    const Math::Vec2 &position() const { return position_; }
    float time() const { return time_; }
    bool visible() const { return visible_; }

private:
    Math::Vec2 position_{GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight / 2.0f};
    float time_ = 0.0f;
    bool visible_ = false;
};

} // namespace base
