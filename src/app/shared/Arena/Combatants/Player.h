#pragma once

#include "Fighter.h"

namespace arena {

struct Context;

// 玩家战斗者的可变状态。Arena 对外只暴露 playerState()（Upgrades 需要写权限）。
// 一帧的玩家输入。模拟层只读这份快照，不再直接访问 BaseService；
// 单机适配器（Games）每帧从 BaseService 填充，网络客户端将来从 InputCommand 填充。
struct InputState {
    Math::Vec2 moveAxis;
    bool mouseActive = false;
    Math::Vec2 mousePosition;
    bool shooting = false;
    bool dashRequested = false;
};

struct PlayerState {
    Math::Vec2 pos;
    Math::Vec2 vel;
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
    Math::Vec2 dashDir{0.0f, -1.0f};
    float invuln = 0.0f;
    float fireFlash = 0.0f;
    float aim = -Math::Pi / 2.0f;
    bool alive = true;
    float spawnPulse = 0.7f;
    Math::Vec2 muzzlePos;
};

// 玩家战斗者组件：通过 Fighter 接口与 Enemy 统一被调用。
class Player : public Fighter {
public:
    void reset();
    void beginWave();

    void update(const Context &ctx, float dt) override;
    void shoot(const Context &ctx) override;
    void takeDamage(const Context &ctx, const Damage &damage) override;
    void die(const Context &ctx) override;
    void draw(QPainter &world, QPainter &glow, const Context &ctx) const override;

    bool isAlive() const override { return state.alive; }
    const Math::Vec2 &position() const override { return state.pos; }
    float collisionRadius() const override { return state.radius; }
    int id() const override { return -1; }
    float contactDamage() const override { return 0.0f; }

    PlayerState state;
    InputState input;
};

} // namespace arena
