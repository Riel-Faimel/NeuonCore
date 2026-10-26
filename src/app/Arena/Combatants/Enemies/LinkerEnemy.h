#pragma once

#include "EnemyTypes.h"

namespace arena {

class LinkerEnemy final : public DefensiveEnemy {
public:
    LinkerEnemy(const Math::Vec2 &position, int wave);

    float damageShareRadius() const override { return supportRadius; }
    float damageShareRatio() const override { return supportValue; }

protected:
    bool showsAura() const override { return true; }
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;
};

} // namespace arena
