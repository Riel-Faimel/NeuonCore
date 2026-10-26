#include "State.h"

#include "Arena/Arena.h"
#include "Arena/Combatants/Enemy.h"
#include "Base/Base.h"

namespace arena {

void State::reset()
{
    phase2_ = false;
    phase3_ = false;
    bossDefeated_ = false;
    playerDefeated_ = false;
}

void State::beginWave()
{
    phase2_ = false;
    phase3_ = false;
    bossDefeated_ = false;
}

void State::onBossDamage(Arena &arena, Enemy &boss)
{
    const float fraction = boss.hp / boss.maxHp;
    if (fraction <= 0.66f && !phase2_) {
        phase2_ = true;
        arena.combatants_.spawnMinions(arena, boss.pos, 2, boss.wave);
        arena.entities_.addShockwave(boss.pos, 130.0f, Base::Magenta, 0.5f, 3);
        arena.addFeedback(Base::Magenta, 36, 0.0f);
        arena.base_.playSound(QStringLiteral("boss"));
    }
    if (fraction <= 0.33f && !phase3_) {
        phase3_ = true;
        arena.combatants_.spawnMinions(arena, boss.pos, 3, boss.wave);
        arena.entities_.addShockwave(boss.pos, 155.0f, Base::Magenta, 0.55f, 3);
        arena.addFeedback(Base::Magenta, 45, 0.0f);
        arena.base_.playSound(QStringLiteral("boss"));
    }
}

void State::markBossDefeated()
{
    bossDefeated_ = true;
}

void State::markPlayerDefeated()
{
    playerDefeated_ = true;
}

} // namespace arena
