#pragma once

#include "Fighter.h"

#include <QColor>

namespace arena {

struct Context;

// 玩家战斗者的可变状态。Arena 对外只暴露 playerState()（Upgrades 需要写权限）。
// 一帧的玩家输入。模拟层只读这份快照，不再直接访问 BaseService；
// 单机适配器（Games）每帧从 BaseService 填充，网络客户端从 InputCommand 填充。
struct InputState {
    Math::Vec2 moveAxis;
    bool mouseActive = false;
    Math::Vec2 mousePosition;
    bool shooting = false;
    bool dashRequested = false;
};

struct PlayerState {
    // 稳定玩家 id/slot（1..4）。输入按这个 id 路由，快照也用它标识本地飞机。
    int playerId = 1;
    // 每架飞机的识别色（单机是 Base::Cyan，多玩家时按 slot 分配）。
    QColor color;

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
    // false 表示这一局已经没有这架飞机（会话离开/被移除），快照不再包含它。
    bool active = true;
    float spawnPulse = 0.7f;
    // 多玩家时的重新部署倒计时；>0 表示阵亡等待复活。
    float respawnTimer = 0.0f;
    Math::Vec2 muzzlePos;
};

// 玩家战斗者组件：通过 Fighter 接口与 Enemy 统一被调用。
class Player : public Fighter {
public:
    void reset();
    void beginWave();
    // 多玩家配置：重置数值后换上指定 id/颜色/出生点。respawn=true 时阵亡会
    // 在 respawnTimer 结束后原地复活（单机保持“阵亡即失败”）。
    void configure(int playerId, const QColor &colorValue, const Math::Vec2 &spawn,
                   bool respawn);
    // 重新部署：满血、回到出生点、短暂无敌。
    void respawn(const Context &ctx);

    void update(const Context &ctx, float dt) override;
    void shoot(const Context &ctx) override;
    void takeDamage(const Context &ctx, const Damage &damage) override;
    void die(const Context &ctx) override;
    void draw(QPainter &world, QPainter &glow, const Context &ctx) const override;

    bool isAlive() const override { return state.alive; }
    const Math::Vec2 &position() const override { return state.pos; }
    float collisionRadius() const override { return state.radius; }
    int id() const override { return state.playerId; }
    float contactDamage() const override { return 0.0f; }

    PlayerState state;
    InputState input;
    // 出生点/复活位置；多玩家时每架飞机固定，单机是屏幕中央偏下。
    Math::Vec2 spawnPos;
    bool respawnEnabled = false;
};

// 玩家 slot 的识别色与出生点：单机固定为原来的中央位置和 Cyan，
// 多玩家按 playerId (1..4) 取不同的位置/颜色。
QColor playerColor(int playerId);
Math::Vec2 playerSpawnPosition(int playerId);

} // namespace arena
