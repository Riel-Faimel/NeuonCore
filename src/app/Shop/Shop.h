#pragma once

#include "Shop/Inventory/Inventory.h"
#include "Shop/Score/Score.h"

// Shop 全局模块：组合 shop::Score（钱包）和 shop::Inventory（库存）两个组件，
// 对外提供查询与购买接口；持久化到 exe 同级的 settings.ini。
// 界面（补给站）是 Menu 的组件，只调用这里的接口。
class Shop {
public:
    void load();
    void save();

    shop::Score &score() { return score_; }
    const shop::Score &score() const { return score_; }
    shop::Inventory &inventory() { return inventory_; }
    const shop::Inventory &inventory() const { return inventory_; }

    bool buyMine();

private:
    shop::Score score_;
    shop::Inventory inventory_;
};

// 进程内唯一的 Shop：钱包与库存。main() 启动时 load()、退出前 save()，
// Menu / Games / Arena / 补给站界面都直接使用它，不再由某个模块持有。
extern Shop ShopService;
