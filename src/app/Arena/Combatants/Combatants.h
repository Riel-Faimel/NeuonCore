#pragma once

#include "Arena/Combatants/Enemy.h"
#include "Arena/Combatants/Player.h"

#include <QColor>
#include <QPainter>

#include <memory>
#include <vector>

namespace arena {

struct Context;

struct BossInfo {
    bool active = false;
    float hp = 0.0f;
    float maxHp = 0.0f;
    float hpDisplay = 0.0f;
};

// 战斗者组件：玩家、敌人容器和 Boss 指针。
class Combatants {
public:
    void reset();
    void beginWave();
    void updatePlayer(const Context &ctx, float dt);
    void updateEnemies(const Context &ctx, float dt);
    void cleanup();
    void draw(QPainter &world, QPainter &glow, const Context &ctx) const;

    void spawnEnemy(const Context &ctx, const QString &kind,
                    const Math::Vec2 &pos, int wave);
    void spawnMinions(const Context &ctx, const Math::Vec2 &pos, int count, int wave);
    void clearForVictory(const Context &ctx);
    void damageEnemiesInRadius(const Context &ctx, const Math::Vec2 &center,
                               float radius, float damage,
                               DamageSource source = DamageSource::Shock);

    int aliveEnemyCount() const;
    BossInfo bossInfo() const;

    Player &player() { return player_; }
    const Player &player() const { return player_; }
    std::vector<std::unique_ptr<Enemy>> &enemies() { return enemies_; }
    const std::vector<std::unique_ptr<Enemy>> &enemies() const { return enemies_; }
    Enemy *boss() { return boss_; }
    const Enemy *boss() const { return boss_; }

private:
    Player player_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    Enemy *boss_ = nullptr;
    // 绘制排序用的暂存缓冲：每帧复用，避免反复分配。
    mutable std::vector<const Fighter *> drawOrder_;
};

} // namespace arena
