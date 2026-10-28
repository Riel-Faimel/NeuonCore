#include "Menu/Editor/Theme.h"

#include "Base/Base.h"

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>

#include <algorithm>

namespace menu::editor {

QColor Theme::accent(Mode mode)
{
    return tabAccent(int(mode));
}

QColor Theme::tabAccent(int index)
{
    switch (index) {
    case 0:
        return Base::Cyan;
    case 1:
        return Base::Violet;
    default:
        return Base::Amber;
    }
}

const QFont &Theme::tinyFont()
{
    return Base::tinyFont();
}

const QFont &Theme::smallFont()
{
    return Base::smallFont();
}

const QFont &Theme::bodyFont()
{
    return Base::bodyFont();
}

const QFont &Theme::mediumFont()
{
    return Base::mediumFont();
}

const QFont &Theme::largeFont()
{
    return Base::largeFont();
}

const QFont &Theme::bigFont()
{
    return Base::bigFont();
}

const QFont &Theme::techTinyFont()
{
    return Base::techTinyFont();
}

const QFont &Theme::techSmallFont()
{
    return Base::techSmallFont();
}

const QFont &Theme::techFont()
{
    return Base::techFont();
}

void Theme::panel(QPainter &painter, const QRectF &rect, const QColor &border,
                  int alpha, int radius, const QColor &fill)
{
    Base::panel(painter, rect, border, alpha, radius, fill);
}

void Theme::keycap(QPainter &painter, const QRectF &rect, const QString &text,
                   const QColor &border, const QColor &textColor, int textAlpha,
                   float radius)
{
    painter.save();
    painter.setBrush(QColor(3, 10, 18, 235));
    painter.setPen(QPen(border, 1.0f));
    painter.drawRoundedRect(rect, radius, radius);
    painter.restore();
    Base::text(painter, techTinyFont(), text, textColor,
               {float(rect.center().x()), float(rect.center().y())},
               QStringLiteral("center"), textAlpha);
}

void Theme::button(QPainter &painter, const QRectF &rect, const QString &key,
                   const QString &label, const QColor &tint, bool primary,
                   bool enabled, const Pointer &pointer)
{
    const bool hovered = enabled && pointer.hovering(rect);
    QColor border = primary ? tint : QColor(58, 96, 118);
    if (!enabled) {
        border = QColor(48, 62, 76);
    } else if (hovered && !primary) {
        border = Base::withAlpha(tint, 185);
    }

    painter.save();
    QLinearGradient fill(0.0f, float(rect.top()), 0.0f, float(rect.bottom()));
    if (!enabled) {
        fill.setColorAt(0.0, QColor(8, 14, 22, 220));
        fill.setColorAt(1.0, QColor(5, 10, 18, 220));
    } else if (primary) {
        fill.setColorAt(0.0, Base::withAlpha(tint, hovered ? 82 : 54));
        fill.setColorAt(1.0, QColor(6, 16, 28, 238));
    } else {
        fill.setColorAt(0.0, hovered ? QColor(12, 28, 42, 242)
                                     : QColor(8, 19, 31, 232));
        fill.setColorAt(1.0, QColor(5, 12, 22, 232));
    }
    painter.setBrush(fill);
    painter.setPen(QPen(border, primary && enabled ? 1.5f : 1.0f));
    painter.drawRoundedRect(rect, 8.0f, 8.0f);
    if (enabled && hovered) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Base::withAlpha(tint, 170));
        painter.drawRoundedRect(QRectF(rect.left() + 12.0f, rect.bottom() - 2.5f,
                                       rect.width() - 24.0f, 1.5f),
                                0.8f, 0.8f);
    }
    painter.restore();

    const QFontMetricsF metrics(techTinyFont());
    const float keyWidth = std::max(22.0f, float(metrics.horizontalAdvance(key)) + 12.0f);
    const QRectF badge(rect.left() + 12.0f, float(rect.center().y()) - 10.0f,
                       keyWidth, 20.0f);
    painter.save();
    painter.setBrush(QColor(3, 10, 18, 235));
    painter.setPen(QPen(enabled ? Base::withAlpha(primary ? tint : QColor(96, 134, 154), 190)
                                : QColor(48, 62, 76),
                        1.0f));
    painter.drawRoundedRect(badge, 4.0f, 4.0f);
    painter.restore();
    Base::text(painter, techTinyFont(), key,
               enabled ? (primary ? tint : Base::Text) : Base::Dim,
               {float(badge.center().x()), float(badge.center().y())},
               QStringLiteral("center"), 235);
    Base::text(painter, techSmallFont(), label,
               enabled ? (primary ? Base::White : Base::Text) : Base::Dim,
               {float(badge.right()) + 9.0f, float(rect.center().y())},
               QStringLiteral("midleft"), 232);
}

void Theme::textField(QPainter &painter, const QRectF &rect, const QString &text,
                      const QString &placeholder, bool focused, bool valid,
                      const QColor &tint, const Pointer &pointer, float pulse)
{
    const bool hovered = pointer.hovering(rect);
    QColor border = QColor(52, 88, 110);
    if (!valid) {
        border = Base::Red;
    } else if (focused) {
        border = tint;
    } else if (hovered) {
        border = Base::withAlpha(tint, 150);
    }

    painter.save();
    painter.setBrush(QColor(3, 10, 18, 245));
    painter.setPen(QPen(border, focused ? 1.6f : 1.1f));
    painter.drawRoundedRect(rect, 8.0f, 8.0f);
    if (focused) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Base::withAlpha(tint, 220));
        painter.drawRoundedRect(QRectF(float(rect.left()) + 1.0f,
                                       float(rect.top()) + 9.0f, 2.2f,
                                       float(rect.height()) - 18.0f),
                                1.1f, 1.1f);
    }
    painter.restore();

    const bool empty = text.isEmpty();
    painter.save();
    painter.setClipRect(rect.adjusted(10.0f, 0.0f, -12.0f, 0.0f));
    if (empty) {
        if (!focused) {
            Base::text(painter, smallFont(), placeholder, Base::Dim,
                       {float(rect.left()) + 14.0f, float(rect.center().y())},
                       QStringLiteral("midleft"), 200);
        }
    } else {
        Base::text(painter, techFont(), text, Base::White,
                   {float(rect.left()) + 14.0f, float(rect.center().y())},
                   QStringLiteral("midleft"), 240);
    }
    painter.restore();

    if (focused) {
        const float advance = empty
                                  ? 0.0f
                                  : float(QFontMetricsF(techFont())
                                              .horizontalAdvance(text));
        const float caretX = float(rect.left()) + 14.0f + advance + 2.0f;
        Base::strokeLine(painter, {caretX, float(rect.top()) + 12.0f},
                         {caretX, float(rect.bottom()) - 12.0f},
                         Base::withAlpha(tint, int(150.0f + 90.0f * pulse)), 1.6f);
    }
}

} // namespace menu::editor
