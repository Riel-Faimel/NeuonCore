#pragma once

#include "EnemyTypes.h"

namespace arena {

class SummonerEnemy final : public RangedEnemy {
public:
    SummonerEnemy(const Math::Vec2 &position, int wave);

    void onAllyKilled(const Context &ctx, Enemy &dead) override;

protected:
    float minRange() const override { return 250.0f; }
    float maxRange() const override { return 340.0f; }
    float strafeFactor() const override { return 0.5f; }
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;
};

} // namespace arena
