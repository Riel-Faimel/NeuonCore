#pragma once

#include "Arena/Combatants/Enemy.h"
#include "Arena/Combatants/EnemyGrid.h"
#include "Arena/Combatants/Player.h"

#include <QColor>
#include <QPainter>

#include <memory>
#include <utility>
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

    // ----- 空间网格 -----
    // 敌人位置被夹在场内，网格覆盖整个战斗区域；Arena 在“子弹结算前”和
    // “敌人移动后”各刷新一次（O(n)），之后的范围查询只碰邻近格子。
    void refreshEnemyGrid();
    // 刷新每个敌人的“150 半径内友军数”（辅助型的密集点选择只查缓存）。
    void refreshEnemyDensity();

    // ----- mini 对象池 -----
    // 召唤物数量大、生命周期短（一次召唤好几只、被打死就消失），如果每次 new/delete
    // 会造成明显的分配抖动。这里把死掉的 mini 收回池里，下次召唤/复活时原地重建，
    // 复用同一块内存（`std::destroy_at` + `std::construct_at`，不分配不释放）。
    // 池只存 mini；其他敌人仍然走普通分配。
    static constexpr int MiniPoolLimit = 1024;
    // 取一个 mini：优先复用池里的对象，池空时新建。返回的指针归调用方所有。
    std::unique_ptr<Enemy> acquireMini(const Math::Vec2 &position, int wave);
    // 把所有已死亡的 mini 收回池（其余死亡敌人照常释放）。
    void recycleEnemies();
    // 池里空闲的 mini 数量（测试/基准用）。
    int pooledMiniCount() const { return int(miniPool_.size()); }

    // ----- 绘制 LOD -----
    // 策略：mini 这类“数量多、体积小”的敌人始终简化（见 Enemy::prefersLowDetail）；
    // 其他敌人尽量保留完整机身，只在人群很大时才逐步降级：
    //   > CrowdLimit         关闭敌人辉光（辉光是逐像素填充，最贵，机身细节保留）
    //   > CrowdExtremeLimit  连机身也简化 + 关抗锯齿（保命档，避免上千单位时卡死）
    static constexpr int CrowdLimit = 420;
    static constexpr int CrowdExtremeLimit = 700;
    const EnemyGrid &enemyGrid() const { return grid_; }
    int enemyCount() const { return int(enemies_.size()); }
    template <typename Fn>
    void forEachEnemyNear(const Math::Vec2 &pos, float radius, Fn &&fn) const
    {
        grid_.forEachNear(pos, radius, std::forward<Fn>(fn));
    }

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
    // mini 的空闲对象池（见 acquireMini/recycleEnemies）。
    std::vector<std::unique_ptr<Enemy>> miniPool_;
    EnemyGrid grid_;
    Enemy *boss_ = nullptr;
    // 客户端视图的本地玩家 id；模拟端保持 0（无本地视角）。
    int localPlayerId_ = 0;
};

} // namespace arena
