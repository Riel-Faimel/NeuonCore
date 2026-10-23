#pragma once

#include "Arena/Combatants/Enemy.h"
#include "Arena/Combatants/Player.h"

#include <QColor>
#include <QPainter>

#include <memory>
#include <vector>

class Arena;

namespace arena {

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
    void updatePlayer(Arena &arena, float dt);
    void updateEnemies(Arena &arena, float dt);
    void cleanup();
    void draw(QPainter &world, QPainter &glow, const Arena &arena) const;

    void spawnEnemy(Arena &arena, const QString &kind,
                    const Math::Vec2 &pos, int wave);
    void spawnMinions(Arena &arena, const Math::Vec2 &pos, int count, int wave);
    void clearForVictory(Arena &arena);
    void damageEnemiesInRadius(Arena &arena, const Math::Vec2 &center,
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
};

} // namespace arena
