#include "Arena/Bullets/Bullets.h"

#include <QPainter>

namespace arena {

Bullets::Bullets()
{
    // 稳态容量：玩家弹最密时也就几十颗，先预留好，避免战斗中反复扩容。
    player_.reserve(256);
    enemy_.reserve(128);
}

void Bullets::submit(std::unique_ptr<Bullet> bullet)
{
    if (!bullet) {
        return;
    }
    if (bullet->standpoint() == Bullet::Standpoint::Player) {
        player_.push_back(std::move(bullet));
    } else {
        enemy_.push_back(std::move(bullet));
    }
}

void Bullets::update(float dt)
{
    for (auto &bullet : player_) {
        bullet->advance(dt);
    }
    for (auto &bullet : enemy_) {
        bullet->advance(dt);
    }
}

// 交换删除：回收不搬移整段数组，也不要求保持顺序（子弹绘制顺序无所谓）。
void Bullets::compact(std::vector<std::unique_ptr<Bullet>> &bullets)
{
    size_t index = 0;
    while (index < bullets.size()) {
        if (bullets[index]->alive()) {
            ++index;
            continue;
        }
        if (index + 1 != bullets.size()) {
            bullets[index] = std::move(bullets.back());
        }
        bullets.pop_back();
    }
}

void Bullets::cleanup()
{
    compact(player_);
    compact(enemy_);
}

void Bullets::draw(QPainter &world, QPainter &glow) const
{
    for (const auto &bullet : player_) {
        bullet->draw(world, glow);
    }
    for (const auto &bullet : enemy_) {
        bullet->draw(world, glow);
    }
}

void Bullets::clear()
{
    player_.clear();
    enemy_.clear();
}

void Bullets::clearEnemy()
{
    enemy_.clear();
}

} // namespace arena
