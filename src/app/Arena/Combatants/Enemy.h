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
        Other,
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
    float contactDamage() const override { return damage; }

    static const EnemyStats &statsFor(const QString &kind);

private:
    static Kind kindOf(const QString &kind);

    void fireAimed(const Context &ctx, int count, float spread);
    void drawBody(QPainter &world, QPainter &glow, float facing) const;
    void drawBoss(QPainter &world, QPainter &glow, float facing) const;

    static int sNextId;
};

} // namespace arena
