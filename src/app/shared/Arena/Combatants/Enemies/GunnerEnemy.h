#pragma once

#include "EnemyTypes.h"

namespace arena {

// GunnerEnemy：持续连射型远程敌人（射速极快的敌人）。
// 走位沿用远程型（保持射程、躲防御者背后、弹幕逼近时侧闪），
// 攻击是**不间断**的：每 0.10–0.15 秒打一发，没有点射/冷却分段，
// 所以它在场上时火力是持续压力。单发伤害低、瞄准有抖动，靠密度压制。
class GunnerEnemy final : public RangedEnemy {
public:
    GunnerEnemy(const Math::Vec2 &position, int wave);

    void shoot(const Context &ctx) override;

protected:
    void updateSpecial(const Context &ctx, float dt) override;
    void drawBody(QPainter &world, QPainter &glow, float facing) const override;

private:
    static constexpr float IntervalMin = 0.10f;  // 连续射击的间隔（约 7–10 发/秒）
    static constexpr float IntervalMax = 0.15f;
    static constexpr float FireRange = 620.0f;
    static constexpr float AimJitter = 0.10f;    // 连射精度差：瞄准角随机偏移
};

} // namespace arena
