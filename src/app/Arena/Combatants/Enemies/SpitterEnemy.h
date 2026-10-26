#pragma once

#include "EnemyTypes.h"

namespace arena {

class SpitterEnemy final : public RangedEnemy {
public:
    SpitterEnemy(const Math::Vec2 &position, int wave);

    void shoot(const Context &ctx) override;

protected:
    void updateSpecial(const Context &ctx, float dt) override;
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;
};

} // namespace arena
