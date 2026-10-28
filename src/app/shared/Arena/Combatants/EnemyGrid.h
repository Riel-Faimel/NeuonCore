#pragma once

#include "Arena/Combatants/Enemy.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace arena {

// 敌人空间网格：按固定格子把场内敌人分箱，供分离、碰撞、光环/治疗/复活等
// 范围查询使用，避免每次都遍历全部敌人（那是 O(n²) 的来源）。
//
// 网格覆盖整个战斗区域（敌人位置总被夹在场内），所以不需要动态扩容；
// 每帧重建一次是 O(n)，查询只碰邻近的 1–4 个格子。
// 网格里保存的是 Enemy*：调用方必须保证查询期间敌人容器没有被增删
// （本帧新生成的敌人下一帧才进网格，已死的敌人用 isAlive() 过滤）。
class EnemyGrid {
public:
    static constexpr float CellSize = 64.0f;
    static constexpr int Columns = 20;   // 20 * 64 = 1280，覆盖整个场地宽度
    static constexpr int Rows = 12;      // 12 * 64 = 768，覆盖整个场地高度
    static constexpr int CellCount = Columns * Rows;

    void rebuild(const std::vector<std::unique_ptr<Enemy>> &enemies)
    {
        for (std::vector<Enemy *> &cell : cells_) {
            cell.clear();
        }
        maxRadius_ = 0.0f;
        for (const auto &owned : enemies) {
            Enemy *enemy = owned.get();
            if (enemy == nullptr || !enemy->isAlive()) {
                continue;
            }
            cells_[size_t(cellIndex(enemy->pos))].push_back(enemy);
            maxRadius_ = std::max(maxRadius_, enemy->radius);
        }
    }

    void clear()
    {
        for (std::vector<Enemy *> &cell : cells_) {
            cell.clear();
        }
        maxRadius_ = 0.0f;
    }

    // 遍历与 (pos, radius) 包围盒相交的格子里的敌人；不保证顺序，也可能重复
    // （同一敌人只在一个格子里，所以不会重复）。fn 返回 false 提前结束。
    template <typename Fn>
    void forEachNear(const Math::Vec2 &pos, float radius, Fn &&fn) const
    {
        const int left = cellX(pos.x - radius);
        const int right = cellX(pos.x + radius);
        const int top = cellY(pos.y - radius);
        const int bottom = cellY(pos.y + radius);
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) {
                for (Enemy *enemy : cells_[size_t(y * Columns + x)]) {
                    if (!fn(*enemy)) {
                        return;
                    }
                }
            }
        }
    }

    // 本帧所有敌人里最大的碰撞半径：查询“任何可能相交的敌人”时加上它。
    float maxRadius() const { return maxRadius_; }

    int cellCount() const
    {
        int count = 0;
        for (const std::vector<Enemy *> &cell : cells_) {
            count += int(cell.size());
        }
        return count;
    }

private:
    static int clampX(int x) { return std::clamp(x, 0, Columns - 1); }
    static int clampY(int y) { return std::clamp(y, 0, Rows - 1); }
    static int cellX(float x) { return clampX(int(x / CellSize)); }
    static int cellY(float y) { return clampY(int(y / CellSize)); }
    static int cellIndex(const Math::Vec2 &pos)
    {
        return cellY(pos.y) * Columns + cellX(pos.x);
    }

    std::array<std::vector<Enemy *>, size_t(CellCount)> cells_;
    float maxRadius_ = 0.0f;
};

} // namespace arena
