#include "Menu/Depot/Depot.h"

#include "Base/Base.h"
#include "Data/GameData.h"
#include "GameConfig.h"
#include "Shop/Shop.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <cmath>

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

void Depot::render(QPainter &painter) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 3, 9));

    QLinearGradient background(0, 0, 0, GameConfig::ScreenHeight);
    background.setColorAt(0.0, QColor(5, 15, 28));
    background.setColorAt(0.6, QColor(3, 8, 18));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     background);

    painter.save();
    painter.setPen(QPen(QColor(23, 50, 70), 1.0f));
    for (int x = 0; x < GameConfig::ScreenWidth; x += 80) {
        painter.drawLine(x, 0, x, GameConfig::ScreenHeight);
    }
    for (int y = 0; y < GameConfig::ScreenHeight; y += 80) {
        painter.drawLine(0, y, GameConfig::ScreenWidth, y);
    }
    painter.restore();

    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("SUPPLY DEPOT  //  ARMORY"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 124}, QStringLiteral("center"), 190);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("补给站"),
                   Base::White, {GameConfig::ScreenWidth / 2.0f, 176},
                   QStringLiteral("center"));
    Base::crate(painter, {GameConfig::ScreenWidth / 2.0f - 112.0f, 176.0f}, 20.0f,
                Base::Amber);

    Base::text(painter, Base::techSmallFont(),
               QStringLiteral("积分  %1").arg(ShopService.score().total()), Base::Amber,
               {GameConfig::ScreenWidth - 34.0f, 34.0f}, QStringLiteral("topright"),
               235);

    const QRectF card = itemRect();
    Base::panel(painter, card, Base::Amber, 225, 12, QColor(8, 20, 32));

    const Math::Vec2 iconCenter(float(card.left()) + 56.0f,
                                float(card.top()) + 62.0f);
    Base::fillCircle(painter, iconCenter, 32.0f, Base::withAlpha(Base::Amber, 40));
    Base::strokeCircle(painter, iconCenter, 32.0f, Base::Amber, 1.5f);
    Base::text(painter, Base::techTinyFont(), QStringLiteral("MINE"), Base::Amber,
               iconCenter, QStringLiteral("center"), 245);

    Base::text(painter, Base::mediumFont(), QStringLiteral("震荡地雷"), Base::White,
               {float(card.left()) + 112.0f, float(card.top()) + 40.0f},
               QStringLiteral("midleft"));
    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("IMPACT MINE  //  CONSUMABLE"),
               Base::Muted,
               {float(card.left()) + 113.0f, float(card.top()) + 66.0f},
               QStringLiteral("midleft"), 190);
    Base::text(painter, Base::techFont(),
               QStringLiteral("%1 积分").arg(data::kMine.price), Base::Amber,
               {float(card.right()) - 34.0f, float(card.top()) + 42.0f},
               QStringLiteral("topright"), 235);

    const QString description =
        QStringLiteral("右键在光标处布置，敌人进入感应范围即引爆，对范围内敌人造成大量伤害。"
                       "一次性消耗，购买后进入库存。");
    const QStringList lines = Base::wrapText(description, Base::bodyFont(),
                                             float(card.width()) - 80.0f);
    for (int i = 0; i < lines.size(); ++i) {
        Base::text(painter, Base::bodyFont(), lines.at(i), QColor(198, 219, 234),
                   {float(card.left()) + 40.0f,
                    float(card.top()) + 128.0f + float(i) * 28.0f},
                   QStringLiteral("topleft"), 225);
    }

    Base::strokeLine(painter, {float(card.left()) + 40.0f, float(card.bottom()) - 92.0f},
                     {float(card.right()) - 40.0f, float(card.bottom()) - 92.0f},
                     Base::shade(Base::Amber, 0.35f), 1.0f);
    Base::text(painter, Base::smallFont(),
               QStringLiteral("持有  ×%1").arg(ShopService.inventory().mines()), Base::Text,
               {float(card.left()) + 40.0f, float(card.bottom()) - 56.0f},
               QStringLiteral("midleft"), 220);

    const bool affordable = ShopService.score().total() >= data::kMine.price;
    Base::panel(painter, buyRect(), affordable ? Base::Amber : QColor(70, 84, 96),
                affordable ? 230 : 170, 9,
                affordable ? QColor(10, 24, 34) : QColor(8, 14, 22));
    Base::text(painter, Base::techSmallFont(),
               affordable ? QStringLiteral("购买  %1").arg(data::kMine.price)
                          : QStringLiteral("积分不足"),
               affordable ? Base::Amber : Base::Muted,
               {float(buyRect().center().x()), float(buyRect().center().y())},
               QStringLiteral("center"), affordable ? 240 : 190);

    Base::panel(painter, backRect(), QColor(65, 104, 124), 210, 9, QColor(7, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回"), Base::Text,
               {float(backRect().center().x()), float(backRect().center().y())},
               QStringLiteral("center"));

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("ENTER / 点击购买    ESC 返回    M 静音"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 34.0f},
               QStringLiteral("center"), 160);
}

} // namespace menu
