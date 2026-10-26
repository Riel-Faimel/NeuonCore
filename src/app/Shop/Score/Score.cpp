#include "Shop/Score/Score.h"

#include "Data/Settings.h"

#include <algorithm>

namespace shop {

void Score::load()
{
    QSettings settings = app::openSettings();
    total_ = std::max(0, settings.value(QStringLiteral("score/total"), 0).toInt());
    runStart_ = total_;
    kills_ = 0;
}

void Score::save()
{
    QSettings settings = app::openSettings();
    settings.setValue(QStringLiteral("score/total"), total_);
}

void Score::beginRun()
{
    runStart_ = total_;
    kills_ = 0;
}

void Score::addKill(int points)
{
    add(points);
    ++kills_;
}

void Score::add(int points)
{
    if (points <= 0) {
        return;
    }
    total_ += points;
}

bool Score::spend(int points)
{
    if (points <= 0 || points > total_) {
        return false;
    }
    total_ -= points;
    return true;
}

} // namespace shop
