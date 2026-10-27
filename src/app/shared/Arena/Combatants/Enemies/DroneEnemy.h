#pragma once

#include "EnemyTypes.h"

namespace arena {

class DroneEnemy final : public OffensiveEnemy {
public:
    DroneEnemy(const Math::Vec2 &position, int wave);

protected:
    float weave() const override;
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;
};

} // namespace arena
