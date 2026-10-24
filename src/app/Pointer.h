#pragma once

#include "GameConfig.h"
#include "Math.h"

class QPainter;

// Pointer 模块：全局鼠标指针。
// 它不属于某一局、也不属于某个画面，而是由窗口在每帧最后叠加到画面上，
// 所以所有界面共享同一个指针和同一个动画相位（局内暂停时外弧也照常旋转）。
class Pointer {
public:
    void moveTo(const Math::Vec2 &position);
    void setVisible(bool visible) { visible_ = visible; }
    bool visible() const { return visible_; }

    void update(float dt) { time_ += dt; }
    void draw(QPainter &painter) const;

private:
    Math::Vec2 position_{GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight / 2.0f};
    float time_ = 0.0f;
    bool visible_ = false;
};
