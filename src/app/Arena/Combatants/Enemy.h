#pragma once

#include "Arena/Combatants/EnemyType.h"
#include "Fighter.h"

#include <QColor>
#include <QString>

#include <memory>
#include <optional>
#include <vector>

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

// 敌人最基类：只保留所有敌人共享的状态、通用移动积分、受击/死亡/绘制骨架。
// 具体敌人的走位、攻击、光环、死亡效果在 Enemies/ 下的子类里实现；
// Enemy.cpp 只负责基类和“kind -> 工厂”的注册表。
class Enemy : public Fighter {
private:
    // RTTI 类型标识；具体类型由子类构造函数传入 typeid(T)。
    EnemyType type_;

public:
    using Type = EnemyType;
    // 寻路/走位类别：近战贴人、远程躲弹幕、增益向友军集群靠拢。
    enum class Role {
        Melee,
        Ranged,
        Support,
    };

    // 敌人类型分类：AI 需要按类别筛选友军；用缓存值避免每帧 dynamic_cast。
    enum class Archetype {
        Offensive,
        Ranged,
        Defensive,
        Support,
        SelfDestruct,
        Boss,
    };

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

    ~Enemy() override = default;

    Enemy(const Enemy &) = delete;
    Enemy &operator=(const Enemy &) = delete;

    // ----- 注册表（按 RTTI 类型组织） -----
    // 运行时按类型创建具体敌人；字符串键只在数据边界（spawn/台本）使用。
    static std::unique_ptr<Enemy> create(Type type, const Math::Vec2 &position, int wave);
    static std::unique_ptr<Enemy> create(const QString &dataKey,
                                         const Math::Vec2 &position, int wave);
    static const EnemyStats &statsFor(Type type);
    static const EnemyStats &statsFor(const QString &dataKey);
    static std::optional<Type> typeForKey(const QString &dataKey);
    // 数据边界：类型 → JSON/协议使用的数据 key（未注册返回空字符串）。
    static QString dataKeyFor(Type type);
    static bool isBoss(Type type);
    void setLevel(int level);
    Type type() const { return type_; }
    Role role() const { return role_; }
    Archetype archetype() const { return archetype_; }

    // ----- 接口 -----
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

    // 友军阵亡通知：necromancer 复活、summoner 累积召唤进度在这里实现。
    virtual void onAllyKilled(const Context &ctx, Enemy &dead) { (void)ctx; (void)dead; }
    // 自己被击杀时的额外效果：splitter 分裂等。
    virtual void onKilled(const Context &ctx) { (void)ctx; }

    // linker 的伤害分担能力；默认不参与。
    virtual float damageShareRadius() const { return 0.0f; }
    virtual float damageShareRatio() const { return 0.0f; }

protected:
    Enemy(Type type, Role role, Archetype archetype,
          const Math::Vec2 &position, int waveNumber);

    // 具体敌人只需返回期望速度，积分、边界、闪避由基类统一处理。
    virtual Math::Vec2 desiredVelocity(const Context &ctx, float dt);
    // 每帧额外逻辑：射击计时、光环、治疗等。
    virtual void updateSpecial(const Context &ctx, float dt) { (void)ctx; (void)dt; }
    virtual void drawBody(QPainter &world, QPainter &glow, float facing) const = 0;
    virtual bool showsAura() const { return false; }

    Math::Vec2 avoidPlayerBullets(const Context &ctx) const;

    Role role_ = Role::Melee;
    Archetype archetype_ = Archetype::Offensive;

private:
    static int sNextId;
};

} // namespace arena
