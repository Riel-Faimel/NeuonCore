#include "Menu/Map/Map.h"

#include "Base/Base.h"
#include "Data/GameData.h"
#include "GameConfig.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>
#include <cmath>

namespace menu {

namespace {

constexpr float MapLeft = 150.0f;
constexpr float MapTop = 250.0f;
constexpr float MapWidth = 980.0f;
constexpr float MapHeight = 290.0f;
constexpr float NodeRadius = 30.0f;

QPointF nodeCenter(int index)
{
    const data::LevelNode &node = data::kLevelNodes.items[size_t(index)];
    return QPointF(MapLeft + node.x * MapWidth, MapTop + node.y * MapHeight);
}

QString toText(std::string_view text)
{
    return QString::fromUtf8(text.data(), int(text.size()));
}

} // namespace

void Map::reset()
{
    selected_ = 0;
    time_ = 0.0f;
    denied_ = 0.0f;
}

void Map::update(float dt, const Math::Vec2 &mouse, bool mouseActive)
{
    time_ += dt;
    denied_ = std::max(0.0f, denied_ - dt);
    if (!mouseActive) {
        return;
    }
    for (int i = 0; i < data::kLevelNodes.count; ++i) {
        if (nodeRect(i).contains(QPointF(mouse.x, mouse.y))) {
            selected_ = i;
        }
    }
}

QRectF Map::nodeRect(int index) const
{
    const QPointF center = nodeCenter(index);
    constexpr float half = NodeRadius + 12.0f;
    return QRectF(center.x() - half, center.y() - half, half * 2.0f, half * 2.0f);
}

QRectF Map::backRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 90.0f, 604.0f, 180.0f, 40.0f);
}

Map::Command Map::keyPress(int key)
{
    Command command;
    const int count = data::kLevelNodes.count;
    if (count == 0) {
        return command;
    }
    if (key == Qt::Key_Left || key == Qt::Key_A || key == Qt::Key_Up || key == Qt::Key_W) {
        selected_ = std::max(0, selected_ - 1);
    } else if (key == Qt::Key_Right || key == Qt::Key_D
               || key == Qt::Key_Down || key == Qt::Key_S) {
        selected_ = std::min(count - 1, selected_ + 1);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
        const data::LevelNode &node = data::kLevelNodes.items[size_t(selected_)];
        if (node.locked) {
            denied_ = 1.4f;
        } else {
            command.action = Action::Start;
            command.level = selected_;
        }
    } else if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        command.action = Action::Back;
    }
    return command;
}

Map::Command Map::mousePress(const Math::Vec2 &position)
{
    Command command;
    const QPointF point(position.x, position.y);
    if (backRect().contains(point)) {
        command.action = Action::Back;
        return command;
    }
    for (int i = 0; i < data::kLevelNodes.count; ++i) {
        if (!nodeRect(i).contains(point)) {
            continue;
        }
        selected_ = i;
        const data::LevelNode &node = data::kLevelNodes.items[size_t(i)];
        if (node.locked) {
            denied_ = 1.4f;
        } else {
            command.action = Action::Start;
            command.level = i;
        }
        return command;
    }
    return command;
}

