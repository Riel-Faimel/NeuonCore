#include "Shop/Shop.h"

#include "Base/Base.h"
#include "Data/GameData.h"

void Shop::load()
{
    score_.load();
    inventory_.load();
}

void Shop::save()
{
    score_.save();
    inventory_.save();
}

bool Shop::buyMine(Base &base)
{
    if (!score_.spend(data::kMine.price)) {
        base.playSound(QStringLiteral("ui"));
        return false;
    }
    inventory_.addMine(1);
    inventory_.save();
    score_.save();
    base.playSound(QStringLiteral("upgrade"));
    return true;
}
