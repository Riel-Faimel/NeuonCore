#pragma once

#include "Net/Protocol.h"

#include "Math.h"

#include <QByteArray>
#include <QColor>
#include <QString>

#include <vector>

// 网络就绪的服务端 → 客户端世界快照（阶段 2：只做模型与序列化，不建连接）。
//
// 约束：
// - 只含数据，不含 Enemy / Bullet / 实体对象或指针；
// - 敌人/传送门类型用注册表的数据 key 字符串（"drone" 等），绝不传 std::type_index；
// - 字段都是绘制和 HUD 需要的信息，客户端用数据 key 重建具体类型后再填充字段；
// - 用 QDataStream 读写，所有结构默认生成 operator==，roundtrip 测试可逐字段比较。
namespace net {

enum class RunState : quint8 {
    Playing,
    Upgrade,
    Paused,
    Victory,
    Defeat,
};

enum class BulletSide : quint8 {
    Player,
    Enemy,
};

struct PlayerSnapshot {
    // 稳定玩家 id/slot（1..4）；同一会话在一局内保持不变。
    int playerId = 1;
    // 每架飞机的识别色，服务端在开局时分配，客户端只负责绘制。
    QColor color;
    Math::Vec2 pos;
    Math::Vec2 vel;
    Math::Vec2 dashDir{0.0f, -1.0f};
    Math::Vec2 muzzlePos;
    float radius = 14.0f;
    float speed = 292.0f;
    float maxHp = 100.0f;
    float hp = 100.0f;
    float hpDisplay = 100.0f;
    float damage = 12.5f;
    float fireCd = 0.155f;
    float shotTimer = 0.0f;
    float bulletSpeed = 920.0f;
    int multishot = 1;
    int pierce = 0;
    float critChance = 0.0f;
    int shockLevel = 0;
    float dashCdMax = 1.45f;
    float dashCd = 0.0f;
    float dashTime = 0.0f;
    float dashBuffer = 0.0f;
    float invuln = 0.0f;
    float fireFlash = 0.0f;
    float aim = -Math::Pi / 2.0f;
    bool alive = true;
    bool active = true;
    float spawnPulse = 0.7f;
    // >0 表示正在等待重新部署（多玩家时才有）。
    float respawnTimer = 0.0f;

    bool operator==(const PlayerSnapshot &) const = default;
};

struct EnemySnapshot {
    QString dataKey;
    int eid = 0;
    Math::Vec2 pos;
    Math::Vec2 vel;
    int wave = 1;
    int enemyLevel = 1;
    float radius = 13.0f;
    QColor color;
    int score = 10;
    float speed = 100.0f;
    float damage = 10.0f;
    float maxHp = 20.0f;
    float hp = 20.0f;
    float hpDisplay = 20.0f;
    bool alive = true;
    bool boss = false;
    bool revived = false;
    float age = 0.0f;
    float hitFlash = 0.0f;
    float attackTimer = 1.5f;
    float radialTimer = 2.0f;
    int strafe = 1;
    float phase = 0.0f;
    float supportRadius = 0.0f;
    float supportValue = 0.0f;
    float damageBuffTimer = 0.0f;
    float damageBuffBonus = 0.0f;
    float healFxTimer = 0.0f;
    float dodgeTimer = 0.0f;
    float dodgeCooldown = 0.0f;
    Math::Vec2 dodgeDir;
    float lungeInterval = 0.0f;
    float lungeTimer = 0.0f;
    int charge = 0;
    int chargeRequired = 0;
    int summonCount = 0;

    bool operator==(const EnemySnapshot &) const = default;
};

struct BulletSnapshot {
    Math::Vec2 pos;
    Math::Vec2 prev;
    Math::Vec2 vel;
    QColor color;
    float damage = 0.0f;
    float radius = 3.0f;
    BulletSide side = BulletSide::Player;
    bool alive = true;

    bool operator==(const BulletSnapshot &) const = default;
};

struct PortalSnapshot {
    QString dataKey;
    bool boss = false;
    Math::Vec2 pos;
    int wave = 1;
    int enemyLevel = 1;
    QColor color;
    float duration = 0.66f;
    float timer = 0.66f;
    float radius = 23.0f;
    bool alive = true;

    bool operator==(const PortalSnapshot &) const = default;
};

struct PickupSnapshot {
    Math::Vec2 pos;
    Math::Vec2 vel;
    int amount = 16;
    float age = 0.0f;
    float life = 14.0f;
    float phase = 0.0f;
    float radius = 10.0f;
    bool alive = true;

