#include "Menu/Depot/Depot.h"

#include "GameConfig.h"
#include "Shop/Shop.h"

#include <Qt>


namespace menu {

QRectF Depot::itemRect() const
{
    return QRectF(300.0f, 214.0f, 680.0f, 290.0f);
}

QRectF Depot::buyRect() const
{
    return QRectF(700.0f, 430.0f, 240.0f, 42.0f);
}

QRectF Depot::backRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 90.0f, 600.0f, 180.0f, 42.0f);
}

Depot::Action Depot::keyPress(int key)
{
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        ShopService.buyMine();
        return Action::None;
    }
    if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
    }
    return Action::None;
}

Depot::Action Depot::mousePress(const Math::Vec2 &position)
{
    const QPointF point(position.x, position.y);
    if (backRect().contains(point)) {
        return Action::Back;
    }
    if (buyRect().contains(point)) {
        ShopService.buyMine();
    }
    return Action::None;
}

} // namespace menu
