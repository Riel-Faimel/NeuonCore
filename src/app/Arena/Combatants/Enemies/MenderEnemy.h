#pragma once

#include "EnemyTypes.h"

namespace arena {

class MenderEnemy final : public SupportEnemy {
public:
    MenderEnemy(const Math::Vec2 &position, int wave);

protected:
    void updateSpecial(const Context &ctx, float dt) override;
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;
};

} // namespace arena
