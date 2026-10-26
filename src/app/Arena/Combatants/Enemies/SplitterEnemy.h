#pragma once

#include "EnemyTypes.h"

namespace arena {

// 自毁型：死亡时分裂成两只 mini。
class SplitterEnemy final : public SelfDestructEnemy {
public:
    SplitterEnemy(const Math::Vec2 &position, int wave);

    void onKilled(const Context &ctx) override;

protected:
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;
};

} // namespace arena