void Map::render(QPainter &painter) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 3, 9));

    QLinearGradient background(0, 0, 0, GameConfig::ScreenHeight);
    background.setColorAt(0.0, QColor(4, 13, 26));
    background.setColorAt(0.6, QColor(3, 8, 18));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     background);

    for (int i = 0; i < 90; ++i) {
        const float x = std::fmod(float(i) * 173.0f + 41.0f,
                                  float(GameConfig::ScreenWidth));
        const float y = std::fmod(float(i) * 97.0f + 23.0f,
                                  float(GameConfig::ScreenHeight));
        const float alpha = 22.0f + 30.0f * (0.5f + 0.5f * std::sin(time_ + i));
        Base::fillCircle(painter, {x, y}, i % 6 == 0 ? 1.6f : 0.9f,
                         Base::withAlpha(Base::Cyan, int(alpha * 0.5f)));
    }

    painter.save();
    painter.setPen(QPen(QColor(20, 46, 64), 1.0f));
    for (int x = 0; x < GameConfig::ScreenWidth; x += 80) {
        painter.drawLine(x, 0, x, GameConfig::ScreenHeight);
    }
    for (int y = 0; y < GameConfig::ScreenHeight; y += 80) {
        painter.drawLine(0, y, GameConfig::ScreenWidth, y);
    }
    painter.restore();

    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("SECTOR MAP  //  STANDARD PROTOCOL"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 124}, QStringLiteral("center"), 190);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("星区地图"),
                   Base::White, {GameConfig::ScreenWidth / 2.0f, 172},
                   QStringLiteral("center"));
    Base::text(painter, Base::smallFont(),
               QStringLiteral("选择作战区域，ENTER 进入。未解锁区域会随进度开放。"),
               Base::Muted,
               {GameConfig::ScreenWidth / 2.0f, 208}, QStringLiteral("center"), 200);

    // 航路
    for (int i = 0; i + 1 < data::kLevelNodes.count; ++i) {
        const QPointF from = nodeCenter(i);
        const QPointF to = nodeCenter(i + 1);
        QPen pen(QColor(41, 74, 94), 1.4f);
        pen.setStyle(Qt::DashLine);
        painter.save();
        painter.setPen(pen);
        painter.drawLine(from, to);
        painter.restore();
    }

    for (int i = 0; i < data::kLevelNodes.count; ++i) {
        const data::LevelNode &node = data::kLevelNodes.items[size_t(i)];
        const QPointF center = nodeCenter(i);
        const bool selected = i == selected_;
        const bool unlocked = !node.locked;
        const float pulse = 0.5f + 0.5f * std::sin(time_ * 2.4f + float(i));

        if (unlocked) {
            Base::fillCircle(painter,
                             {float(center.x()), float(center.y())},
                             NodeRadius + 10.0f + pulse * 3.0f,
                             Base::withAlpha(Base::Cyan, 18));
            Base::strokeCircle(painter,
                               {float(center.x()), float(center.y())},
                               NodeRadius + 6.0f, Base::withAlpha(Base::Cyan, 90), 1.0f);
            Base::fillCircle(painter,
                             {float(center.x()), float(center.y())},
                             NodeRadius, QColor(8, 22, 34));
            Base::strokeCircle(painter,
                               {float(center.x()), float(center.y())},
                               NodeRadius, Base::Cyan, 2.0f);
            Base::text(painter, Base::techFont(),
                       QStringLiteral("%1").arg(i + 1, 2, 10, QChar('0')),
                       Base::White, {float(center.x()), float(center.y())},
                       QStringLiteral("center"));
        } else {
            painter.save();
            QPen pen(QColor(74, 98, 112));
            pen.setWidthF(1.6f);
            pen.setStyle(Qt::DashLine);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(center, NodeRadius, NodeRadius);
            painter.restore();
            Base::text(painter, Base::techTinyFont(), QStringLiteral("LOCK"),
                       QColor(104, 128, 142),
                       {float(center.x()), float(center.y())},
                       QStringLiteral("center"), 190);
        }

        if (selected) {
            Base::strokeCircle(painter,
                               {float(center.x()), float(center.y())},
                               NodeRadius + 14.0f,
                               unlocked ? Base::withAlpha(Base::Cyan, 200)
                                        : QColor(120, 140, 152, 160),
                               1.5f);
            Base::text(painter, Base::techTinyFont(),
                       unlocked ? QStringLiteral("DEPLOY") : QStringLiteral("LOCKED"),
                       unlocked ? Base::Cyan : Base::Muted,
                       {float(center.x()), float(center.y()) - NodeRadius - 24.0f},
                       QStringLiteral("center"), 220);
        }

        Base::text(painter, Base::smallFont(), toText(node.name),
                   unlocked ? Base::White : QColor(126, 150, 164),
                   {float(center.x()), float(center.y()) + NodeRadius + 22.0f},
                   QStringLiteral("center"), 220);
        Base::text(painter, Base::tinyFont(), toText(node.subtitle),
                   unlocked ? Base::Muted : QColor(88, 108, 122),
                   {float(center.x()), float(center.y()) + NodeRadius + 42.0f},
                   QStringLiteral("center"), 180);
    }

    if (denied_ > 0.0f) {
        const int alpha = int(180.0f + 60.0f * std::sin(time_ * 10.0f));
        Base::text(painter, Base::smallFont(), QStringLiteral("该区域尚未解锁"),
                   Base::withAlpha(Base::Amber, alpha),
                   {GameConfig::ScreenWidth / 2.0f, 566.0f},
                   QStringLiteral("center"), 230);
    }

    Base::panel(painter, backRect(), QColor(65, 104, 124), 210, 9, QColor(7, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回"), Base::Text,
               {float(backRect().center().x()), float(backRect().center().y())},
               QStringLiteral("center"));

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("← → 选择区域    ENTER 进入    ESC 返回"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 34.0f},
               QStringLiteral("center"), 160);
}

} // namespace menu
