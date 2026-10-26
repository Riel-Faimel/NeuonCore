#include "Base/Pointer/Pointer.h"

#include "Base/Paint/Paint.h"

#include <QPainter>

namespace base {

void Pointer::moveTo(const Math::Vec2 &position)
{
    position_ = position;
    visible_ = true;
}

void Pointer::draw(QPainter &painter) const
{
    if (!visible_) {
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    Paint::reticle(painter, position_, time_);
    painter.restore();
}

} // namespace base
