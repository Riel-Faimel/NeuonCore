#pragma once

#include "Fighter.h"

#include <QColor>
#include <QString>

namespace arena {

struct Context;

struct EnemyStats {
    float hp = 10.0f;
    float speed = 100.0f;
    float radius = 10.0f;
    float damage = 10.0f;
    int score = 10;
    float auraRadius = 0.0f;
    float auraValue = 0.0f;
    float shareRadius = 0.0f;
    float shareRatio = 0.0f;
    float healRadius = 0.0f;
    float healRate = 0.0f;
    float reviveRadius = 0.0f;
    float reviveChance = 0.0f;
    float lungeInterval = 0.0f;
    int chargeRequired = 0;
    int summonCount = 0;
    QColor color;
};

// 敌人战斗者组件。一个实例代表一只敌人；Boss 也是其中一种 kind。
class Enemy : public Fighter {
public:
    // 热路径（update/draw）用枚举分支，避免每帧对 QString 做多次比较。
    enum class Kind {
        Drone,
        Spitter,
        Tank,
        Splitter,
        Mini,
        Boss,
        Amplifier,
        Linker,
        Mender,
        Rusher,
        Necromancer,
        Summoner,
        Other,
    };

    // 寻路/走位类别：近战贴人、远程躲弹幕、增益向友军集群靠拢。
    enum class Role {
        Melee,
        Ranged,
        Support,
    };

    QString kind;
    Kind kindId = Kind::Other;
    int eid = 0;
    Math::Vec2 pos;
    Math::Vec2 vel;
    int wave = 1;
    float radius = 13.0f;
    QColor color;
    int score = 10;
    float speed = 100.0f;
    float damage = 10.0f;
    float maxHp = 20.0f;
    float hp = 20.0f;
    float hpDisplay = 20.0f;
    bool alive = true;
    float age = 0.0f;
    float hitFlash = 0.0f;
    float attackTimer = 1.5f;
    float radialTimer = 2.0f;
    int strafe = 1;
    float phase = 0.0f;
    bool boss = false;

    // 支援型敌人的光环与被支援状态
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
    bool revived = false;

    Enemy(const QString &enemyKind, const Math::Vec2 &position, int waveNumber);

    void update(const Context &ctx, float dt) override;
    void shoot(const Context &ctx) override;
    void takeDamage(const Context &ctx, const Damage &damage) override;
    void die(const Context &ctx) override;
    void draw(QPainter &world, QPainter &glow, const Context &ctx) const override;

    bool isAlive() const override { return alive; }
    const Math::Vec2 &position() const override { return pos; }
    float collisionRadius() const override { return radius; }
    int id() const override { return eid; }
    float contactDamage() const override;
    float damageMultiplier() const;

    static const EnemyStats &statsFor(const QString &kind);
    static Role roleOf(Kind kind);
    Role role() const { return roleOf(kindId); }

private:
    static Kind kindOf(const QString &kind);

    void fireAimed(const Context &ctx, int count, float spread);
    void applyAura(const Context &ctx);
    void healAllies(const Context &ctx, float dt);
    Math::Vec2 allyClusterDirection(const Context &ctx) const;
    Math::Vec2 avoidPlayerBullets(const Context &ctx) const;
    void drawBody(QPainter &world, QPainter &glow, float facing) const;
    void drawBoss(QPainter &world, QPainter &glow, float facing) const;

    static int sNextId;
};

} // namespace arena
