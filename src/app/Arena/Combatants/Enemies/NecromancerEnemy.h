#pragma once

#include "EnemyTypes.h"

namespace arena {

class NecromancerEnemy final : public SupportEnemy {
public:
    NecromancerEnemy(const Math::Vec2 &position, int wave);

    void onAllyKilled(const Context &ctx, Enemy &dead) override;

protected:
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;
};

} // namespace arena
