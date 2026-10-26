#pragma once

#include "EnemyTypes.h"

namespace arena {

class RusherEnemy final : public OffensiveEnemy {
public:
    RusherEnemy(const Math::Vec2 &position, int wave);

protected:
    Math::Vec2 desiredVelocity(const Context &ctx, float dt) override;
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;
};

} // namespace arena
