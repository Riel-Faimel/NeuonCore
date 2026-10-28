#include "GunnerEnemy.h"

#include "Arena/Bullets/Bullets.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemies/EnemyRegistry.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Base/Base.h"
#include "Data/GameData.h"
#include "Random.h"

#include <algorithm>
#include <cmath>

namespace arena {

namespace {
const EnemyRegistrar<GunnerEnemy> kRegistration("gunner", QColor(255, 96, 120));
}

GunnerEnemy::GunnerEnemy(const Math::Vec2 &position, int wave)
    : RangedEnemy(typeid(GunnerEnemy), Role::Ranged, position, wave)
{
    // 进场后 ~0.4s 就开始持续输出（比 spitter 的首发更早）。
    attackTimer = RandomService.real(0.35f, 0.6f);
}

void GunnerEnemy::updateSpecial(const Context &ctx, float dt)
{
    (void)dt;
    if (age <= 0.4f || attackTimer > 0.0f) {
        return;
    }
    const Math::Vec2 toPlayer =
        ctx.combatants.nearestLivingPlayer(pos).state.pos - pos;
    if (toPlayer.length() > FireRange) {
        return;
    }
    // 持续射击：没有点射/长冷却，只有下一发的间隔。
    attackTimer = RandomService.real(IntervalMin, IntervalMax);
    shoot(ctx);
}

void GunnerEnemy::shoot(const Context &ctx)
{
    const Math::Vec2 toPlayer = ctx.combatants.nearestLivingPlayer(pos).state.pos - pos;
    const float base = std::atan2(toPlayer.y, toPlayer.x);
    const Math::Vec2 aim = Math::Vec2(std::cos(base), std::sin(base))
                               .rotated(RandomService.real(-AimJitter, AimJitter));
    const Math::Vec2 muzzle = pos + aim * 16.0f;
    // 敌方子弹默认不吸附：吸附是玩家强化“磁吸弹道”给的 buff。
    ctx.bullets.spawnStandard(
        muzzle, aim * data::kGunnerBullet.speed,
        data::kGunnerBullet.damage * damageMultiplier(), color,
        Bullet::Standpoint::Enemy, data::kGunnerBullet.radius);
    ctx.entities.spawnBurst(ctx, muzzle, color, 2, 60.0f, 0.16f);
    BaseService.playSound(QStringLiteral("shoot_enemy"));
}

void GunnerEnemy::drawBody(QPainter &world, QPainter &glow, float facing) const
{
    const Math::Vec2 forward(std::cos(facing), std::sin(facing));
    const float r = radius;
    Base::glow(glow, pos, r * 1.3f, color, 0.95f);

    // 炮塔：外圈 + 转动的三根枪管，age 驱动，连射时视觉上一直在转。
    Base::strokeCircle(world, pos, r, Base::withAlpha(color, 190), 1.6f);
    Base::fillCircle(world, pos, r * 0.40f, Base::shade(color, 0.28f));
    const float spin = age * 9.0f;
    for (int i = 0; i < 3; ++i) {
        const float angle = spin + Math::Tau * float(i) / 3.0f;
        const Math::Vec2 barrel(std::cos(angle), std::sin(angle));
        Base::strokeLine(world, pos + barrel * (r * 0.34f), pos + barrel * (r + 5.0f),
                         Base::withAlpha(color, 195), 1.8f);
    }
    Base::strokeCircle(world, pos, r * 0.62f, Base::withAlpha(Base::White, 40), 1.0f);
    Base::fillCircle(world, pos + forward * (r + 4.0f), 2.4f, Base::White);
}

} // namespace arena
