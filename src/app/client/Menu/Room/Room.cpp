#include "Menu/Room/Room.h"

#include "Base/Base.h"
#include "GameConfig.h"

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>
#include <cmath>

namespace menu {

QRectF Room::startRect() const
{
    return QRectF(286.0f, 552.0f, 250.0f, 48.0f);
}

QRectF Room::dissolveRect() const
{
    return QRectF(556.0f, 552.0f, 190.0f, 48.0f);
}

QRectF Room::backRect() const
{
    return QRectF(766.0f, 552.0f, 228.0f, 48.0f);
}

void Room::update(float dt, const Math::Vec2 &mouse, bool mouseActive)
{
    time_ += dt;
    pointer_ = mouse;
    pointerActive_ = mouseActive;
}

void Room::mouseMove(const Math::Vec2 &position)
{
    pointer_ = position;
    pointerActive_ = true;
}

bool Room::pointerHovering(const QRectF &rect) const
{
    return pointerActive_ && rect.contains(QPointF(pointer_.x, pointer_.y));
}

float Room::pulse() const
{
    return 0.5f + 0.5f * std::sin(time_ * 2.2f);
}

void Room::drawButton(QPainter &painter, const QRectF &rect, const QString &key,
                      const QString &label, const QColor &tint, bool primary,
                      bool enabled) const
{
    const bool hovered = enabled && pointerHovering(rect);
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
    painter.restore();

    const QFontMetricsF metrics(Base::techTinyFont());
    const float keyWidth = std::max(22.0f, float(metrics.horizontalAdvance(key)) + 12.0f);
    const QRectF badge(float(rect.left()) + 12.0f, float(rect.center().y()) - 10.0f,
                       keyWidth, 20.0f);
    painter.save();
    painter.setBrush(QColor(3, 10, 18, 235));
    painter.setPen(QPen(enabled ? Base::withAlpha(primary ? tint : QColor(96, 134, 154), 190)
                                : QColor(48, 62, 76),
                        1.0f));
    painter.drawRoundedRect(badge, 4.0f, 4.0f);
    painter.restore();
    Base::text(painter, Base::techTinyFont(), key,
               enabled ? (primary ? tint : Base::Text) : Base::Dim,
               {float(badge.center().x()), float(badge.center().y())},
               QStringLiteral("center"), 235);
    Base::text(painter, Base::techSmallFont(), label,
               enabled ? (primary ? Base::White : Base::Text) : Base::Dim,
               {float(badge.right()) + 9.0f, float(rect.center().y())},
               QStringLiteral("midleft"), 232);
}

void Room::render(QPainter &painter, const View &view) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 3, 9));

    QLinearGradient background(0, 0, 0, GameConfig::ScreenHeight);
    background.setColorAt(0.0, QColor(5, 15, 29));
    background.setColorAt(0.55, QColor(3, 9, 19));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     background);

    for (int i = 0; i < 110; ++i) {
        const float x = std::fmod(float(i) * 157.0f + 37.0f,
                                  float(GameConfig::ScreenWidth));
        const float y = std::fmod(float(i) * 83.0f + 19.0f,
                                  float(GameConfig::ScreenHeight));
        const float alpha = 14.0f + 26.0f * (0.5f + 0.5f * std::sin(time_ + i * 0.7f));
        Base::fillCircle(painter, {x, y}, 0.8f + float(i % 3) * 0.25f,
                         QColor(120, 200, 226, int(alpha)));
    }

    const QColor accent = view.host ? Base::Cyan : Base::Amber;
    Base::text(painter, Base::techTinyFont(),
               view.host ? QStringLiteral("ROOM HOST // 局域网大厅")
                         : QStringLiteral("ROOM JOIN // 已加入房间"),
               accent, {GameConfig::ScreenWidth / 2.0f, 72.0f},
               QStringLiteral("center"), 225);
    Base::glowText(painter, Base::bigFont(), view.roomName, Base::White,
                   {GameConfig::ScreenWidth / 2.0f, 122.0f}, QStringLiteral("center"));
    Base::text(painter, Base::smallFont(), view.status, Base::Muted,
               {GameConfig::ScreenWidth / 2.0f, 162.0f}, QStringLiteral("center"), 220);

    const QRectF card(250.0f, 202.0f, 780.0f, 340.0f);
    QLinearGradient cardFill(0.0f, float(card.top()), 0.0f, float(card.bottom()));
    cardFill.setColorAt(0.0, QColor(12, 26, 41, 245));
    cardFill.setColorAt(1.0, QColor(5, 12, 22, 245));
    painter.save();
    painter.setBrush(cardFill);
    painter.setPen(QPen(Base::withAlpha(accent, 190), 1.4f));
    painter.drawRoundedRect(card, 12.0f, 12.0f);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Base::withAlpha(accent, 190));
    painter.drawRoundedRect(QRectF(float(card.left()) + 18.0f,
                                   float(card.top()) + 1.0f,
                                   float(card.width()) - 36.0f, 2.0f),
                            1.0f, 1.0f);
    painter.restore();

    Base::text(painter, Base::techTinyFont(), QStringLiteral("ROOM ID  房间"),
               Base::Muted, {286.0f, 232.0f}, QStringLiteral("midleft"), 190);
    Base::text(painter, Base::techSmallFont(), view.roomName, Base::White,
               {286.0f, 252.0f}, QStringLiteral("midleft"), 235);
    Base::text(painter, Base::techTinyFont(), QStringLiteral("ENDPOINT  地址"),
               Base::Muted, {974.0f, 232.0f}, QStringLiteral("midright"), 190);
    Base::text(painter, Base::techSmallFont(),
               view.endpoint.isEmpty() ? QStringLiteral("等待服务端…") : view.endpoint,
               Base::withAlpha(accent, 235), {974.0f, 252.0f},
               QStringLiteral("midright"), 235);

    Base::strokeLine(painter, {286.0f, 274.0f}, {974.0f, 274.0f},
                     Base::withAlpha(accent, 70), 1.0f);

    const auto drawPlayer = [&](float y, const QString &role, const QString &detail,
                                const QString &badge, const QColor &tint,
                                bool occupied) {
        Base::fillCircle(painter, {302.0f, y}, 11.0f,
                         occupied ? Base::withAlpha(tint, 30) : QColor(24, 38, 52));
        Base::strokeCircle(painter, {302.0f, y}, 11.0f,
                           occupied ? Base::withAlpha(tint, 190) : QColor(52, 78, 96),
                           1.2f);
        if (occupied) {
            Base::fillCircle(painter, {302.0f, y}, 3.4f, tint);
        }
        Base::text(painter, Base::techSmallFont(), role,
                   occupied ? Base::White : Base::Dim,
                   {326.0f, y - 8.0f}, QStringLiteral("midleft"), 235);
        Base::text(painter, Base::tinyFont(), detail,
                   occupied ? Base::Muted : Base::Dim,
                   {326.0f, y + 10.0f}, QStringLiteral("midleft"), 200);
        if (!badge.isEmpty()) {
            const QRectF badgeRect(886.0f, y - 12.0f, 88.0f, 24.0f);
            Base::panel(painter, badgeRect, Base::withAlpha(tint, 190), 210, 6,
                        QColor(5, 14, 24));
            Base::text(painter, Base::techTinyFont(), badge, tint,
                       {float(badgeRect.center().x()), float(badgeRect.center().y())},
                       QStringLiteral("center"), 235);
        }
    };

    // 服务端 RoomList 的会话槽位：列表与人数始终对应，连接/断开会同步更新；
    // 空槽显示等待提示。
    const int slotCount = std::max(2, std::min(view.maxPlayers, 4));
    for (int i = 0; i < slotCount; ++i) {
        const float y = 306.0f + float(i) * 38.0f;
        if (i < int(view.playerSlots.size())) {
            const PlayerSlot &slot = view.playerSlots[size_t(i)];
            const QColor tint = slot.host ? (slot.you ? Base::Cyan : Base::Amber)
                                          : (slot.you ? Base::Green : Base::Cyan);
            QString badge;
            if (slot.you) {
                badge = QStringLiteral("YOU");
            } else if (slot.host) {
                badge = QStringLiteral("HOST");
            } else if (slot.inMatch) {
                badge = QStringLiteral("LIVE");
            }
            drawPlayer(y, slot.name, slot.detail, badge, tint, true);
        } else {
            drawPlayer(y, QStringLiteral("空槽位 / OPEN"),
                       QStringLiteral("等待其他玩家加入…"), QString(), Base::Cyan,
                       false);
        }
    }

    Base::strokeLine(painter, {286.0f, 470.0f}, {974.0f, 470.0f},
                     Base::withAlpha(accent, 55), 1.0f);

    Base::text(painter, Base::techTinyFont(), QStringLiteral("PLAYERS  玩家"),
               Base::Muted, {286.0f, 492.0f}, QStringLiteral("midleft"), 190);
    Base::text(painter, Base::techFont(),
               QStringLiteral("%1 / %2").arg(view.players).arg(view.maxPlayers),
               Base::withAlpha(accent, 240), {974.0f, 492.0f},
               QStringLiteral("midright"), 235);
    Base::text(painter, Base::tinyFont(), view.scriptSummary, Base::Text,
               {286.0f, 516.0f}, QStringLiteral("midleft"), 220);
    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("SERVER-AUTHORITATIVE  //  房间状态以主机为准"),
               Base::Muted, {974.0f, 516.0f}, QStringLiteral("midright"), 180);

    if (view.host) {
        drawButton(painter, startRect(), QStringLiteral("ENTER"),
                   QStringLiteral("开始对局"), Base::Cyan, true, view.canStart);
        drawButton(painter, dissolveRect(), QStringLiteral("X"),
                   QStringLiteral("解散房间"), Base::Red, false, true);
        drawButton(painter, backRect(), QStringLiteral("ESC"),
                   QStringLiteral("离开房间"), QColor(65, 104, 124), false, true);
    } else {
        drawButton(painter, backRect(), QStringLiteral("ESC"),
                   QStringLiteral("离开房间"), Base::Amber, false, true);
    }

    QString hint;
    if (!view.connected) {
        hint = QStringLiteral("正在连接主机，请稍候…");
    } else if (view.host) {
        hint = view.canStart
                   ? QStringLiteral("ENTER 开始对局  ·  X 解散房间  ·  ESC 离开房间")
                   : QStringLiteral("正在同步房间状态…  ·  X 解散房间  ·  ESC 离开房间");
    } else {
        hint = QStringLiteral("等待房主开始对局  ·  ESC 离开房间");
    }
    Base::text(painter, Base::tinyFont(), hint, QColor(140, 178, 196),
               {GameConfig::ScreenWidth / 2.0f, 642.0f},
               QStringLiteral("center"), 205);
    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("ROOM STATE  //  %1")
                   .arg(view.playing ? QStringLiteral("MATCH STARTING")
                                     : (view.connected ? QStringLiteral("WAITING")
                                                       : QStringLiteral("CONNECTING"))),
               Base::withAlpha(accent, 170),
               {GameConfig::ScreenWidth / 2.0f, 672.0f},
               QStringLiteral("center"), 170);
}

Room::Action Room::keyPress(int key, bool autoRepeat, const View &view) const
{
    if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
    }
    if (!view.host) {
        return Action::None;
    }
    if ((key == Qt::Key_Return || key == Qt::Key_Enter) && !autoRepeat) {
        return view.canStart ? Action::Start : Action::None;
    }
    if (key == Qt::Key_X && !autoRepeat) {
        return Action::Dissolve;
    }
    return Action::None;
}

Room::Action Room::mousePress(const Math::Vec2 &position, const View &view) const
{
    const QPointF point(position.x, position.y);
    if (view.host) {
        if (startRect().contains(point)) {
            return view.canStart ? Action::Start : Action::None;
        }
        if (dissolveRect().contains(point)) {
            return Action::Dissolve;
        }
    }
    if (backRect().contains(point)) {
        return Action::Back;
    }
    return Action::None;
}

} // namespace menu
