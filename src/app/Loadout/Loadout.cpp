#include "Loadout/Loadout.h"

#include "Base/Base.h"
#include "Data/Settings.h"

#include <QSettings>

#include <algorithm>

Loadout LoadoutService;

namespace {

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), int(text.size()));
}

} // namespace

void Loadout::load()
{
    QSettings settings = app::openSettings();
    const QString saved = settings.value(QStringLiteral("loadout/launcher")).toString();
    selected_ = 0;
    if (!saved.isEmpty()) {
        for (int i = 0; i < count(); ++i) {
            if (toQString(data::kLaunchers.items[size_t(i)].id) == saved) {
                selected_ = i;
                break;
            }
        }
    }
}

void Loadout::save() const
{
    if (count() <= 0) {
        return;
    }
    QSettings settings = app::openSettings();
    settings.setValue(QStringLiteral("loadout/launcher"),
                      toQString(launcher().id));
}

void Loadout::select(int index)
{
    if (count() <= 0) {
        return;
    }
    selected_ = std::clamp(index, 0, count() - 1);
    save();
}

const data::LauncherDef &Loadout::launcher() const
{
    static const data::LauncherDef fallback;
    if (count() <= 0) {
        return fallback;
    }
    return data::kLaunchers.items[size_t(std::clamp(selected_, 0, count() - 1))];
}

QColor Loadout::color() const
{
    if (count() <= 0) {
        return Base::Cyan;
    }
    const std::string_view key = launcher().color;
    if (key == "amber") {
        return Base::Amber;
    }
    if (key == "violet") {
        return Base::Violet;
    }
    return Base::Cyan;
}
