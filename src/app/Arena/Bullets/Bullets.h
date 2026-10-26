#pragma once

#include "Bullet.h"

#include <QColor>

#include <memory>
#include <vector>

class QPainter;

namespace arena {

// Bullets 组件：局内唯一的子弹池。shoot() 只负责把对象提交进来，
// 推进、回收和绘制都由池统一做；池按阵营分成两组。
// 标准弹会回到空闲池复用，稳态射击不再产生堆分配。
class Bullets {
public:
    Bullets();

    void submit(std::unique_ptr<Bullet> bullet);
    // 发射标准弹：优先复用回收池里的对象。
    void spawnStandard(const Math::Vec2 &position, const Math::Vec2 &velocity,
                       float damage, const QColor &color, Bullet::Standpoint standpoint,
                       float radius = 3.0f, int pierce = 0, float critChance = 0.0f);
    void update(float dt);
    void cleanup();
    void draw(QPainter &world, QPainter &glow) const;

    void clear();
    void clearEnemy();

    int pooledBulletCount() const { return int(free_.size()); }

    // 名字不能叫 player()/enemy()：Arena 同时继承 Combatants，
    // 而 Combatants::player() 返回 Player&，会形成二义性。
    std::vector<std::unique_ptr<Bullet>> &playerBullets() { return player_; }
    std::vector<std::unique_ptr<Bullet>> &enemyBullets() { return enemy_; }

private:
    static constexpr int MaxPooledBullets = 256;

    static void compact(std::vector<std::unique_ptr<Bullet>> &bullets,
                        std::vector<std::unique_ptr<Bullet>> &free);
    static void recycle(std::vector<std::unique_ptr<Bullet>> &bullets,
                        std::vector<std::unique_ptr<Bullet>> &free);

    std::vector<std::unique_ptr<Bullet>> player_;
    std::vector<std::unique_ptr<Bullet>> enemy_;
    std::vector<std::unique_ptr<Bullet>> free_;
};

} // namespace arena
