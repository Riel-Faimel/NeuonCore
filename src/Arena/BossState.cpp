#include "BossState.h"

#include "Arena/Arena.h"
#include "Arena/Enemy.h"
#include "base/Base.h"

namespace arena {

void BossState::reset()
{
    phase2_ = false;
    phase3_ = false;
    defeated_ = false;
}

void BossState::onDamage(Arena &arena, Enemy &boss)
{
    const float fraction = boss.hp / boss.maxHp;
    if (fraction <= 0.66f && !phase2_) {
        phase2_ = true;
        arena.spawnMinions(boss.pos, 2, boss.wave);
        arena.addShockwave(boss.pos, 130.0f, Base::Magenta, 0.5f, 3);
        arena.addFeedback(Base::Magenta, 36, 0.0f);
        arena.base_.playSound(QStringLiteral("boss"));
    }
    if (fraction <= 0.33f && !phase3_) {
        phase3_ = true;
        arena.spawnMinions(boss.pos, 3, boss.wave);
        arena.addShockwave(boss.pos, 155.0f, Base::Magenta, 0.55f, 3);
        arena.addFeedback(Base::Magenta, 45, 0.0f);
        arena.base_.playSound(QStringLiteral("boss"));
    }
}

void BossState::markDefeated()
{
    defeated_ = true;
}

} // namespace arena
