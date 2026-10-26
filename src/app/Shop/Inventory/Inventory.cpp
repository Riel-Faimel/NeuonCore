#include "Shop/Inventory/Inventory.h"

#include <QCoreApplication>
#include <QSettings>

#include <algorithm>

namespace shop {

QString Inventory::path()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/settings.ini");
}

void Inventory::load()
{
    QSettings settings(path(), QSettings::IniFormat);
    mines_ = std::max(0, settings.value(QStringLiteral("inventory/mines"), 0).toInt());
}

void Inventory::save()
{
    QSettings settings(path(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("inventory/mines"), mines_);
}

void Inventory::addMine(int count)
{
    if (count > 0) {
        mines_ += count;
    }
}

bool Inventory::takeMine()
{
    if (mines_ <= 0) {
        return false;
    }
    --mines_;
    return true;
}

} // namespace shop
