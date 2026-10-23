#pragma once

#include "Arena/Fighter.h"

#include <QColor>
#include <QString>

class Arena;
class Random;

namespace arena {

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
    QString kind;
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
    float spawnAnim = 0.28f;
    bool boss = false;

    Enemy(const QString &enemyKind, const Math::Vec2 &position, int waveNumber,
          Random &random);

    void update(Arena &arena, float dt) override;
    void shoot(Arena &arena) override;
    void takeDamage(Arena &arena, const Damage &damage) override;
    void die(Arena &arena) override;
    void draw(QPainter &world, QPainter &glow, const Arena &arena) const override;

    bool isAlive() const override { return alive; }
    const Math::Vec2 &position() const override { return pos; }
    float collisionRadius() const override { return radius; }
    int id() const override { return eid; }
    float contactDamage() const override { return damage; }

    static const EnemyStats &statsFor(const QString &kind);

private:
    void fireAimed(Arena &arena, int count, float spread, float bulletSpeed);
    void drawBody(QPainter &world, QPainter &glow, float facing) const;
    void drawBoss(QPainter &world, QPainter &glow, float facing) const;

    static int sNextId;
};

} // namespace arena
