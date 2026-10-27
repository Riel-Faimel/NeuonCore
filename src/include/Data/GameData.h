#pragma once

#include "GameDataJson.h"   // 构建期生成：src/json/*.json 的原样内容

#include "Json.h"

#include <array>
#include <string_view>

// 编译期数值表：src/json/*.json 由 constexpr 解析器在这里展开成常量，
// 运行时只读这些常量，不再解析 JSON。
namespace data {

// ----- 玩家基础数值（player.json） -----
struct PlayerBase {
    float hp = 100.0f;
    float speed = 292.0f;
    float damage = 12.5f;
    float fireCd = 0.155f;
    float bulletSpeed = 920.0f;
    float bulletRadius = 3.2f;
};

inline constexpr json::Document kPlayerDoc = json::parse(game_data::player_json);

constexpr PlayerBase loadPlayer()
{
    PlayerBase result;
    const json::Node &root = kPlayerDoc.root();
    result.hp = float(kPlayerDoc.number(kPlayerDoc.member(root, "hp"), result.hp));
    result.speed = float(kPlayerDoc.number(kPlayerDoc.member(root, "speed"), result.speed));
    result.damage = float(kPlayerDoc.number(kPlayerDoc.member(root, "damage"), result.damage));
    result.fireCd = float(kPlayerDoc.number(kPlayerDoc.member(root, "fire_cd"), result.fireCd));
    result.bulletSpeed = float(
        kPlayerDoc.number(kPlayerDoc.member(root, "bullet_speed"), result.bulletSpeed));
    result.bulletRadius = float(
        kPlayerDoc.number(kPlayerDoc.member(root, "bullet_radius"), result.bulletRadius));
    return result;
}

inline constexpr PlayerBase kPlayer = loadPlayer();

// ----- 敌人基础数值（enemies.json） -----
struct EnemyBase {
    std::string_view kind;
    float hp = 0.0f;
    float speed = 0.0f;
    float radius = 0.0f;
    float damage = 0.0f;
    int score = 0;
    // 支援型敌人的光环参数（普通敌人为 0）。
    float auraRadius = 0.0f;     // amplifier：增伤半径
    float auraValue = 0.0f;      // amplifier：增伤比例
    float shareRadius = 0.0f;    // linker：分担半径
    float shareRatio = 0.0f;     // linker：分担比例
    float healRadius = 0.0f;     // mender：治疗半径
    float healRate = 0.0f;       // mender：每秒治疗量
    float reviveRadius = 0.0f;   // necromancer：复活半径
    float reviveChance = 0.0f;   // necromancer：复活概率
    float lungeInterval = 0.0f;  // rusher：突进间隔
    int chargeRequired = 0;      // summoner：召唤所需击杀数
    int summonCount = 0;         // summoner：一次召唤数量
};

inline constexpr int MaxEnemyKinds = 16;

struct EnemyTable {
    std::array<EnemyBase, MaxEnemyKinds> items{};
    int count = 0;
};

inline constexpr json::Document kEnemiesDoc = json::parse(game_data::enemies_json);

constexpr EnemyTable loadEnemies()
{
    EnemyTable table;
    const json::Node &root = kEnemiesDoc.root();
    const int total = kEnemiesDoc.size(root);
    for (int i = 0; i < total && table.count < MaxEnemyKinds; ++i) {
        const json::Node *entry = kEnemiesDoc.at(root, i);
        if (entry == nullptr || entry->key < 0) {
            continue;
        }
        EnemyBase base;
        base.kind = kEnemiesDoc.node(entry->key).text;
        base.hp = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "hp"), base.hp));
        base.speed = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "speed"), base.speed));
        base.radius = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "radius"), base.radius));
        base.damage = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "damage"), base.damage));
        base.score = kEnemiesDoc.integer(kEnemiesDoc.member(*entry, "score"), base.score);
        base.auraRadius = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "aura_radius"), base.auraRadius));
        base.auraValue = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "aura_damage"), base.auraValue));
        base.shareRadius = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "share_radius"), base.shareRadius));
        base.shareRatio = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "share_ratio"), base.shareRatio));
        base.healRadius = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "heal_radius"), base.healRadius));
        base.healRate = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "heal_rate"), base.healRate));
        base.reviveRadius = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "revive_radius"), base.reviveRadius));
        base.reviveChance = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "revive_chance"), base.reviveChance));
        base.lungeInterval = float(kEnemiesDoc.number(kEnemiesDoc.member(*entry, "lunge_interval"), base.lungeInterval));
        base.chargeRequired = kEnemiesDoc.integer(kEnemiesDoc.member(*entry, "charge_required"), base.chargeRequired);
        base.summonCount = kEnemiesDoc.integer(kEnemiesDoc.member(*entry, "summon_count"), base.summonCount);
        table.items[table.count++] = base;
    }
    return table;
}

