#pragma once

#include "Bullet.h"

#include <memory>
#include <vector>

class QPainter;

namespace arena {

// Bullets 组件：局内唯一的子弹池。shoot() 只负责把对象提交进来，
// 推进、回收和绘制都由池统一做；池按阵营分成两组。
class Bullets {
public:
    Bullets();

    void submit(std::unique_ptr<Bullet> bullet);
    void update(float dt);
    void cleanup();
    void draw(QPainter &world, QPainter &glow) const;

    void clear();
    void clearEnemy();

    int playerCount() const { return int(player_.size()); }
    int enemyCount() const { return int(enemy_.size()); }

    std::vector<std::unique_ptr<Bullet>> &player() { return player_; }
    std::vector<std::unique_ptr<Bullet>> &enemy() { return enemy_; }

private:
    static void compact(std::vector<std::unique_ptr<Bullet>> &bullets);

    std::vector<std::unique_ptr<Bullet>> player_;
    std::vector<std::unique_ptr<Bullet>> enemy_;
};

} // namespace arena
