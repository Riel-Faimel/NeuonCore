#pragma once

#include "Data/GameData.h"

#include <QColor>

// Loadout 全局模块：玩家在“战备”界面选择的发射器。
// 只保存选择并持久化；数值来自 src/json/loadout.json，由 Player 在开局/射击时读取。
class Loadout {
public:
    void load();
    void save() const;

    int selected() const { return selected_; }
    void select(int index);

    int count() const { return data::kLaunchers.count; }
    const data::LauncherDef &launcher() const;
    QColor color() const;

private:
    int selected_ = 0;
};

// 进程内唯一的战备配置。
extern Loadout LoadoutService;
