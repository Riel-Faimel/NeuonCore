#pragma once

#include "EnemyTypes.h"

namespace arena {

class BossEnemy final : public Enemy {
public:
    BossEnemy(const Math::Vec2 &position, int wave);

protected:
    Math::Vec2 desiredVelocity(const Context &ctx, float dt) override;
    void updateSpecial(const Context &ctx, float dt) override;
    void shoot(const Context &ctx) override;
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;

private:
    void fireAimed(const Context &ctx, int count, float spread);
    void drawBoss(QPainter &world, QPainter &glow, float facing) const;
};

} // namespace arena
