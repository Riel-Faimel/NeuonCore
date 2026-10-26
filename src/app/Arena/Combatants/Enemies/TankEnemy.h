#pragma once

#include "EnemyTypes.h"

namespace arena {

class TankEnemy final : public DefensiveEnemy {
public:
    TankEnemy(const Math::Vec2 &position, int wave);

protected:
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;
};

} // namespace arena
