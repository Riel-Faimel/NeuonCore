#include "Games/Upgrades/Upgrades.h"

#include "Arena/Arena.h"
#include "Random.h"
#include "Base/Base.h"

#include <algorithm>

namespace games {

Upgrades::Upgrades(Random &random, Arena &arena, Base &base)
    : random_(random), arena_(arena), base_(base)
{
}

void Upgrades::reset()
{
    counts_.clear();
    offers_.clear();
    open_ = false;
}

const std::vector<Upgrades::Definition> &Upgrades::catalog()
{
    static const std::vector<Definition> definitions = {
        {QStringLiteral("damage"), QStringLiteral("火力校准"), QStringLiteral("DMG"),
         QStringLiteral("所有脉冲弹丸伤害 +25%。简单、直接、有效。"), Base::Coral},
        {QStringLiteral("rate"), QStringLiteral("超频供弹"), QStringLiteral("RPM"),
         QStringLiteral("射速 +18%。让下一波弹幕再密一点。"), Base::Amber},
        {QStringLiteral("multi"), QStringLiteral("分裂弹头"), QStringLiteral("x2"),
         QStringLiteral("每次射击额外发射 1 枚弹丸，弹道会轻微展开。"), Base::Cyan},
        {QStringLiteral("pierce"), QStringLiteral("穿甲弹芯"), QStringLiteral("PEN"),
         QStringLiteral("子弹额外穿透 1 个目标，适合对付成群的敌人。"), Base::Violet},
        {QStringLiteral("speed"), QStringLiteral("相位推进"), QStringLiteral("SPD"),
         QStringLiteral("移动速度 +12%，相位冲刺冷却 -10%。"), Base::Green},
        {QStringLiteral("maxhp"), QStringLiteral("复合装甲"), QStringLiteral("HP"),
         QStringLiteral("最大生命 +30，并立刻回复 30 点生命。"),
         QColor(255, 121, 136)},
        {QStringLiteral("heal"), QStringLiteral("纳米修复"), QStringLiteral("FIX"),
         QStringLiteral("立刻回复 50% 最大生命。活着才有输出。"),
         QColor(119, 255, 178)},
        {QStringLiteral("shock"), QStringLiteral("冲击核心"), QStringLiteral("AOE"),
         QStringLiteral("冲刺时释放冲击波，对附近敌人造成伤害并击退。"), Base::Magenta},
        {QStringLiteral("crit"), QStringLiteral("精准瞄具"), QStringLiteral("CRIT"),
         QStringLiteral("暴击率 +12%。暴击造成双倍伤害。"),
         QColor(255, 224, 121)},
    };
    return definitions;
}

bool Upgrades::eligible(const Definition &definition) const
{
    const Arena::PlayerState &player = arena_.player();
    if (definition.key == QStringLiteral("multi") && player.multishot >= 5) {
        return false;
    }
    if (definition.key == QStringLiteral("pierce") && player.pierce >= 5) {
        return false;
    }
    if (definition.key == QStringLiteral("crit") && player.critChance >= 0.60f) {
        return false;
    }
    if (definition.key == QStringLiteral("heal") && player.hp >= player.maxHp * 0.90f) {
        return false;
    }
    return true;
}

bool Upgrades::open()
{
    if (open_) {
        return false;
    }
    std::vector<Definition> pool;
    for (const Definition &definition : catalog()) {
        if (eligible(definition)) {
            pool.push_back(definition);
        }
    }
    if (pool.size() < 3) {
        pool = catalog();
    }
    random_.shuffle(pool);
    if (pool.size() > 3) {
        pool.resize(3);
    }

    offers_.clear();
    offers_.reserve(pool.size());
    for (const Definition &definition : pool) {
        offers_.push_back(Offer{definition.key, definition.name, definition.tag,
                                definition.desc, definition.color,
                                counts_.value(definition.key)});
    }
    open_ = true;
    base_.playSound(QStringLiteral("upgrade"));
    return true;
}

bool Upgrades::apply(int index)
{
    if (!open_ || index < 0 || index >= int(offers_.size())) {
        return false;
    }
    const Offer offer = offers_.at(size_t(index));
    applyEffect(offer.key);
    counts_[offer.key] += 1;
    if (arena_.player().hp < arena_.player().maxHp) {
        arena_.editablePlayer().hp = std::min(
            arena_.player().maxHp, arena_.player().hp + 10.0f);
    }
    arena_.celebrateUpgrade();
    base_.playSound(QStringLiteral("upgrade"));
    offers_.clear();
    open_ = false;
    return true;
}

void Upgrades::applyEffect(const QString &key)
{
    Arena::PlayerState &player = arena_.editablePlayer();
    if (key == QStringLiteral("damage")) {
        player.damage *= 1.25f;
    } else if (key == QStringLiteral("rate")) {
        player.fireCd = std::max(0.056f, player.fireCd * 0.82f);
    } else if (key == QStringLiteral("multi")) {
        player.multishot = std::min(5, player.multishot + 1);
    } else if (key == QStringLiteral("pierce")) {
        player.pierce = std::min(5, player.pierce + 1);
    } else if (key == QStringLiteral("speed")) {
        player.speed *= 1.12f;
        player.dashCdMax = std::max(0.72f, player.dashCdMax * 0.90f);
    } else if (key == QStringLiteral("maxhp")) {
        player.maxHp += 30.0f;
        player.hp = std::min(player.maxHp, player.hp + 30.0f);
        player.hpDisplay = std::min(player.hpDisplay, player.hp);
    } else if (key == QStringLiteral("heal")) {
        player.hp = std::min(player.maxHp, player.hp + player.maxHp * 0.50f);
    } else if (key == QStringLiteral("shock")) {
        player.shockLevel += 1;
    } else if (key == QStringLiteral("crit")) {
        player.critChance = std::min(0.60f, player.critChance + 0.12f);
    }
}

} // namespace games
