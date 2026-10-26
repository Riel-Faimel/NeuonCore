#include "Games/Upgrades/Upgrades.h"

#include "Arena/Arena.h"
#include "Random.h"
#include "Base/Base.h"

#include <algorithm>
#include <cmath>

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

// 每项的 min/max 是本轮随机数值的区间：百分比按 1% 取整，护甲按 1 点取整。
const std::vector<Upgrades::Definition> &Upgrades::catalog()
{
    static const std::vector<Definition> definitions = {
        {QStringLiteral("damage"), QStringLiteral("火力校准"), QStringLiteral("DMG"),
         Base::Coral, 0.15f, 0.35f},
        {QStringLiteral("rate"), QStringLiteral("超频供弹"), QStringLiteral("RPM"),
         Base::Amber, 0.10f, 0.24f},
        {QStringLiteral("multi"), QStringLiteral("分裂弹头"), QStringLiteral("x2"),
         Base::Cyan, 1.0f, 1.0f},
        {QStringLiteral("pierce"), QStringLiteral("穿甲弹芯"), QStringLiteral("PEN"),
         Base::Violet, 1.0f, 1.0f},
        {QStringLiteral("speed"), QStringLiteral("相位推进"), QStringLiteral("SPD"),
         Base::Green, 0.08f, 0.18f},
        {QStringLiteral("maxhp"), QStringLiteral("复合装甲"), QStringLiteral("HP"),
         QColor(255, 121, 136), 20.0f, 40.0f},
        {QStringLiteral("heal"), QStringLiteral("纳米修复"), QStringLiteral("FIX"),
         QColor(119, 255, 178), 0.35f, 0.60f},
        {QStringLiteral("shock"), QStringLiteral("冲击核心"), QStringLiteral("AOE"),
         Base::Magenta, 1.0f, 1.0f},
        {QStringLiteral("crit"), QStringLiteral("精准瞄具"), QStringLiteral("CRIT"),
         QColor(255, 224, 121), 0.08f, 0.16f},
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
    if (definition.key == QStringLiteral("rate") && player.fireCd <= 0.0565f) {
        return false;
    }
    if (definition.key == QStringLiteral("heal") && player.hp >= player.maxHp * 0.90f) {
        return false;
    }
    return true;
}

float Upgrades::rollValue(const Definition &definition) const
{
    float value = definition.minValue;
    if (definition.maxValue > definition.minValue) {
        value = random_.real(definition.minValue, definition.maxValue);
    }
    if (definition.key == QStringLiteral("maxhp")) {
        return std::round(value);
    }
    return std::round(value * 100.0f) / 100.0f;
}

QString Upgrades::describe(const Definition &definition, float value)
{
    const int percent = int(std::lround(value * 100.0f));
    if (definition.key == QStringLiteral("damage")) {
        return QStringLiteral("所有脉冲弹丸伤害 +%1%。简单、直接、有效。").arg(percent);
    }
    if (definition.key == QStringLiteral("rate")) {
        return QStringLiteral("射速 +%1%。让下一波弹幕再密一点。").arg(percent);
    }
    if (definition.key == QStringLiteral("multi")) {
        return QStringLiteral("每次射击额外发射 1 枚弹丸，弹道会轻微展开。");
    }
    if (definition.key == QStringLiteral("pierce")) {
        return QStringLiteral("子弹额外穿透 1 个目标，适合对付成群的敌人。");
    }
    if (definition.key == QStringLiteral("speed")) {
        return QStringLiteral("移动速度 +%1%，相位冲刺冷却 -%2%。")
            .arg(percent)
            .arg(int(std::lround(value * 0.85f * 100.0f)));
    }
    if (definition.key == QStringLiteral("maxhp")) {
        return QStringLiteral("最大生命 +%1，并立刻回复 %1 点生命。")
            .arg(int(std::lround(value)));
    }
    if (definition.key == QStringLiteral("heal")) {
        return QStringLiteral("立刻回复 %1% 最大生命。活着才有输出。").arg(percent);
    }
    if (definition.key == QStringLiteral("shock")) {
        return QStringLiteral("冲刺时释放冲击波，对附近敌人造成伤害并击退。");
    }
    if (definition.key == QStringLiteral("crit")) {
        return QStringLiteral("暴击率 +%1%。暴击造成双倍伤害。").arg(percent);
    }
    return QString();
}

QString Upgrades::valueLabel(const Definition &definition, float value)
{
    const int percent = int(std::lround(value * 100.0f));
    if (definition.key == QStringLiteral("maxhp")) {
        return QStringLiteral("+%1 HP").arg(int(std::lround(value)));
    }
    if (definition.key == QStringLiteral("heal")) {
        return QStringLiteral("+%1% HP").arg(percent);
    }
    if (definition.key == QStringLiteral("multi")) {
        return QStringLiteral("+1 弹");
    }
    if (definition.key == QStringLiteral("pierce")) {
        return QStringLiteral("+1 穿透");
    }
    if (definition.key == QStringLiteral("shock")) {
        return QStringLiteral("+1 级");
    }
    return QStringLiteral("+%1%").arg(percent);
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
        Offer offer;
        offer.key = definition.key;
        offer.name = definition.name;
        offer.tag = definition.tag;
        offer.color = definition.color;
        offer.level = counts_.value(definition.key);
        offer.value = rollValue(definition);
        offer.desc = describe(definition, offer.value);
        offer.valueText = valueLabel(definition, offer.value);
        offers_.push_back(offer);
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
    applyEffect(offer);
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

void Upgrades::applyEffect(const Offer &offer)
{
    Arena::PlayerState &player = arena_.editablePlayer();
    const QString &key = offer.key;
    if (key == QStringLiteral("damage")) {
        player.damage *= 1.0f + offer.value;
    } else if (key == QStringLiteral("rate")) {
        player.fireCd = std::max(0.056f, player.fireCd * (1.0f - offer.value));
    } else if (key == QStringLiteral("multi")) {
        player.multishot = std::min(5, player.multishot + 1);
    } else if (key == QStringLiteral("pierce")) {
        player.pierce = std::min(5, player.pierce + 1);
    } else if (key == QStringLiteral("speed")) {
        player.speed *= 1.0f + offer.value;
        player.dashCdMax = std::max(0.72f, player.dashCdMax * (1.0f - offer.value * 0.85f));
    } else if (key == QStringLiteral("maxhp")) {
        player.maxHp += offer.value;
        player.hp = std::min(player.maxHp, player.hp + offer.value);
        player.hpDisplay = std::min(player.hpDisplay, player.hp);
    } else if (key == QStringLiteral("heal")) {
        player.hp = std::min(player.maxHp, player.hp + player.maxHp * offer.value);
    } else if (key == QStringLiteral("shock")) {
        player.shockLevel += 1;
    } else if (key == QStringLiteral("crit")) {
        player.critChance = std::min(0.60f, player.critChance + offer.value);
    }
}

} // namespace games
