#include "Menu/Main/Main.h"

#include "Base/Base.h"
#include "GameConfig.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>
#include <cmath>

namespace menu {

void Main::update(float dt, const std::vector<Card> &cards,
                  const Math::Vec2 &mouse, bool mouseActive)
{
    time_ += dt;
    if (!mouseActive || cards.empty()) {
        return;
    }
    for (int i = 0; i < int(cards.size()); ++i) {
        if (levelRect(i, int(cards.size())).contains(QPointF(mouse.x, mouse.y))) {
            selected_ = i;
        }
    }
}

QRectF Main::levelRect(int index, int cardCount) const
{
    constexpr float width = 360.0f;
    constexpr float height = 156.0f;
    constexpr float gap = 26.0f;
    const int count = std::max(1, cardCount);
    const float total = width * float(count) + gap * float(count - 1);
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + index * (width + gap), 402.0f, width, height);
}

QRectF Main::settingsButtonRect() const
{
    return QRectF(GameConfig::ScreenWidth - 152.0f, 26.0f, 122.0f, 38.0f);
}

QRectF Main::shopButtonRect() const
{
    return QRectF(GameConfig::ScreenWidth - 286.0f, 26.0f, 122.0f, 38.0f);
}

QRectF Main::armoryButtonRect() const
{
    return QRectF(GameConfig::ScreenWidth - 420.0f, 26.0f, 122.0f, 38.0f);
}

