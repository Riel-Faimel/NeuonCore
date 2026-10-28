#include "Arena/Bullets/Bullets.h"

#include "Arena/Bullets/StandardBullet.h"
#include "Net/Snapshot.h"

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
                            float critChance, float homingRadius, float homingRatio)
{
    if (!free_.empty()) {
        std::unique_ptr<Bullet> bullet = std::move(free_.back());
        free_.pop_back();
        if (auto *standard = dynamic_cast<StandardBullet *>(bullet.get())) {
            standard->reset(position, velocity, damage, color, standpoint, radius, pierce,
                            critChance, homingRadius, homingRatio);
            submit(std::move(bullet));
            return;
        }
        // 池里只放标准弹；异常情况下丢弃并重新分配。
    }
    submit(std::make_unique<StandardBullet>(position, velocity, damage, color, standpoint,
                                            radius, pierce, critChance, homingRadius,
                                            homingRatio));
}

void Bullets::update(const Context &ctx, float dt)
{
    // 先吸附（只改方向），再推进；没有吸附参数的弹种 steer() 是空实现。
    for (auto &bullet : player_) {
        bullet->steer(ctx, dt);
        bullet->advance(dt);
    }
    for (auto &bullet : enemy_) {
        bullet->steer(ctx, dt);
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

void Bullets::writeSnapshot(net::Snapshot &out) const
{
    out.bullets.clear();
    out.bullets.reserve(player_.size() + enemy_.size());
    auto append = [&out](const std::vector<std::unique_ptr<Bullet>> &bullets) {
        for (const auto &bullet : bullets) {
            if (!bullet->alive()) {
                continue;
            }
            // 目前只有标准弹；新弹种需要自己的协议 key 和重建路径。
            const auto *standard = dynamic_cast<const StandardBullet *>(bullet.get());
            if (standard == nullptr) {
                continue;
            }
            net::BulletSnapshot data;
            data.pos = standard->position();
            data.prev = standard->prev();
            data.vel = standard->velocity();
            data.color = standard->color();
            data.damage = standard->damage();
            data.radius = standard->radius();
            data.side = standard->standpoint() == Bullet::Standpoint::Player
                            ? net::BulletSide::Player
                            : net::BulletSide::Enemy;
            data.alive = true;
            out.bullets.push_back(data);
        }
    };
    append(player_);
    append(enemy_);
}

void Bullets::applySnapshot(const net::Snapshot &snapshot)
{
    clear();
    for (const net::BulletSnapshot &data : snapshot.bullets) {
        spawnStandard(data.pos, data.vel, data.damage, data.color,
                      data.side == net::BulletSide::Player
                          ? Bullet::Standpoint::Player
                          : Bullet::Standpoint::Enemy,
                      data.radius);
    }
}

} // namespace arena