    bool operator==(const PickupSnapshot &) const = default;
};

struct MineSnapshot {
    Math::Vec2 pos;
    float radius = 14.0f;
    float triggerRadius = 26.0f;
    float blastRadius = 110.0f;
    float damage = 240.0f;
    float armTime = 0.35f;
    float life = 22.0f;
    float age = 0.0f;
    bool alive = true;

    bool operator==(const MineSnapshot &) const = default;
};

struct ParticleSnapshot {
    Math::Vec2 pos;
    Math::Vec2 vel;
    float life = 0.0f;
    float maxLife = 1.0f;
    float radius = 2.0f;
    QColor color;
    float drag = 2.2f;
    float gravity = 0.0f;
    int layer = 1;

    bool operator==(const ParticleSnapshot &) const = default;
};

struct FloatingTextSnapshot {
    Math::Vec2 pos;
    QString text;
    QColor color;
    float life = 0.8f;
    float maxLife = 0.8f;
    int size = 15;
    bool crit = false;

    bool operator==(const FloatingTextSnapshot &) const = default;
};

struct ShockwaveSnapshot {
    Math::Vec2 pos;
    float maxRadius = 50.0f;
    QColor color;
    float duration = 0.38f;
    float life = 0.38f;
    int width = 2;

    bool operator==(const ShockwaveSnapshot &) const = default;
};

// HUD 需要的最小波次状态；剩余威胁由服务端算好，客户端不访问模拟。
struct WaveSnapshot {
    int currentWave = 0;
    int totalWaves = 0;
    int totalEnemies = 0;
    int remainingThreats = 0;
    bool bossWave = false;
    bool finalWave = false;
    bool cleared = false;

    bool operator==(const WaveSnapshot &) const = default;
};

struct UpgradeOfferSnapshot {
    QString key;
    QString name;
    QString tag;
    QString desc;
    QString valueText;
    QColor color;
    int level = 0;
    float value = 0.0f;

    bool operator==(const UpgradeOfferSnapshot &) const = default;
};

struct BossSnapshot {
    bool active = false;
    float hp = 0.0f;
    float maxHp = 0.0f;
    float hpDisplay = 0.0f;

    bool operator==(const BossSnapshot &) const = default;
};

struct Snapshot {
    RunState state = RunState::Playing;
    float runTime = 0.0f;
    int score = 0;
    int kills = 0;
    // 本快照的接收者坐在哪一架飞机上（0 表示观战/没有飞机）。
    // 服务端对每个会话单独填写后再编码，客户端据此区分本地/远程飞机。
    int localPlayerId = 0;
    BossSnapshot boss;
    // 一局内的所有玩家飞机；顺序按 playerId 升序，单机时只有一架。
    std::vector<PlayerSnapshot> players;
    std::vector<EnemySnapshot> enemies;
    std::vector<BulletSnapshot> bullets;
    std::vector<PortalSnapshot> portals;
    std::vector<PickupSnapshot> pickups;
    std::vector<MineSnapshot> mines;
    std::vector<ParticleSnapshot> particles;
    std::vector<FloatingTextSnapshot> floatingTexts;
    std::vector<ShockwaveSnapshot> shockwaves;
    WaveSnapshot waves;
    std::vector<UpgradeOfferSnapshot> upgrades;

    // 清空内容但保留 vector 容量：单机每帧复用同一份 Snapshot。
    void clear();

    bool operator==(const Snapshot &) const = default;
};

// ----- QDataStream 序列化 -----
void writeSnapshot(QDataStream &stream, const Snapshot &snapshot);
bool readSnapshot(QDataStream &stream, Snapshot *snapshot, QString *error = nullptr);

// 裸 payload（带 kSnapshotDataVersion 校验）。
QByteArray serializeSnapshot(const Snapshot &snapshot);
bool deserializeSnapshot(const QByteArray &payload, Snapshot *snapshot,
                         QString *error = nullptr);

// 带 [length][type][version] 头的完整 Snapshot 消息，阶段 3 直接复用。
QByteArray encodeSnapshotMessage(const Snapshot &snapshot,
                                 quint16 version = kProtocolVersion);
bool decodeSnapshotMessage(const QByteArray &frame, Snapshot *snapshot,
                           QString *error = nullptr);

} // namespace net
