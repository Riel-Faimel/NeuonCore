#pragma once

#include "Arena/Combatants/Enemy.h"
#include "Arena/Combatants/EnemyType.h"

#include <QColor>

#include <memory>
#include <string_view>
#include <unordered_map>

namespace arena {

// 一个敌人类型的注册信息：颜色、工厂，以及仅用于数据边界的 JSON 键。
// 运行期行为判断一律用 EnemyType（RTTI），不再比较字符串。
struct EnemyDef {
    QColor color;
    std::unique_ptr<Enemy> (*factory)(const Math::Vec2 &, int) = nullptr;
    std::string_view dataKey;
};

using EnemyRegistry = std::unordered_map<EnemyType, EnemyDef>;

EnemyRegistry &enemyRegistry();

// CRTP 注册器：具体敌人类在 .cpp 里定义一个静态实例即可完成自动注册。
template <typename T>
struct EnemyRegistrar {
    EnemyRegistrar(const char *dataKey, const QColor &color)
    {
        enemyRegistry()[EnemyType(typeid(T))] = EnemyDef{
            color,
            [](const Math::Vec2 &position, int wave) -> std::unique_ptr<Enemy> {
                return std::make_unique<T>(position, wave);
            },
            dataKey};
    }
};

} // namespace arena
