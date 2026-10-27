#include "Menu/Failure/Failure.h"

#include "Base/Base.h"
#include "GameConfig.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <cmath>

namespace menu {

QRectF Failure::retryRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 270.0f, 508.0f, 250.0f, 48.0f);
}

QRectF Failure::backRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f + 20.0f, 508.0f, 250.0f, 48.0f);
}

void Failure::render(QPainter &painter, const View &view) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(3, 2, 7));

    QLinearGradient background(0, 0, 0, GameConfig::ScreenHeight);
    background.setColorAt(0.0, QColor(24, 8, 15));
    background.setColorAt(0.55, QColor(9, 5, 12));
    background.setColorAt(1.0, QColor(2, 2, 7));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     background);

    for (int i = 0; i < 90; ++i) {
        const float x = std::fmod(float(i) * 149.0f + 53.0f,
                                  float(GameConfig::ScreenWidth));
        const float y = std::fmod(float(i) * 89.0f + 31.0f,
                                  float(GameConfig::ScreenHeight));
        Base::fillCircle(painter, {x, y}, 0.9f,
                         QColor(228, 92, 112, 26 + (i % 4) * 9));
    }

    const QColor accent = Base::Red;
    Base::strokeLine(painter, {GameConfig::ScreenWidth / 2.0f - 250.0f, 132.0f},
                     {GameConfig::ScreenWidth / 2.0f + 250.0f, 132.0f},
                     Base::withAlpha(accent, 200), 1.2f);
    Base::text(painter, Base::techSmallFont(),
               QStringLiteral("CONNECTION FAILED  //  NO AUTHORITATIVE SERVER"),
               accent, {GameConfig::ScreenWidth / 2.0f, 166.0f},
               QStringLiteral("center"), 230);
    Base::glowText(painter, Base::bigFont(), view.title, QColor(255, 139, 150),
                   {GameConfig::ScreenWidth / 2.0f, 236.0f}, QStringLiteral("center"));
    Base::text(painter, Base::bodyFont(),
               QStringLiteral("本机没有连接到可用的房间服务器。"),
               QColor(220, 191, 200), {GameConfig::ScreenWidth / 2.0f, 286.0f},
               QStringLiteral("center"), 225);
    Base::text(painter, Base::smallFont(), view.detail, Base::Muted,
               {GameConfig::ScreenWidth / 2.0f, 326.0f}, QStringLiteral("center"), 210);

    const QRectF card(GameConfig::ScreenWidth / 2.0f - 320.0f, 362.0f, 640.0f, 92.0f);
    QLinearGradient cardFill(0.0f, float(card.top()), 0.0f, float(card.bottom()));
    cardFill.setColorAt(0.0, QColor(24, 12, 20, 240));
    cardFill.setColorAt(1.0, QColor(9, 6, 12, 240));
    painter.save();
    painter.setBrush(cardFill);
    painter.setPen(QPen(Base::withAlpha(accent, 130), 1.0f));
    painter.drawRoundedRect(card, 10.0f, 10.0f);
    painter.restore();

    Base::fillCircle(painter, {float(card.left()) + 24.0f, float(card.center().y()) - 14.0f},
                     3.2f, accent);
    Base::text(painter, Base::techTinyFont(), QStringLiteral("TARGET  目标地址"),
               Base::Muted, {float(card.left()) + 40.0f, float(card.top()) + 24.0f},
               QStringLiteral("midleft"), 190);
    Base::text(painter, Base::techFont(),
               view.endpoint.isEmpty() ? QStringLiteral("未指定服务器") : view.endpoint,
               Base::White, {float(card.left()) + 40.0f, float(card.top()) + 54.0f},
               QStringLiteral("midleft"), 235);
    Base::text(painter, Base::tinyFont(),
               QStringLiteral("确认主机已启动、地址端口正确；若房主已解散，请让房主重新开房。"),
               Base::Muted, {float(card.right()) - 24.0f, float(card.center().y())},
               QStringLiteral("midright"), 195);

    if (view.retryable) {
        Base::panel(painter, retryRect(), Base::Cyan, 230, 9, QColor(7, 18, 31));
        Base::text(painter, Base::techSmallFont(), QStringLiteral("ENTER  重试连接"),
                   Base::Cyan,
                   {float(retryRect().center().x()), float(retryRect().center().y())},
                   QStringLiteral("center"));
    }
    Base::panel(painter, backRect(), QColor(65, 104, 124), 210, 9, QColor(7, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回主菜单"),
               QColor(190, 215, 228),
               {float(backRect().center().x()), float(backRect().center().y())},
               QStringLiteral("center"));

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("这不是对局结算：房间从未进入战斗，或主机已关闭房间。"),
               QColor(154, 122, 132), {GameConfig::ScreenWidth / 2.0f, 602.0f},
               QStringLiteral("center"), 190);
}

Failure::Action Failure::keyPress(int key) const
{
    if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_R) {
        return Action::Retry;
    }
    if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
    }
    return Action::None;
}

Failure::Action Failure::mousePress(const Math::Vec2 &position) const
{
    const QPointF point(position.x, position.y);
    if (retryRect().contains(point)) {
        return Action::Retry;
    }
    if (backRect().contains(point)) {
        return Action::Back;
    }
    return Action::None;
}

} // namespace menu
