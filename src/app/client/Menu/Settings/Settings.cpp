#include "Menu/Settings/Settings.h"

#include "Base/Base.h"
#include "Data/Settings.h"
#include "GameConfig.h"

#include <QLinearGradient>
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

void Settings::setVolume(int row, float value)
{
    select(row);
    value = std::clamp(value, 0.0f, 1.0f);
    if (row_ == 0) {
        BaseService.setSoundVolume(value);
        BaseService.playSound(QStringLiteral("ui"));
    } else {
        BaseService.setMusicVolume(value);
    }
    save();
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

} // namespace menu
