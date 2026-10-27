#include "Menu/Settings/Settings.h"

#include "Base/Base.h"
#include "Data/Settings.h"
#include "GameConfig.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>
#include <array>
#include <cmath>

namespace menu {

Settings::Settings(bool persistent) : persistent_(persistent) {}

void Settings::reset()
{
    row_ = 0;
    dragging_ = false;
}

QRectF Settings::rowRect(int row) const
{
    return QRectF(300.0f, 286.0f + float(row) * 100.0f, 680.0f, 80.0f);
}

QRectF Settings::trackRect(int row) const
{
    const QRectF rect = rowRect(row);
    return QRectF(566.0f, float(rect.center().y()) - 8.0f, 356.0f, 16.0f);
}

QRectF Settings::backRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 90.0f, 600.0f, 180.0f, 42.0f);
}

void Settings::load()
{
    if (!persistent_) {
        return;
    }
    QSettings settings = app::openSettings();
    const int sound = settings.value(QStringLiteral("audio/sound"), 68).toInt();
    const int music = settings.value(QStringLiteral("audio/music"), 45).toInt();
    BaseService.setSoundVolume(float(std::clamp(sound, 0, 100)) / 100.0f);
    BaseService.setMusicVolume(float(std::clamp(music, 0, 100)) / 100.0f);
}

void Settings::save() const
{
    if (!persistent_) {
        return;
    }
    QSettings settings = app::openSettings();
    settings.setValue(QStringLiteral("audio/sound"),
                      int(std::lround(BaseService.soundVolume() * 100.0f)));
    settings.setValue(QStringLiteral("audio/music"),
                      int(std::lround(BaseService.musicVolume() * 100.0f)));
}

void Settings::adjust(int row, float delta)
{
    if (row == 0) {
        BaseService.setSoundVolume(std::clamp(BaseService.soundVolume() + delta, 0.0f, 1.0f));
        BaseService.playSound(QStringLiteral("ui"));
    } else {
        BaseService.setMusicVolume(std::clamp(BaseService.musicVolume() + delta, 0.0f, 1.0f));
    }
    save();
}

void Settings::setFromPointer(const Math::Vec2 &position)
{
    const QRectF track = trackRect(row_);
    if (track.width() <= 0.0f) {
        return;
    }
    const float ratio = std::clamp(
        (position.x - float(track.left())) / float(track.width()), 0.0f, 1.0f);
    if (row_ == 0) {
        BaseService.setSoundVolume(ratio);
    } else {
        BaseService.setMusicVolume(ratio);
    }
    save();
}

Settings::Action Settings::keyPress(int key)
{
    if (key == Qt::Key_Up || key == Qt::Key_W) {
        row_ = std::max(0, row_ - 1);
    } else if (key == Qt::Key_Down || key == Qt::Key_S) {
        row_ = std::min(1, row_ + 1);
    } else if (key == Qt::Key_Left || key == Qt::Key_A) {
        adjust(row_, -0.05f);
    } else if (key == Qt::Key_Right || key == Qt::Key_D) {
        adjust(row_, 0.05f);
    } else if (key == Qt::Key_Escape || key == Qt::Key_Q
               || key == Qt::Key_Return || key == Qt::Key_Enter) {
        return Action::Back;
    }
    return Action::None;
}

Settings::Action Settings::mousePress(const Math::Vec2 &position)
{
    const QPointF point(position.x, position.y);
    if (backRect().contains(point)) {
        return Action::Back;
    }
    for (int row = 0; row < 2; ++row) {
        if (!rowRect(row).contains(point)) {
            continue;
        }
        row_ = row;
        if (trackRect(row).adjusted(-8.0f, -14.0f, 8.0f, 14.0f).contains(point)) {
            setFromPointer(position);
            dragging_ = true;
            if (row == 0) {
                BaseService.playSound(QStringLiteral("ui"));
            }
        }
        return Action::None;
    }
    return Action::None;
}

void Settings::mouseMove(const Math::Vec2 &position)
{
    if (dragging_) {
        setFromPointer(position);
    }
}

void Settings::mouseRelease()
{
    dragging_ = false;
}

void Settings::render(QPainter &painter) const
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
               QStringLiteral("AUDIO CONFIGURATION  //  SETTINGS"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 132}, QStringLiteral("center"), 190);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("设置"),
                   Base::White, {GameConfig::ScreenWidth / 2.0f, 186},
                   QStringLiteral("center"));
    Base::gear(painter, {GameConfig::ScreenWidth / 2.0f - 96.0f, 186.0f}, 17.0f,
               Base::Cyan);
    Base::text(painter, Base::smallFont(),
               QStringLiteral("音量调整会立即生效并保存"), Base::Muted,
               {GameConfig::ScreenWidth / 2.0f, 226}, QStringLiteral("center"), 210);

    const std::array<QString, 2> titles = {
        QStringLiteral("音效音量  //  SFX"),
        QStringLiteral("背景音乐  //  BGM")};
    const std::array<QString, 2> notes = {
        QStringLiteral("射击、命中、界面提示"),
        QStringLiteral("music 目录轮转播放")};
    for (int row = 0; row < 2; ++row) {
        const QRectF rect = rowRect(row);
        const bool active = row == row_;
        Base::panel(painter, rect, active ? Base::Cyan : QColor(48, 91, 112),
                    active ? 235 : 190, 12,
                    active ? QColor(8, 22, 36) : QColor(6, 13, 24));
        Base::text(painter, Base::techSmallFont(), titles[size_t(row)],
                   active ? Base::White : QColor(191, 216, 230),
                   {float(rect.left()) + 28.0f, float(rect.center().y()) - 10.0f},
                   QStringLiteral("midleft"), 225);
        Base::text(painter, Base::tinyFont(), notes[size_t(row)], Base::Muted,
                   {float(rect.left()) + 28.0f, float(rect.center().y()) + 16.0f},
                   QStringLiteral("midleft"), 190);

        const float value = row == 0 ? BaseService.soundVolume() : BaseService.musicVolume();
        const QRectF track = trackRect(row);
        Base::bar(painter, track, value, active ? Base::Cyan : Base::CyanSoft,
                  -1.0f, QColor(16, 30, 44));
        Base::fillCircle(painter,
                         {float(track.left()) + float(track.width()) * value,
                          float(track.center().y())},
                         7.0f, active ? Base::White : Base::CyanSoft);
        Base::text(painter, Base::techFont(),
                   QStringLiteral("%1%").arg(int(std::lround(value * 100.0f))),
                   active ? Base::Cyan : Base::Text,
                   {float(track.right()) + 28.0f, float(rect.center().y())},
                   QStringLiteral("midleft"));
    }

    Base::panel(painter, backRect(), Base::Cyan, 225, 9, QColor(7, 18, 31));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回"), Base::Cyan,
               {float(backRect().center().x()), float(backRect().center().y())},
               QStringLiteral("center"));

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("↑ ↓ 选择    ← → 调整    ESC 返回    M 静音"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 34.0f},
               QStringLiteral("center"), 160);
}

} // namespace menu
