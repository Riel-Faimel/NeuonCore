#include "Pointer.h"

#include "Base/Base.h"

#include <QPainter>

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
    Base::reticle(painter, position_, time_);
    painter.restore();
}