void Main::render(QPainter &painter, const std::vector<Card> &cards,
                  const Math::Vec2 &mouse, bool mouseActive, bool muted,
                  bool audioUnavailable)
{
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 3, 9));

    QLinearGradient background(0, 0, 0, GameConfig::ScreenHeight);
    background.setColorAt(0.0, QColor(5, 15, 28));
    background.setColorAt(0.55, QColor(3, 8, 18));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     background);

    for (int i = 0; i < 120; ++i) {
        const float x = std::fmod(float(i) * 137.0f + 31.0f,
                                  float(GameConfig::ScreenWidth));
        const float y = std::fmod(float(i) * 83.0f + 17.0f,
                                  float(GameConfig::ScreenHeight));
        const float alpha = 35.0f + 45.0f * (0.5f + 0.5f * std::sin(time_ + i));
        Base::fillCircle(painter, {x, y}, i % 5 == 0 ? 1.8f : 1.0f,
                         Base::withAlpha(Base::Cyan, int(alpha * 0.55f)));
    }

    painter.save();
    painter.setPen(QPen(QColor(25, 56, 76), 1.0f));
    for (int x = 0; x < GameConfig::ScreenWidth; x += 80) {
        painter.drawLine(x, 0, x, GameConfig::ScreenHeight);
    }
    for (int y = 0; y < GameConfig::ScreenHeight; y += 80) {
        painter.drawLine(0, y, GameConfig::ScreenWidth, y);
    }
    painter.restore();

    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("SECTOR 07  //  DEEP SPACE DEFENSE GRID"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 102}, QStringLiteral("center"), 185);
    Base::glowText(painter, Base::titleFont(), QStringLiteral("星核协议"),
                   Base::White,
                   {GameConfig::ScreenWidth / 2.0f, 204}, QStringLiteral("center"));
    Base::text(painter, Base::techFont(),
               QStringLiteral("N E O N   C O R E   P R O T O C O L"),
               Base::Cyan,
               {GameConfig::ScreenWidth / 2.0f, 278}, QStringLiteral("center"), 230);
    Base::text(painter, Base::bodyFont(),
               QStringLiteral("选择行动。每一次部署，都会改写星核的命运。"),
               Base::Text,
               {GameConfig::ScreenWidth / 2.0f, 335}, QStringLiteral("center"), 220);

    Base::text(painter, Base::techSmallFont(), QStringLiteral("SELECT OPERATION"),
               Base::Muted, {GameConfig::ScreenWidth / 2.0f, 375},
               QStringLiteral("center"), 190);
    const float pulse = 0.5f + 0.5f * std::sin(time_ * 2.6f);
    const int count = int(cards.size());
    for (int i = 0; i < count; ++i) {
        const Card &card = cards[size_t(i)];
        const QRectF rect = levelRect(i, count);
        const bool active = selected_ == i;
        const QColor border = active ? Base::Cyan : QColor(48, 91, 112);
        Base::panel(painter, rect, border, active ? 235 : 190, 12,
                    active ? QColor(8, 22, 36) : QColor(6, 13, 24));
        if (active) {
            painter.save();
            painter.setBrush(Qt::NoBrush);
            QPen pen(Base::withAlpha(Base::Cyan, int(45 + 45 * pulse)));
            pen.setWidthF(3.0f);
            painter.setPen(pen);
            painter.drawRoundedRect(rect.adjusted(-7, -7, 7, 7), 16, 16);
            painter.restore();
        }
        Base::text(painter, Base::techTinyFont(),
                   QStringLiteral("0%1").arg(i + 1), border,
                   {float(rect.left()) + 24.0f, float(rect.top()) + 24.0f},
                   QStringLiteral("topleft"), 210);
        Base::text(painter, Base::mediumFont(), card.name,
                   active ? Base::White : QColor(191, 216, 230),
                   {float(rect.left()) + 24.0f, float(rect.top()) + 54.0f},
                   QStringLiteral("topleft"));
        Base::text(painter, Base::smallFont(), card.subtitle, Base::Muted,
                   {float(rect.left()) + 24.0f, float(rect.top()) + 88.0f},
                   QStringLiteral("topleft"), 215);
        Base::text(painter, Base::techTinyFont(),
                   card.custom ? QStringLiteral("SCRIPT EDITOR")
                               : QStringLiteral("CAMPAIGN"),
                   Base::shade(border, 0.85f),
                   {float(rect.left()) + 24.0f, float(rect.bottom()) - 20.0f},
                   QStringLiteral("midleft"), 190);
        const QString action = card.custom
                                   ? (active ? QStringLiteral("ENTER  EDIT")
                                             : QStringLiteral("CLICK  EDIT"))
                                   : (active ? QStringLiteral("ENTER  MAP")
                                             : QStringLiteral("CLICK  MAP"));
        Base::text(painter, Base::techSmallFont(), action,
                   active ? Base::Cyan : Base::Muted,
                   {float(rect.right()) - 24.0f, float(rect.bottom()) - 22.0f},
                   QStringLiteral("midright"), 210);
    }

    const QRectF armoryButton = armoryButtonRect();
    const bool armoryHover = mouseActive
                             && armoryButton.contains(QPointF(mouse.x, mouse.y));
    Base::panel(painter, armoryButton,
                armoryHover ? Base::Cyan : QColor(48, 91, 112),
                armoryHover ? 235 : 195, 10, QColor(6, 14, 25));
    {
        const Math::Vec2 center(float(armoryButton.left()) + 26.0f,
                                float(armoryButton.center().y()));
        const QColor icon = armoryHover ? Base::Cyan : QColor(150, 200, 220);
        Base::strokeCircle(painter, center, 8.0f, icon, 1.5f);
        Base::strokeLine(painter, {center.x - 12.0f, center.y},
                         {center.x - 4.0f, center.y}, icon, 1.5f);
        Base::strokeLine(painter, {center.x + 4.0f, center.y},
                         {center.x + 12.0f, center.y}, icon, 1.5f);
        Base::strokeLine(painter, {center.x, center.y - 12.0f},
                         {center.x, center.y - 4.0f}, icon, 1.5f);
        Base::strokeLine(painter, {center.x, center.y + 4.0f},
                         {center.x, center.y + 12.0f}, icon, 1.5f);
        Base::fillCircle(painter, center, 2.0f, icon);
    }
    Base::text(painter, Base::smallFont(), QStringLiteral("战备"),
               armoryHover ? Base::White : QColor(191, 216, 230),
               {float(armoryButton.left()) + 46.0f,
                float(armoryButton.center().y())},
               QStringLiteral("midleft"), 220);

    const QRectF shopButton = shopButtonRect();
    const bool shopHover = mouseActive
                           && shopButton.contains(QPointF(mouse.x, mouse.y));
    Base::panel(painter, shopButton,
                shopHover ? Base::Amber : QColor(48, 91, 112),
                shopHover ? 235 : 195, 10, QColor(6, 14, 25));
    Base::crate(painter,
                {float(shopButton.left()) + 26.0f,
                 float(shopButton.center().y())},
                9.0f, shopHover ? Base::Amber : QColor(196, 178, 140));
    Base::text(painter, Base::smallFont(), QStringLiteral("商店"),
               shopHover ? Base::White : QColor(191, 216, 230),
               {float(shopButton.left()) + 46.0f,
                float(shopButton.center().y())},
               QStringLiteral("midleft"), 220);

    const QRectF settingsButton = settingsButtonRect();
    const bool settingsHover = mouseActive
                               && settingsButton.contains(
                                   QPointF(mouse.x, mouse.y));
    Base::panel(painter, settingsButton,
                settingsHover ? Base::Cyan : QColor(48, 91, 112),
                settingsHover ? 235 : 195, 10, QColor(6, 14, 25));
    Base::gear(painter,
               {float(settingsButton.left()) + 26.0f,
                float(settingsButton.center().y())},
               9.0f, settingsHover ? Base::Cyan : QColor(150, 190, 205));
    Base::text(painter, Base::smallFont(), QStringLiteral("设置"),
               settingsHover ? Base::White : QColor(191, 216, 230),
               {float(settingsButton.left()) + 46.0f,
                float(settingsButton.center().y())},
               QStringLiteral("midleft"), 220);

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("WASD 移动    LMB / J 射击    SPACE 冲刺"),
               QColor(105, 138, 158),
               {34, GameConfig::ScreenHeight - 34.0f}, QStringLiteral("midleft"), 150);
    if (audioUnavailable) {
        Base::text(painter, Base::tinyFont(), QStringLiteral("音频设备不可用"),
                   Base::Amber,
                   {34, GameConfig::ScreenHeight - 58.0f}, QStringLiteral("midleft"), 210);
    }
    const QString muteHint = muted ? QStringLiteral("M 已静音")
                                   : QStringLiteral("M 静音");
    const bool customSelected = selected_ >= 0 && selected_ < count
                                && cards[size_t(selected_)].custom;
    Base::text(painter, Base::tinyFont(),
               customSelected
                   ? QStringLiteral("↑ / ↓ 选择    ENTER 编辑台本    ") + muteHint
                   : QStringLiteral("↑ / ↓ 选择    ENTER 开始    ") + muteHint,
               muted ? Base::Amber : QColor(105, 138, 158),
               {GameConfig::ScreenWidth - 34.0f, GameConfig::ScreenHeight - 34.0f},
               QStringLiteral("midright"), 150);
}

