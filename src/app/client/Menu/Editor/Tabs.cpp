#include "Menu/Editor/Tabs.h"

#include "Base/Base.h"
#include "GameConfig.h"
#include "Menu/Editor/Theme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

namespace menu::editor {

namespace {
const QString kTabLabels[Tabs::Count] = {
    QStringLiteral("单机"), QStringLiteral("局域网主机"),
    QStringLiteral("加入主机")};
const QString kTabSubtitles[Tabs::Count] = {
    QStringLiteral("LOCAL SCRIPT"), QStringLiteral("HOST SESSION"),
    QStringLiteral("JOIN SESSION")};
const QString kTabKeys[Tabs::Count] = {
    QStringLiteral("F1"), QStringLiteral("F2"), QStringLiteral("F3")};
} // namespace

QRectF Tabs::rect(int index) const
{
    const float total = Theme::TabWidth * 3.0f + Theme::TabGap * 2.0f;
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + float(index) * (Theme::TabWidth + Theme::TabGap),
                  Theme::TabsTop, Theme::TabWidth, Theme::TabHeight);
}

void Tabs::draw(QPainter &painter, Mode mode, const Pointer &pointer,
                float pulse) const
{
    for (int tab = 0; tab < Count; ++tab) {
        const QRectF tabRect = rect(tab);
        const QColor tint = Theme::tabAccent(tab);
        const bool selected = int(mode) == tab;
        const bool hovered = pointer.hovering(tabRect);

        if (selected || hovered) {
            painter.save();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Base::withAlpha(tint, selected ? 55 : 30),
                                selected ? 3.0f : 2.0f));
            painter.drawRoundedRect(tabRect.adjusted(-1.0f, -1.0f, 1.0f, 1.0f),
                                    11.0f, 11.0f);
            painter.restore();
        }

        QLinearGradient fill(0.0f, float(tabRect.top()), 0.0f,
                             float(tabRect.bottom()));
        if (selected) {
            fill.setColorAt(0.0, QColor(12, 30, 46, 250));
            fill.setColorAt(1.0, QColor(6, 16, 28, 250));
        } else if (hovered) {
            fill.setColorAt(0.0, QColor(10, 23, 37, 242));
            fill.setColorAt(1.0, QColor(5, 13, 24, 242));
        } else {
            fill.setColorAt(0.0, QColor(7, 16, 28, 230));
            fill.setColorAt(1.0, QColor(4, 11, 21, 230));
        }
        painter.save();
        painter.setBrush(fill);
        painter.setPen(QPen(selected ? tint
                                    : (hovered ? Base::withAlpha(tint, 150)
                                               : QColor(42, 76, 96)),
                            selected ? 1.6f : 1.0f));
        painter.drawRoundedRect(tabRect, 10.0f, 10.0f);
        painter.restore();

        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(Base::withAlpha(tint, selected ? 210 : (hovered ? 95 : 38)));
        painter.drawRoundedRect(QRectF(tabRect.left() + 12.0f, tabRect.top() + 2.0f,
                                       tabRect.width() - 24.0f, 2.0f),
                                1.0f, 1.0f);
        painter.restore();

        const QRectF key(tabRect.left() + 14.0f,
                         float(tabRect.center().y()) - 11.0f, 30.0f, 22.0f);
        Theme::keycap(painter, key, kTabKeys[tab],
                      selected ? Base::withAlpha(tint, 220)
                               : (hovered ? Base::withAlpha(tint, 120)
                                          : QColor(44, 78, 98)),
                      selected ? Base::White : (hovered ? Base::Text : Base::Muted),
                      230, 5.0f);

        Base::text(painter, Theme::smallFont(), kTabLabels[tab],
                   selected ? Base::White : (hovered ? Base::Ink : Base::Text),
                   {float(tabRect.left()) + 56.0f,
                    float(tabRect.center().y()) - 6.0f},
                   QStringLiteral("midleft"), 238);
        Base::text(painter, Theme::techTinyFont(), kTabSubtitles[tab],
                   selected ? tint : (hovered ? Base::withAlpha(tint, 200)
                                              : Base::Muted),
                   {float(tabRect.left()) + 56.0f,
                    float(tabRect.center().y()) + 11.0f},
                   QStringLiteral("midleft"), selected ? 220 : 180);

        if (selected) {
            Base::fillCircle(painter,
                             {float(tabRect.right()) - 16.0f,
                              float(tabRect.center().y())},
                             2.6f,
                             Base::withAlpha(tint, int(120.0f + 120.0f * pulse)));
        }
    }
}

} // namespace menu::editor