inline constexpr EnemyTable kEnemies = loadEnemies();

constexpr const EnemyBase *findEnemy(std::string_view kind)
{
    for (int i = 0; i < kEnemies.count; ++i) {
        if (kEnemies.items[i].kind == kind) {
            return &kEnemies.items[i];
        }
    }
    return nullptr;
}

constexpr int findEnemyIndex(std::string_view kind)
{
    for (int i = 0; i < kEnemies.count; ++i) {
        if (kEnemies.items[i].kind == kind) {
            return i;
        }
    }
    return -1;
}

// ----- 战役台本与出怪节奏（src/json/waves/level-0N.json） -----
struct WaveSpawn {
    std::string_view kind;
    int count = 0;
};

inline constexpr int MaxSpawnsPerWave = 8;
inline constexpr int MaxCampaignWaves = 16;

struct WaveDef {
    std::array<WaveSpawn, MaxSpawnsPerWave> spawns{};
    int count = 0;
};

struct Campaign {
    std::array<WaveDef, MaxCampaignWaves> waves{};
    int count = 0;
};

struct WaveTiming {
    float firstSpawn = 0.95f;
    float laterSpawn = 1.15f;
    float gapMin = 0.38f;
    float gapMax = 0.68f;
};

struct WaveScaling {
    float speed = 0.022f;
    float damage = 0.045f;
    float hp = 0.16f;
};

inline constexpr std::array<json::Document, 4> kWavesDocs = {
    json::parse(game_data::waves_level_01),
    json::parse(game_data::waves_level_02),
    json::parse(game_data::waves_level_03),
    json::parse(game_data::waves_level_04),
};

constexpr Campaign loadCampaign(const json::Document &doc)
{
    Campaign campaign;
    const json::Node *waves = doc.member(doc.root(), "campaign");
    if (waves == nullptr) {
        return campaign;
    }
    const int waveCount = doc.size(*waves);
    for (int w = 0; w < waveCount && campaign.count < MaxCampaignWaves; ++w) {
        const json::Node *waveNode = doc.at(*waves, w);
        if (waveNode == nullptr) {
            continue;
        }
        WaveDef wave;
        const int spawnCount = doc.size(*waveNode);
        for (int s = 0; s < spawnCount && wave.count < MaxSpawnsPerWave; ++s) {
            const json::Node *spawn = doc.at(*waveNode, s);
            if (spawn == nullptr) {
                continue;
            }
            WaveSpawn entry;
            entry.kind = doc.text(doc.member(*spawn, "kind"));
            entry.count = doc.integer(doc.member(*spawn, "count"));
            wave.spawns[wave.count++] = entry;
        }
        campaign.waves[campaign.count++] = wave;
    }
    return campaign;
}

constexpr WaveTiming loadWaveTiming(const json::Document &doc)
{
    WaveTiming timing;
    const json::Node *spawn = doc.member(doc.root(), "spawn");
    if (spawn == nullptr) {
        return timing;
    }
    timing.firstSpawn = float(doc.number(doc.member(*spawn, "first"), timing.firstSpawn));
    timing.laterSpawn = float(doc.number(doc.member(*spawn, "later"), timing.laterSpawn));
    timing.gapMin = float(doc.number(doc.member(*spawn, "gap_min"), timing.gapMin));
    timing.gapMax = float(doc.number(doc.member(*spawn, "gap_max"), timing.gapMax));
    return timing;
}

constexpr WaveScaling loadWaveScaling(const json::Document &doc)
{
    WaveScaling scaling;
    const json::Node *node = doc.member(doc.root(), "scaling");
    if (node == nullptr) {
        return scaling;
    }
    scaling.speed = float(doc.number(doc.member(*node, "speed"), scaling.speed));
    scaling.damage = float(doc.number(doc.member(*node, "damage"), scaling.damage));
    scaling.hp = float(doc.number(doc.member(*node, "hp"), scaling.hp));
    return scaling;
}

inline constexpr std::array<Campaign, 4> kCampaigns = {
    loadCampaign(kWavesDocs[0]),
    loadCampaign(kWavesDocs[1]),
    loadCampaign(kWavesDocs[2]),
    loadCampaign(kWavesDocs[3]),
};
inline constexpr Campaign kCampaign = kCampaigns[0];
inline constexpr WaveTiming kWaveTiming = loadWaveTiming(kWavesDocs[0]);
inline constexpr WaveScaling kWaveScaling = loadWaveScaling(kWavesDocs[0]);

// ----- 战备发射器（loadout.json） -----
struct LauncherDef {
    std::string_view id;
    std::string_view name;
    std::string_view subtitle;
    float damageScale = 1.0f;
    float speedScale = 1.0f;
    float fireCdScale = 1.0f;
    float spreadScale = 1.0f;
    float radius = 3.0f;
    int extraShots = 0;
    int pierceBonus = 0;
    std::string_view color;
};

