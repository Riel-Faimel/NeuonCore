#pragma once

#include <QString>

class SelfTest;

// Inventory 模块：局外持有的消耗品（目前只有地雷）。
// 进程级，由 Menu 持有；购买与使用后写入 exe 同级的 settings.ini（[inventory] mines）。
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