Main::Command Main::keyPress(int key, const std::vector<Card> &cards)
{
    Command command;
    const int count = int(cards.size());
    if (count == 0) {
        return command;
    }
    if (key == Qt::Key_Up || key == Qt::Key_W || key == Qt::Key_Left) {
        selected_ = std::max(0, selected_ - 1);
    } else if (key == Qt::Key_Down || key == Qt::Key_S || key == Qt::Key_Right) {
        selected_ = std::min(count - 1, selected_ + 1);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        if (cards[size_t(selected_)].custom) {
            command.action = Action::OpenEditor;
        } else {
            command.action = Action::OpenMap;
        }
    } else if (key == Qt::Key_1) {
        command.action = Action::OpenMap;
    } else if (key == Qt::Key_2) {
        command.action = Action::OpenEditor;
    } else if (key == Qt::Key_3) {
        command.action = Action::OpenArmory;
    }
    return command;
}

Main::Command Main::mousePress(const Math::Vec2 &position,
                               const std::vector<Card> &cards)
{
    Command command;
    const QPointF point(position.x, position.y);
    if (armoryButtonRect().contains(point)) {
        command.action = Action::OpenArmory;
        return command;
    }
    if (shopButtonRect().contains(point)) {
        command.action = Action::OpenShop;
        return command;
    }
    if (settingsButtonRect().contains(point)) {
        command.action = Action::OpenSettings;
        return command;
    }
    const int count = int(cards.size());
    for (int i = 0; i < count; ++i) {
        if (!levelRect(i, count).contains(point)) {
            continue;
        }
        selected_ = i;
        command.level = i;
        command.action = cards[size_t(i)].custom ? Action::OpenEditor
                                                 : Action::OpenMap;
        return command;
    }
    return command;
}

} // namespace menu