inline constexpr int MaxLaunchers = 4;

struct LauncherTable {
    std::array<LauncherDef, MaxLaunchers> items{};
    int count = 0;
};

inline constexpr json::Document kLoadoutDoc = json::parse(game_data::loadout_json);

constexpr LauncherTable loadLaunchers()
{
    LauncherTable table;
    const json::Node *launchers = kLoadoutDoc.member(kLoadoutDoc.root(), "launchers");
    if (launchers == nullptr) {
        return table;
    }
    const int total = kLoadoutDoc.size(*launchers);
    for (int i = 0; i < total && table.count < MaxLaunchers; ++i) {
        const json::Node *entry = kLoadoutDoc.at(*launchers, i);
        if (entry == nullptr) {
            continue;
        }
        LauncherDef def;
        def.id = kLoadoutDoc.text(kLoadoutDoc.member(*entry, "id"));
        def.name = kLoadoutDoc.text(kLoadoutDoc.member(*entry, "name"));
        def.subtitle = kLoadoutDoc.text(kLoadoutDoc.member(*entry, "subtitle"));
        def.damageScale = float(kLoadoutDoc.number(kLoadoutDoc.member(*entry, "damage"), def.damageScale));
        def.speedScale = float(kLoadoutDoc.number(kLoadoutDoc.member(*entry, "speed"), def.speedScale));
        def.fireCdScale = float(kLoadoutDoc.number(kLoadoutDoc.member(*entry, "fire_cd"), def.fireCdScale));
        def.spreadScale = float(kLoadoutDoc.number(kLoadoutDoc.member(*entry, "spread"), def.spreadScale));
        def.radius = float(kLoadoutDoc.number(kLoadoutDoc.member(*entry, "radius"), def.radius));
        def.extraShots = kLoadoutDoc.integer(kLoadoutDoc.member(*entry, "extra_shots"), def.extraShots);
        def.pierceBonus = kLoadoutDoc.integer(kLoadoutDoc.member(*entry, "pierce"), def.pierceBonus);
        def.color = kLoadoutDoc.text(kLoadoutDoc.member(*entry, "color"));
        table.items[table.count++] = def;
    }
    return table;
}

inline constexpr LauncherTable kLaunchers = loadLaunchers();

// ----- 强化数值区间（upgrades.json） -----
struct UpgradeRange {
    std::string_view key;
    float min = 0.0f;
    float max = 0.0f;
};

inline constexpr int MaxUpgradeRanges = 16;

struct UpgradeTable {
    std::array<UpgradeRange, MaxUpgradeRanges> items{};
    int count = 0;
};

inline constexpr json::Document kUpgradesDoc = json::parse(game_data::upgrades_json);

constexpr UpgradeTable loadUpgradeRanges()
{
    UpgradeTable table;
    const json::Node &root = kUpgradesDoc.root();
    const int total = kUpgradesDoc.size(root);
    for (int i = 0; i < total && table.count < MaxUpgradeRanges; ++i) {
        const json::Node *entry = kUpgradesDoc.at(root, i);
        if (entry == nullptr || entry->key < 0) {
            continue;
        }
        UpgradeRange range;
        range.key = kUpgradesDoc.node(entry->key).text;
        range.min = float(kUpgradesDoc.number(kUpgradesDoc.member(*entry, "min"), range.min));
        range.max = float(kUpgradesDoc.number(kUpgradesDoc.member(*entry, "max"), range.max));
        table.items[table.count++] = range;
    }
    return table;
}

inline constexpr UpgradeTable kUpgradeRanges = loadUpgradeRanges();

constexpr const UpgradeRange *findUpgrade(std::string_view key)
{
    for (int i = 0; i < kUpgradeRanges.count; ++i) {
        if (kUpgradeRanges.items[i].key == key) {
            return &kUpgradeRanges.items[i];
        }
    }
    return nullptr;
}

// ----- 子弹数值（bullets.json） -----
struct BulletSpec {
    float speed = 0.0f;
    float damage = 0.0f;
    float radius = 3.0f;
};

inline constexpr json::Document kBulletsDoc = json::parse(game_data::bullets_json);

constexpr BulletSpec loadBullet(std::string_view key)
{
    BulletSpec spec;
    const json::Node *node = kBulletsDoc.member(kBulletsDoc.root(), key);
    if (node == nullptr) {
        return spec;
    }
    spec.speed = float(kBulletsDoc.number(kBulletsDoc.member(*node, "speed"), spec.speed));
    spec.damage = float(kBulletsDoc.number(kBulletsDoc.member(*node, "damage"), spec.damage));
    spec.radius = float(kBulletsDoc.number(kBulletsDoc.member(*node, "radius"), spec.radius));
    return spec;
}

