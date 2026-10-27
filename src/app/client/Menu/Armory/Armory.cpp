#include "Menu/Armory/Armory.h"

#include "Base/Base.h"
#include "Data/GameData.h"
#include "GameConfig.h"
#include "Loadout/Loadout.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>

namespace menu {

namespace {

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), int(text.size()));
}

} // namespace

void Armory::open()
{
    selected_ = LoadoutService.selected();
}

QRectF Armory::cardRect(int index) const
{
    constexpr float width = 290.0f;
    constexpr float gap = 28.0f;
    const float total = width * float(data::kLaunchers.count)
                        + gap * float(std::max(0, data::kLaunchers.count - 1));
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + float(index) * (width + gap), 214.0f, width, 314.0f);
}

QRectF Armory::backRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 90.0f, 600.0f, 180.0f, 42.0f);
}

Armory::Action Armory::keyPress(int key)
{
    if (data::kLaunchers.count <= 0) {
        return key == Qt::Key_Escape || key == Qt::Key_Q ? Action::Back
                                                         : Action::None;
    }
    if (key == Qt::Key_Left || key == Qt::Key_A) {
        selected_ = std::max(0, selected_ - 1);
    } else if (key == Qt::Key_Right || key == Qt::Key_D) {
        selected_ = std::min(data::kLaunchers.count - 1, selected_ + 1);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter
               || key == Qt::Key_Space) {
        LoadoutService.select(selected_);
        BaseService.playSound(QStringLiteral("upgrade"));
    } else if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
    }
    return Action::None;
}

Armory::Action Armory::mousePress(const Math::Vec2 &position)
{
    const QPointF point(position.x, position.y);
    if (backRect().contains(point)) {
        return Action::Back;
    }
    for (int i = 0; i < data::kLaunchers.count; ++i) {
        if (cardRect(i).contains(point)) {
            selected_ = i;
            LoadoutService.select(i);
            BaseService.playSound(QStringLiteral("upgrade"));
            break;
        }
    }
    return Action::None;
}

void Armory::render(QPainter &painter) const
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
               QStringLiteral("WAR PREP  //  LAUNCHER LOADOUT"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 78}, QStringLiteral("center"), 190);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("战备"),
                   Base::White, {GameConfig::ScreenWidth / 2.0f, 128},
                   QStringLiteral("center"));
    Base::text(painter, Base::smallFont(),
               QStringLiteral("选择发射器，默认标准弹仍然保留"),
               Base::Muted, {GameConfig::ScreenWidth / 2.0f, 168},
               QStringLiteral("center"), 210);

    for (int i = 0; i < data::kLaunchers.count; ++i) {
        const data::LauncherDef &def = data::kLaunchers.items[size_t(i)];
        const QRectF card = cardRect(i);
        const bool selected = i == selected_;
        const bool equipped = i == LoadoutService.selected();
        const QColor accent = i == 1 ? Base::Amber
                                     : (i == 2 ? Base::Violet : Base::Cyan);
        Base::panel(painter, card,
                    selected ? accent : QColor(48, 91, 112),
                    selected ? 240 : 190, 12,
                    selected ? QColor(9, 24, 38) : QColor(6, 14, 25));

        Base::text(painter, Base::techTinyFont(),
                   equipped ? QStringLiteral("EQUIPPED") : QStringLiteral("LAUNCHER"),
                   equipped ? Base::Cyan : Base::Muted,
                   {float(card.left()) + 18.0f, float(card.top()) + 24.0f},
                   QStringLiteral("midleft"), 200);
        Base::text(painter, Base::mediumFont(), toQString(def.name),
                   Base::White,
                   {float(card.left()) + 18.0f, float(card.top()) + 66.0f},
                   QStringLiteral("midleft"), 235);
        Base::text(painter, Base::tinyFont(), toQString(def.subtitle),
                   Base::Muted,
                   {float(card.left()) + 18.0f, float(card.top()) + 94.0f},
                   QStringLiteral("midleft"), 190);

        // 弹道示意
        const float centerX = float(card.center().x());
        const float centerY = float(card.top()) + 150.0f;
        Base::fillCircle(painter, {centerX, centerY}, 5.0f, accent);
        for (int shot = 0; shot < std::max(1, 1 + def.extraShots); ++shot) {
            const float offset = (float(shot) - float(def.extraShots) / 2.0f)
                                 * 16.0f;
            Base::fillCircle(painter, {centerX + offset, centerY - 44.0f},
                             2.4f + def.radius * 0.35f, accent);
            Base::strokeLine(painter, {centerX + offset, centerY - 22.0f},
                             {centerX + offset, centerY - 8.0f},
                             Base::withAlpha(accent, 100), 1.0f);
        }

        const struct {
            const char *label;
            QString value;
        } rows[] = {
            {"DAMAGE", QStringLiteral("x%1").arg(def.damageScale, 0, 'f', 2)},
            {"RATE", QStringLiteral("x%1").arg(1.0f / std::max(0.01f, def.fireCdScale),
                                               0, 'f', 2)},
            {"SHOTS", QStringLiteral("+%1").arg(def.extraShots)},
            {"PIERCE", QStringLiteral("+%1").arg(def.pierceBonus)},
        };
        for (int row = 0; row < 4; ++row) {
            const float y = float(card.top()) + 204.0f + float(row) * 22.0f;
            Base::text(painter, Base::techTinyFont(), QString::fromLatin1(rows[row].label),
                       Base::Muted, {float(card.left()) + 18.0f, y},
                       QStringLiteral("midleft"), 190);
            Base::text(painter, Base::techSmallFont(), rows[row].value,
                       Base::Text, {float(card.right()) - 18.0f, y},
                       QStringLiteral("midright"), 220);
        }
        Base::text(painter, Base::tinyFont(),
                   equipped ? QStringLiteral("已装备") : QStringLiteral("ENTER / 点击装备"),
                   equipped ? Base::Cyan : Base::Muted,
                   {centerX, float(card.bottom()) - 22.0f},
                   QStringLiteral("center"), 200);
    }

    Base::panel(painter, backRect(), QColor(65, 104, 124), 210, 9,
                QColor(7, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回"), Base::Text,
               {float(backRect().center().x()), float(backRect().center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::tinyFont(),
               QStringLiteral("← → 选择    ENTER 装备    ESC 返回"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 28.0f},
               QStringLiteral("center"), 160);
}

} // namespace menu
