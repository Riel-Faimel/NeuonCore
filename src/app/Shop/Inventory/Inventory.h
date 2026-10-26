#pragma once

#include <QString>

class SelfTest;

namespace shop {

// Inventory 组件：局外持有的消耗品（目前只有地雷）。
// 属于 Shop 全局模块；购买与使用后写入 exe 同级的 settings.ini（[inventory] mines）。
class Inventory {
public:
    void load();
    void save();

    int mines() const { return mines_; }
    void addMine(int count);
    bool takeMine();

private:
    friend class ::SelfTest;

    static QString path();

    int mines_ = 0;
};

} // namespace shop
