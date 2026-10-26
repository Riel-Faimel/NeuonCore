#include "Arena/Bullets/Bullets.h"

#include "Arena/Bullets/StandardBullet.h"

#include <QPainter>

namespace arena {

Bullets::Bullets()
{
    // 稳态容量：玩家弹最密时也就几十颗，先预留好，避免战斗中反复扩容。
    player_.reserve(256);
    enemy_.reserve(128);
    free_.reserve(MaxPooledBullets);
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

void Bullets::spawnStandard(const Math::Vec2 &position, const Math::Vec2 &velocity,
                            float damage, const QColor &color,
                            Bullet::Standpoint standpoint, float radius, int pierce,
                            float critChance)
{
    if (!free_.empty()) {
        std::unique_ptr<Bullet> bullet = std::move(free_.back());
        free_.pop_back();
        if (auto *standard = dynamic_cast<StandardBullet *>(bullet.get())) {
            standard->reset(position, velocity, damage, color, standpoint, radius, pierce,
                            critChance);
            submit(std::move(bullet));
            return;
        }
        // 池里只放标准弹；异常情况下丢弃并重新分配。
    }
    submit(std::make_unique<StandardBullet>(position, velocity, damage, color, standpoint,
                                            radius, pierce, critChance));
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
// 死亡的标准弹回到空闲池复用，避免每次射击都分配/释放。
void Bullets::compact(std::vector<std::unique_ptr<Bullet>> &bullets,
                      std::vector<std::unique_ptr<Bullet>> &free)
{
    size_t index = 0;
    while (index < bullets.size()) {
        if (bullets[index]->alive()) {
            ++index;
            continue;
        }
        std::unique_ptr<Bullet> dead = std::move(bullets[index]);
        if (dynamic_cast<StandardBullet *>(dead.get()) != nullptr
            && free.size() < size_t(MaxPooledBullets)) {
            free.push_back(std::move(dead));
        }
        if (index + 1 != bullets.size()) {
            bullets[index] = std::move(bullets.back());
        }
        bullets.pop_back();
    }
}

void Bullets::recycle(std::vector<std::unique_ptr<Bullet>> &bullets,
                      std::vector<std::unique_ptr<Bullet>> &free)
{
    for (auto &bullet : bullets) {
        if (bullet != nullptr && dynamic_cast<StandardBullet *>(bullet.get()) != nullptr
            && free.size() < size_t(MaxPooledBullets)) {
            free.push_back(std::move(bullet));
        }
    }
    bullets.clear();
}

void Bullets::cleanup()
{
    compact(player_, free_);
    compact(enemy_, free_);
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
    recycle(player_, free_);
    recycle(enemy_, free_);
}

void Bullets::clearEnemy()
{
    recycle(enemy_, free_);
}

} // namespace arena
