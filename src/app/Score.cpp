#include "Score.h"

#include <QCoreApplication>
#include <QSettings>

#include <algorithm>

QString Score::path()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/settings.ini");
}

void Score::load()
{
    QSettings settings(path(), QSettings::IniFormat);
    total_ = std::max(0, settings.value(QStringLiteral("score/total"), 0).toInt());
    runStart_ = total_;
    kills_ = 0;
}

void Score::save()
{
    QSettings settings(path(), QSettings::IniFormat);
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
