#pragma once

#include "Arena/Combatants/Enemy.h"
#include "Arena/Combatants/Player.h"

#include <QColor>
#include <QPainter>

#include <memory>
#include <vector>

namespace net {
struct Snapshot;
}

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

    void spawnEnemy(const Context &ctx, EnemyType type,
                    const Math::Vec2 &pos, int wave, int enemyLevel = 1);
    // 数据/测试边界：字符串键在入口解析成 EnemyType。
    void spawnEnemy(const Context &ctx, const QString &dataKey,
                    const Math::Vec2 &pos, int wave, int enemyLevel = 1);
    void spawnMinions(const Context &ctx, const Math::Vec2 &pos, int count, int wave,
                       int enemyLevel = 1);
    void clearForVictory(const Context &ctx);
    void damageEnemiesInRadius(const Context &ctx, const Math::Vec2 &center,
                               float radius, float damage,
                               DamageSource source = DamageSource::Shock);

    int aliveEnemyCount() const;
    BossInfo bossInfo() const;

    // ----- 网络快照（阶段 2） -----
    // 提取：只写数据，不传 RTTI；类型在数据边界转成注册表的 dataKey。
    void writeSnapshot(net::Snapshot &out) const;
    // 应用：按 dataKey 重建具体敌人类型，再填充绘制字段（客户端视图不推进模拟）。
    void applySnapshot(const net::Snapshot &snapshot);

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
