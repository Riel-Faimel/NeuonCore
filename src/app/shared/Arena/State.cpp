#include "State.h"

#include "Arena/Combatants/Combatants.h"
#include "Arena/Context.h"
#include "Arena/Entities/Entities.h"
#include "Arena/Host.h"
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

void State::onBossDamage(const Context &ctx, Enemy &boss)
{
    const float fraction = boss.hp / boss.maxHp;
    if (fraction <= 0.66f && !phase2_) {
        phase2_ = true;
        ctx.combatants.spawnMinions(ctx, boss.pos, 2, boss.wave);
        ctx.entities.addShockwave(boss.pos, 130.0f, Base::Magenta, 0.5f, 3);
        ctx.host.addFeedback(Base::Magenta, 36, 0.0f);
        BaseService.playSound(QStringLiteral("boss"));
    }
    if (fraction <= 0.33f && !phase3_) {
        phase3_ = true;
        ctx.combatants.spawnMinions(ctx, boss.pos, 3, boss.wave);
        ctx.entities.addShockwave(boss.pos, 155.0f, Base::Magenta, 0.55f, 3);
        ctx.host.addFeedback(Base::Magenta, 45, 0.0f);
        BaseService.playSound(QStringLiteral("boss"));
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
