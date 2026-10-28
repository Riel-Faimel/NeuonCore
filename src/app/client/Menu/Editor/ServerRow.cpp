#include "Menu/Editor/ServerRow.h"

#include "Base/Base.h"
#include "Menu/Editor/Theme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>

namespace menu::editor {

QRectF ServerRow::rowRect(int waveCount)
{
    const float top = std::min(508.0f,
                               Theme::RowsTop + float(waveCount) * Theme::RowHeight
                                   + 4.0f);
    return QRectF(Theme::Left, top, Theme::Right - Theme::Left, 48.0f);
}

QRectF ServerRow::portRect(int waveCount)
{
    const QRectF row = rowRect(waveCount);
    return QRectF(Theme::Left + 360.0f, float(row.top()) + 4.0f, 250.0f, 40.0f);
}

void ServerRow::draw(QPainter &painter, int waveCount, const QString &portText,
                     bool focused, bool valid, const Pointer &pointer,
                     float pulse) const
{
    const QRectF row = rowRect(waveCount);
    const QColor tint = Base::Violet;
    const bool hoveredRow = pointer.hovering(row);

    QLinearGradient fill(0.0f, float(row.top()), 0.0f, float(row.bottom()));
    fill.setColorAt(0.0, hoveredRow ? QColor(14, 20, 40, 245)
                                    : QColor(9, 16, 32, 240));
    fill.setColorAt(1.0, QColor(5, 11, 22, 240));
    painter.save();
    painter.setBrush(fill);
    painter.setPen(QPen(Base::withAlpha(tint, hoveredRow ? 165 : 110), 1.2f));
    painter.drawRoundedRect(row, 9.0f, 9.0f);
    painter.setPen(Qt::NoPen);
    painter.setBrush(tint);
    painter.drawRoundedRect(QRectF(float(row.left()) + 1.0f,
                                   float(row.top()) + 10.0f, 2.5f,
                                   float(row.height()) - 20.0f),
                            1.2f, 1.2f);
    painter.restore();

    Base::text(painter, Theme::techSmallFont(), QStringLiteral("服务器端口"),
               Base::White,
               {float(row.left()) + 20.0f, float(row.center().y()) - 8.0f},
               QStringLiteral("midleft"), 235);
    Base::text(painter, Theme::techTinyFont(),
               QStringLiteral("SERVER PORT // TCP · LISTEN 0.0.0.0"), Base::Muted,
               {float(row.left()) + 20.0f, float(row.center().y()) + 11.0f},
               QStringLiteral("midleft"), 190);
    Base::strokeLine(painter, {float(row.left()) + 340.0f,
                               float(row.top()) + 14.0f},
                     {float(row.left()) + 340.0f, float(row.bottom()) - 14.0f},
                     Base::withAlpha(tint, 70), 1.0f);

    const QRectF field = portRect(waveCount);
    Theme::textField(painter, field, portText, QStringLiteral("端口"), focused,
                     valid, tint, pointer, pulse);

    Base::text(painter, Theme::tinyFont(),
               QStringLiteral("同一局域网客户端填本机地址 + 此端口"), Base::Muted,
               {float(field.right()) + 24.0f, float(row.center().y())},
               QStringLiteral("midleft"), 200);

    const QRectF pill(Theme::Right - 166.0f, float(row.center().y()) - 13.0f,
                      142.0f, 26.0f);
    painter.save();
    painter.setBrush(QColor(3, 10, 18, 230));
    painter.setPen(QPen(Base::withAlpha(valid ? tint : Base::Red, 150), 1.0f));
    painter.drawRoundedRect(pill, 13.0f, 13.0f);
    painter.restore();
    Base::fillCircle(painter, {float(pill.left()) + 14.0f, float(pill.center().y())},
                     2.6f, valid ? Base::Green : Base::Red);
    Base::text(painter, Theme::techTinyFont(),
               valid ? QStringLiteral("TCP 0.0.0.0:%1").arg(portText)
                     : QStringLiteral("端口 1–65535"),
               valid ? Base::Text : Base::Red,
               {float(pill.left()) + 24.0f, float(pill.center().y())},
               QStringLiteral("midleft"), 220);
}

} // namespace menu::editor
