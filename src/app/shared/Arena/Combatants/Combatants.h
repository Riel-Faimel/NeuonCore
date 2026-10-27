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

// 战斗者组件：玩家飞机（1..4 架）、敌人容器和 Boss 指针。
// 玩家以 vector 保存：player() 仍是第一架（单机路径与旧代码），
// 多玩家时通过 id/slot 定位自己的飞机。
class Combatants {
public:
    void reset();
    // 按服务端分配的 playerId 列表重建玩家（单机传入 {1}）。
    // 只有一架时保持原来的中央出生点/Cyan；多架时按 slot 使用不同位置/颜色。
    void configurePlayers(const std::vector<int> &playerIds);
    void beginWave();
    void updatePlayers(const Context &ctx, float dt);
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

    // ----- 多玩家查询 -----
    int playerCount() const;                 // active 玩家数量
    int livingPlayerCount() const;           // active && alive
    bool hasPlayer(int playerId) const;
    // 没有玩家飞机时仍返回 players_.front()（Arena 至少保留一架占位机）。
    Player &playerById(int playerId);
    const Player &playerById(int playerId) const;
    Player &playerAt(int index);
    const Player &playerAt(int index) const;
    // 敌人/掉落等的目标选择：最近的存活玩家；没有存活玩家时退回第一架。
    const Player &nearestLivingPlayer(const Math::Vec2 &from) const;
    // 本机玩家（客户端视图用它决定 HUD/瞄准线）；localPlayerId<=0 表示
    // 没有指定（单机/观战）→ 第一架飞机。
    bool isLocalPlayer(int playerId) const;
    int localPlayerId() const { return localPlayerId_; }
    void setLocalPlayerId(int playerId) { localPlayerId_ = playerId; }
    // 移除会话对应的飞机：标记 inactive（不 erase，player() 永远有效），
    // 之后快照不再包含它，敌人也不会再把它当作目标。
    bool removePlayer(int playerId);

    // ----- 网络快照（阶段 2） -----
    // 提取：只写数据，不传 RTTI；类型在数据边界转成注册表的 dataKey。
    void writeSnapshot(net::Snapshot &out) const;
    // 应用：按 dataKey 重建具体敌人类型，再填充绘制字段（客户端视图不推进模拟）。
    void applySnapshot(const net::Snapshot &snapshot);

    Player &player() { return players_.front(); }
    const Player &player() const { return players_.front(); }
    std::vector<Player> &players() { return players_; }
    const std::vector<Player> &players() const { return players_; }
    std::vector<std::unique_ptr<Enemy>> &enemies() { return enemies_; }
    const std::vector<std::unique_ptr<Enemy>> &enemies() const { return enemies_; }
    Enemy *boss() { return boss_; }
    const Enemy *boss() const { return boss_; }

private:
    std::vector<Player> players_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    Enemy *boss_ = nullptr;
    // 客户端视图的本地玩家 id；模拟端保持 0（无本地视角）。
    int localPlayerId_ = 0;
};

} // namespace arena
