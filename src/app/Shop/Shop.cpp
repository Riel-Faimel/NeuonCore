#include "Shop/Shop.h"

#include "Base/Base.h"
#include "Data/GameData.h"

// 进程内唯一的 Shop（声明见 Shop.h）。
Shop ShopService;

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

bool Shop::buyMine()
{
    if (!score_.spend(data::kMine.price)) {
        BaseService.playSound(QStringLiteral("ui"));
        return false;
    }
    inventory_.addMine(1);
    inventory_.save();
    score_.save();
    BaseService.playSound(QStringLiteral("upgrade"));
    return true;
}
