#include "Menu/Editor/JoinPanel.h"

#include "Base/Base.h"
#include "Menu/Editor/Theme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

namespace menu::editor {

QRectF JoinPanel::cardRect()
{
    return QRectF(280.0f, 236.0f, 720.0f, 300.0f);
}

QRectF JoinPanel::hostRect()
{
    return QRectF(314.0f, 336.0f, 440.0f, 44.0f);
}

QRectF JoinPanel::portRect()
{
    return QRectF(774.0f, 336.0f, 190.0f, 44.0f);
}

void JoinPanel::draw(QPainter &painter, const QString &hostText,
                     const QString &portText, bool hostFocused, bool portFocused,
                     bool hostValid, bool portValid, const Pointer &pointer,
                     float pulse) const
{
    const QColor tint = Base::Amber;
    const QRectF card = cardRect();

    QLinearGradient fill(0.0f, float(card.top()), 0.0f, float(card.bottom()));
    fill.setColorAt(0.0, QColor(12, 26, 40, 245));
    fill.setColorAt(1.0, QColor(5, 12, 22, 245));
    painter.save();
    painter.setBrush(fill);
    painter.setPen(QPen(Base::withAlpha(tint, 200), 1.4f));
    painter.drawRoundedRect(card, 12.0f, 12.0f);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Base::withAlpha(tint, 190));
    painter.drawRoundedRect(QRectF(float(card.left()) + 18.0f,
                                   float(card.top()) + 1.0f,
                                   float(card.width()) - 36.0f, 2.0f),
                            1.0f, 1.0f);
    painter.restore();

    // 右上角的“主机 ← 客户端”示意
    const Math::Vec2 server(936.0f, 284.0f);
    const Math::Vec2 client(972.0f, 309.0f);
    Base::strokeLine(painter, server, client, Base::withAlpha(tint, 125), 1.2f);
    Base::strokeCircle(painter, server, 15.0f, Base::withAlpha(tint, 115), 1.2f);
    Base::strokeCircle(painter, client, 10.0f, Base::withAlpha(tint, 115), 1.2f);
    Base::strokeCircle(painter, client, 13.0f + 4.0f * pulse,
                       Base::withAlpha(tint, int(70.0f * (1.0f - pulse))), 1.0f);
    Base::fillCircle(painter, server, 3.0f, tint);
    Base::fillCircle(painter, client, 2.4f, Base::Cyan);

    Base::text(painter, Theme::techTinyFont(),
               QStringLiteral("JOIN HOST // 连接远端"), tint, {314.0f, 260.0f},
               QStringLiteral("midleft"), 220);
    Base::text(painter, Theme::mediumFont(), QStringLiteral("加入已有主机"),
               Base::White, {314.0f, 286.0f}, QStringLiteral("midleft"), 240);
    Base::text(painter, Theme::tinyFont(),
               QStringLiteral("连接后由主机推进台本；本机只发送输入并渲染快照"),
               Base::Muted, {314.0f, 310.0f}, QStringLiteral("midleft"), 210);

    Base::text(painter, Theme::techTinyFont(),
               QStringLiteral("HOST ADDRESS  主机地址"), Base::Muted,
               {314.0f, 323.0f}, QStringLiteral("midleft"), 190);
    Base::text(painter, Theme::techTinyFont(), QStringLiteral("PORT  端口"),
               Base::Muted, {774.0f, 323.0f}, QStringLiteral("midleft"), 190);

    Theme::textField(painter, hostRect(), hostText,
                     QStringLiteral("输入主机地址"), hostFocused, hostValid, tint,
                     pointer, pulse);
    Theme::textField(painter, portRect(), portText, QStringLiteral("端口"),
                     portFocused, portValid, tint, pointer, pulse);

    const bool valid = hostValid && portValid;
    const float statusY = 401.0f;
    Base::fillCircle(painter, {320.0f, statusY}, 3.0f,
                     valid ? Base::Green : Base::Amber);
    Base::text(painter, Theme::techSmallFont(),
               valid ? QStringLiteral("准备连接  %1 : %2").arg(hostText).arg(portText)
                     : QStringLiteral("地址或端口无效：请填写主机地址与 1–65535 端口"),
               valid ? Base::Green : Base::Amber, {330.0f, statusY},
               QStringLiteral("midleft"), 225);
    Base::text(painter, Theme::techTinyFont(),
               valid ? QStringLiteral("ENTER 连接") : QStringLiteral("等待输入"),
               Base::Muted, {966.0f, statusY}, QStringLiteral("midright"), 190);

    Base::strokeLine(painter, {314.0f, 430.0f}, {966.0f, 430.0f},
                     Base::withAlpha(tint, 55), 1.0f);
    Base::text(painter, Theme::tinyFont(),
               QStringLiteral("同一局域网内填写主机 IP；端口范围 1–65535"),
               Base::Muted, {314.0f, 452.0f}, QStringLiteral("midleft"), 200);
    Base::text(painter, Theme::tinyFont(),
               QStringLiteral("BACKSPACE 删除字符  ·  点击字段切换输入  ·  TAB / F1–F3 切换选项卡"),
               QColor(105, 138, 158), {314.0f, 476.0f},
               QStringLiteral("midleft"), 180);
}

} // namespace menu::editor
