#include "Shop/Inventory/Inventory.h"

#include "Data/Settings.h"

#include <algorithm>

namespace shop {

void Inventory::load()
{
    QSettings settings = app::openSettings();
    mines_ = std::max(0, settings.value(QStringLiteral("inventory/mines"), 0).toInt());
}

void Inventory::save()
{
    QSettings settings = app::openSettings();
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
