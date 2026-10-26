#include "Input.h"

#include "GameConfig.h"

#include <Qt>

namespace base {

Input::Input()
    : mousePosition_{GameConfig::ScreenWidth / 2.0f,
                     GameConfig::ScreenHeight / 2.0f}
{
}

void Input::keyPress(int key)
{
    keys_.insert(key);
}

void Input::keyRelease(int key)
{
    keys_.remove(key);
}

void Input::mouseMove(const Math::Vec2 &position)
{
    mousePosition_ = position;
    mouseActive_ = true;
}

void Input::mousePressLeft(const Math::Vec2 &position)
{
    mousePosition_ = position;
    mouseActive_ = true;
    mouseDown_ = true;
}

void Input::mouseReleaseLeft()
{
    mouseDown_ = false;
}

void Input::clear()
{
    keys_.clear();
    mouseDown_ = false;
}

Math::Vec2 Input::movementAxis() const
{
    Math::Vec2 axis;
    if (keys_.contains(Qt::Key_D) || keys_.contains(Qt::Key_Right)) {
        axis.x += 1.0f;
    }
    if (keys_.contains(Qt::Key_A) || keys_.contains(Qt::Key_Left)) {
        axis.x -= 1.0f;
    }
    if (keys_.contains(Qt::Key_S) || keys_.contains(Qt::Key_Down)) {
        axis.y += 1.0f;
    }
    if (keys_.contains(Qt::Key_W) || keys_.contains(Qt::Key_Up)) {
        axis.y -= 1.0f;
    }
    if (axis.lengthSquared() > 0.0f) {
        axis = axis.normalized();
    }
    return axis;
}

bool Input::shootingHeld() const
{
    return mouseDown_ || keys_.contains(Qt::Key_J);
}

bool Input::mouseActive() const
{
    return mouseActive_;
}

Math::Vec2 Input::mousePosition() const
{
    return mousePosition_;
}

void Input::setMouseActive(bool active)
{
    mouseActive_ = active;
}

} // namespace base
