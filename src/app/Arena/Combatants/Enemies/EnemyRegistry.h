#pragma once

#include "Arena/Combatants/Enemy.h"

#include <QColor>

#include <memory>
#include <string>
#include <unordered_map>

namespace arena {

// 一个 kind 的注册信息：类名、枚举 id、寻路角色、颜色和工厂。
struct EnemyDef {
    Enemy::Kind kindId = Enemy::Kind::Other;
    Enemy::Role role = Enemy::Role::Melee;
    QColor color;
    std::unique_ptr<Enemy> (*factory)(const Math::Vec2 &, int) = nullptr;
};

using EnemyRegistry = std::unordered_map<std::string, EnemyDef>;

EnemyRegistry &enemyRegistry();

// CRTP 注册器：具体敌人类在 .cpp 里定义一个静态实例即可完成自动注册。
template <typename T>
struct EnemyRegistrar {
    EnemyRegistrar(Enemy::Kind kindId, const char *kindName, Enemy::Role role,
                   const QColor &color)
    {
        enemyRegistry()[kindName] = EnemyDef{
            kindId, role, color,
            [](const Math::Vec2 &position, int wave) -> std::unique_ptr<Enemy> {
                return std::make_unique<T>(position, wave);
            }};
    }
};

} // namespace arena