inline constexpr BulletSpec kBossAimedBullet = loadBullet("boss_aimed");
inline constexpr BulletSpec kBossRadialBullet = loadBullet("boss_radial");
inline constexpr BulletSpec kSpitterBullet = loadBullet("spitter");

// ----- 商店道具（items.json） -----
struct MineSpec {
    int price = 120;
    float damage = 240.0f;
    float triggerRadius = 26.0f;
    float blastRadius = 110.0f;
    float armTime = 0.35f;
    float lifetime = 22.0f;
};

inline constexpr json::Document kItemsDoc = json::parse(game_data::items_json);

constexpr MineSpec loadMine()
{
    MineSpec spec;
    const json::Node *node = kItemsDoc.member(kItemsDoc.root(), "mine");
    if (node == nullptr) {
        return spec;
    }
    spec.price = kItemsDoc.integer(kItemsDoc.member(*node, "price"), spec.price);
    spec.damage = float(kItemsDoc.number(kItemsDoc.member(*node, "damage"), spec.damage));
    spec.triggerRadius = float(
        kItemsDoc.number(kItemsDoc.member(*node, "trigger_radius"), spec.triggerRadius));
    spec.blastRadius = float(
        kItemsDoc.number(kItemsDoc.member(*node, "blast_radius"), spec.blastRadius));
    spec.armTime = float(kItemsDoc.number(kItemsDoc.member(*node, "arm_time"), spec.armTime));
    spec.lifetime = float(kItemsDoc.number(kItemsDoc.member(*node, "lifetime"), spec.lifetime));
    return spec;
}

inline constexpr MineSpec kMine = loadMine();

// ----- 星区地图（levels.json） -----
struct LevelNode {
    std::string_view name;
    std::string_view subtitle;
    std::string_view waves;   // src/json/waves/<waves>.json
    float x = 0.5f;
    float y = 0.5f;
    bool locked = true;
};

inline constexpr int MaxLevelNodes = 8;

struct LevelMap {
    std::array<LevelNode, MaxLevelNodes> items{};
    int count = 0;
};

inline constexpr json::Document kLevelsDoc = json::parse(game_data::levels_json);

constexpr LevelMap loadLevelMap()
{
    LevelMap map;
    const json::Node *levels = kLevelsDoc.member(kLevelsDoc.root(), "levels");
    if (levels == nullptr) {
        return map;
    }
    const int total = kLevelsDoc.size(*levels);
    for (int i = 0; i < total && map.count < MaxLevelNodes; ++i) {
        const json::Node *entry = kLevelsDoc.at(*levels, i);
        if (entry == nullptr) {
            continue;
        }
        LevelNode node;
        node.name = kLevelsDoc.text(kLevelsDoc.member(*entry, "name"));
        node.subtitle = kLevelsDoc.text(kLevelsDoc.member(*entry, "subtitle"));
        node.waves = kLevelsDoc.text(kLevelsDoc.member(*entry, "waves"));
        node.x = float(kLevelsDoc.number(kLevelsDoc.member(*entry, "x"), node.x));
        node.y = float(kLevelsDoc.number(kLevelsDoc.member(*entry, "y"), node.y));
        node.locked = kLevelsDoc.boolean(kLevelsDoc.member(*entry, "locked"), node.locked);
        map.items[map.count++] = node;
    }
    return map;
}

inline constexpr LevelMap kLevelNodes = loadLevelMap();

// 运行期数据自检：返回 nullptr 表示通过，否则返回具体错误。
// 构建期的 JSON 解析失败会让常量退化成默认值/空表；这里在启动入口显式处理，
// 不依赖断言（release 构建同样会执行这套检查）。
inline const char *validationError()
{
    if (!(kPlayer.hp > 0.0f)) {
        return "src/json/player.json: hp must be > 0";
    }
    if (kEnemies.count <= 0) {
        return "src/json/enemies.json: enemy table is empty";
    }
    for (const Campaign &campaign : kCampaigns) {
        if (campaign.count <= 0) {
            return "src/json/waves/: a campaign is empty";
        }
    }
    if (kLaunchers.count <= 0) {
        return "src/json/loadout.json: launcher table is empty";
    }
    if (kUpgradeRanges.count <= 0) {
        return "src/json/upgrades.json: upgrade table is empty";
    }
    if (!(kBossAimedBullet.speed > 0.0f)) {
        return "src/json/bullets.json: boss aimed bullet speed must be > 0";
    }
    if (kMine.price <= 0) {
        return "src/json/items.json: mine price must be > 0";
    }
    if (kLevelNodes.count <= 0) {
        return "src/json/levels.json: level table is empty";
    }
    return nullptr;
}

} // namespace data
