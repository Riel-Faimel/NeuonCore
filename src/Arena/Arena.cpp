#include "Arena/Arena.h"

#include "GameConfig.h"
#include "Random.h"
#include "base/Base.h"

#include <QPolygonF>

#include <algorithm>
#include <array>
#include <cmath>

Arena::Arena(Random &random, Base &base) : random_(random), base_(base)
{
    reset();
}

Arena::~Arena() = default;

void Arena::reset()
{
    player_.reset();
    enemies_.clear();
    playerBullets_.clear();
    enemyBullets_.clear();
    portals_.clear();
    pickups_.clear();
    particles_.clear();
    floatingTexts_.clear();
    shockwaves_.clear();
    boss_ = nullptr;
    score_.reset();
    playerDefeated_ = false;
    bossState_.reset();
    effectsTimer_ = 0.0f;
    feedback_ = Feedback{};
}

void Arena::beginWave()
{
    playerBullets_.clear();
    enemyBullets_.clear();
    player_.beginWave();
    boss_ = nullptr;
    bossState_.reset();
    spawnBurst(player_.state.pos, Base::Cyan, 14, 90.0f, 0.5f);
}

void Arena::update(float dt)
{
    if (!player_.state.alive) {
        updateEffects(dt);
        return;
    }

    player_.update(*this, dt);
    if (playerDefeated_ || bossState_.defeated()) {
        updateEffects(dt);
        return;
    }

    updatePortals(dt);
    collision_.resolvePlayerBullets(*this, dt);
    if (playerDefeated_ || bossState_.defeated()) {
        updateEffects(dt);
        return;
    }

    collision_.resolveEnemyBullets(*this, dt);
    if (playerDefeated_) {
        updateEffects(dt);
        return;
    }

    updateEnemies(dt);
    collision_.separateEnemies(*this);
    collision_.contactDamage(*this);
    if (playerDefeated_) {
        updateEffects(dt);
        return;
    }

    updatePickups(dt);
    cleanup();
    updateEffects(dt);
}

void Arena::draw(QPainter &world, QPainter &glow) const
{
    drawBorder(world);
    for (const arena::Portal &portal : portals_) {
        portal.draw(world, glow);
    }
    for (const arena::Pickup &pickup : pickups_) {
        pickup.draw(world, glow);
    }
    for (const arena::Particle &particle : particles_) {
        if (particle.layer == 0) {
            particle.draw(world, glow);
        }
    }
    for (const arena::Shockwave &wave : shockwaves_) {
        wave.draw(world, glow);
    }
    for (const arena::Bullet &bullet : playerBullets_) {
        bullet.draw(world, glow);
    }
    for (const arena::Bullet &bullet : enemyBullets_) {
        bullet.draw(world, glow);
    }

    std::vector<const arena::Fighter *> sorted;
    sorted.reserve(enemies_.size());
    for (const auto &enemy : enemies_) {
        if (enemy->alive) {
            sorted.push_back(enemy.get());
        }
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const arena::Fighter *a, const arena::Fighter *b) {
                  return a->position().y < b->position().y;
              });
    for (const arena::Fighter *fighter : sorted) {
        fighter->draw(world, glow, *this);
    }
    player_.draw(world, glow, *this);

    for (const arena::Particle &particle : particles_) {
        if (particle.layer == 1) {
            particle.draw(world, glow);
        }
    }
    for (const arena::FloatingText &text : floatingTexts_) {
        text.draw(world);
    }
}

void Arena::spawnPortal(const QString &kind, int wave)
{
    const Math::Vec2 position =
        kind == QStringLiteral("boss")
            ? Math::Vec2(GameConfig::ScreenWidth / 2.0f, 190.0f)
            : randomSpawnPosition();
    portals_.emplace_back(kind, position, wave);
    spawnBurst(position, arena::Enemy::statsFor(kind).color, 5, 50.0f, 0.35f);
}

int Arena::aliveEnemyCount() const
{
    int count = 0;
    for (const auto &enemy : enemies_) {
        if (enemy->alive) {
            ++count;
        }
    }
    return count;
}

int Arena::portalCount() const
{
    return int(portals_.size());
}

Arena::BossInfo Arena::bossInfo() const
{
    BossInfo info;
    if (boss_ != nullptr && boss_->alive) {
        info.active = true;
        info.hp = boss_->hp;
        info.maxHp = boss_->maxHp;
        info.hpDisplay = boss_->hpDisplay;
    }
    return info;
}

void Arena::updateVisuals(float dt)
{
    updateEffects(dt);
}

void Arena::clearEnemyBullets()
{
    enemyBullets_.clear();
}

void Arena::clearProjectiles()
{
    playerBullets_.clear();
    enemyBullets_.clear();
}

void Arena::clearForVictory()
{
    enemyBullets_.clear();
    for (const auto &enemy : enemies_) {
        if (enemy->alive && !enemy->boss) {
            enemy->alive = false;
            spawnBurst(enemy->pos, enemy->color, 8, 130.0f, 0.45f);
        }
    }
    std::erase_if(enemies_, [](const std::unique_ptr<arena::Enemy> &enemy) {
        return !enemy->boss;
    });
    boss_ = nullptr;
    player_.state.invuln = 3.0f;
}

void Arena::requestDash()
{
    player_.state.dashBuffer = 0.16f;
}

void Arena::celebrateUpgrade()
{
    spawnBurst(player_.state.pos, Base::Cyan, 26, 190.0f, 0.6f);
    addShockwave(player_.state.pos, 110.0f, Base::Cyan, 0.55f, 2);
}

void Arena::celebrateVictory()
{
    addShockwave(player_.state.pos, 150.0f, Base::Cyan, 0.7f, 2);
    addShockwave(player_.state.pos, 250.0f, Base::Cyan, 0.9f, 2);
    addShockwave(player_.state.pos, 380.0f, Base::Cyan, 1.1f, 2);
}

Arena::Feedback Arena::takeFeedback()
{
    Feedback result = feedback_;
    feedback_ = Feedback{};
    return result;
}

void Arena::updatePortals(float dt)
{
    for (arena::Portal &portal : portals_) {
        portal.update(*this, dt);
    }
    std::erase_if(portals_, [](const arena::Portal &portal) {
        return !portal.alive;
    });
}

void Arena::updateEnemies(float dt)
{
    const size_t count = enemies_.size();
    for (size_t i = 0; i < count; ++i) {
        if (i < enemies_.size()) {
            arena::Fighter &fighter = *enemies_[i];
            if (fighter.isAlive()) {
                fighter.update(*this, dt);
            }
        }
    }
}

void Arena::updatePickups(float dt)
{
    for (arena::Pickup &pickup : pickups_) {
        pickup.update(*this, dt);
    }
    std::erase_if(pickups_, [](const arena::Pickup &pickup) {
        return !pickup.alive;
    });
}

void Arena::updateEffects(float dt)
{
    effectsTimer_ = std::max(0.0f, effectsTimer_ - dt);
    player_.state.hpDisplay +=
        (player_.state.hp - player_.state.hpDisplay) * std::min(1.0f, dt * 3.0f);

    for (arena::Particle &particle : particles_) {
        particle.update(dt);
    }
    std::erase_if(particles_, [](const arena::Particle &particle) {
        return particle.life <= 0.0f;
    });
    if (particles_.size() > 850) {
        particles_.erase(particles_.begin(),
                         particles_.begin() + (particles_.size() - 850));
    }

    for (arena::FloatingText &text : floatingTexts_) {
        text.update(dt);
    }
    std::erase_if(floatingTexts_, [](const arena::FloatingText &text) {
        return text.life <= 0.0f;
    });

    for (arena::Shockwave &wave : shockwaves_) {
        wave.update(dt);
    }
    std::erase_if(shockwaves_, [](const arena::Shockwave &wave) {
        return wave.life <= 0.0f;
    });
}

void Arena::cleanup()
{
    if (boss_ != nullptr && !boss_->alive) {
        boss_ = nullptr;
    }
    std::erase_if(enemies_, [](const std::unique_ptr<arena::Enemy> &enemy) {
        return !enemy->alive;
    });
    std::erase_if(playerBullets_, [](const arena::Bullet &bullet) {
        return !bullet.alive;
    });
    std::erase_if(enemyBullets_, [](const arena::Bullet &bullet) {
        return !bullet.alive;
    });
    std::erase_if(portals_, [](const arena::Portal &portal) {
        return !portal.alive;
    });
    std::erase_if(pickups_, [](const arena::Pickup &pickup) {
        return !pickup.alive;
    });
}

void Arena::spawnEnemy(const QString &kind, const Math::Vec2 &pos, int wave)
{
    auto enemy = std::make_unique<arena::Enemy>(kind, pos, wave, random_);
    arena::Enemy *ptr = enemy.get();
    enemies_.push_back(std::move(enemy));
    if (kind == QStringLiteral("boss")) {
        boss_ = ptr;
        base_.playSound(QStringLiteral("boss"));
        addFeedback(Base::Magenta, 45, 10.0f);
    } else {
        spawnBurst(pos, ptr->color, 8, 120.0f, 0.4f);
    }
}

void Arena::spawnMinions(const Math::Vec2 &pos, int count, int wave)
{
    for (int i = 0; i < count; ++i) {
        const float angle = random_.real(0.0f, Math::Tau);
        const Math::Vec2 offset = Math::Vec2(std::cos(angle), std::sin(angle))
            * random_.real(60.0f, 90.0f);
        auto child = std::make_unique<arena::Enemy>(QStringLiteral("mini"),
                                                    pos + offset, wave, random_);
        spawnBurst(child->pos, child->color, 6, 110.0f, 0.35f);
        enemies_.push_back(std::move(child));
    }
}

void Arena::spawnPickup(const Math::Vec2 &pos, int amount)
{
    arena::Pickup pickup(pos, amount);
    pickup.vel = Math::Vec2(random_.real(-25.0f, 25.0f),
                            random_.real(-25.0f, 25.0f));
    pickup.phase = random_.real(0.0f, Math::Tau);
    pickups_.push_back(pickup);
}

void Arena::spawnBurst(const Math::Vec2 &pos, const QColor &color, int count,
                       float speed, float life)
{
    for (int i = 0; i < count; ++i) {
        const float angle = random_.real(0.0f, Math::Tau);
        const Math::Vec2 velocity = Math::Vec2(std::cos(angle), std::sin(angle))
            * random_.real(speed * 0.18f, speed);
        Math::Vec2 origin = pos;
        if (velocity.lengthSquared() > 0.0f) {
            origin += velocity.normalized() * random_.real(0.0f, 10.0f);
        }
        particles_.push_back(arena::Particle{
            origin, velocity, random_.real(life * 0.45f, life), life,
            random_.real(1.0f, 4.2f), color, random_.real(1.4f, 3.6f),
            random_.real(-8.0f, 8.0f), random_.integer(0, 2) == 2 ? 1 : 0});
    }
}

void Arena::addShockwave(const Math::Vec2 &pos, float radius, const QColor &color,
                         float duration, int width)
{
    shockwaves_.emplace_back(pos, radius, color, duration, width);
}

void Arena::damageEnemiesInRadius(const Math::Vec2 &center, float radius,
                                  float damage, arena::DamageSource source)
{
    const size_t count = enemies_.size();
    for (size_t i = 0; i < count; ++i) {
        if (i >= enemies_.size()) {
            break;
        }
        arena::Enemy *enemy = enemies_[i].get();
        if (!enemy->alive) {
            continue;
        }
        if (center.distanceTo(enemy->pos) <= radius + enemy->radius) {
            const Math::Vec2 direction = enemy->pos - center;
            if (direction.lengthSquared() > 0.01f && !enemy->boss) {
                enemy->pos += direction.normalized() * 32.0f;
            }
            enemy->takeDamage(*this, arena::Damage{damage, false, source});
            if (bossState_.defeated()) {
                break;
            }
        }
    }
}

Math::Vec2 Arena::randomSpawnPosition()
{
    Math::Vec2 candidate(60.0f, 60.0f);
    for (int attempt = 0; attempt < 20; ++attempt) {
        const int edge = random_.integer(0, 3);
        if (edge == 0) {
            candidate = {random_.real(GameConfig::FieldLeft + 40.0f,
                                      GameConfig::FieldRight - 40.0f),
                         GameConfig::FieldTop + 18.0f};
        } else if (edge == 1) {
            candidate = {GameConfig::FieldRight - 18.0f,
                         random_.real(GameConfig::FieldTop + 40.0f,
                                      GameConfig::FieldBottom - 30.0f)};
        } else if (edge == 2) {
            candidate = {random_.real(GameConfig::FieldLeft + 40.0f,
                                      GameConfig::FieldRight - 40.0f),
                         GameConfig::FieldBottom - 18.0f};
        } else {
            candidate = {GameConfig::FieldLeft + 18.0f,
                         random_.real(GameConfig::FieldTop + 40.0f,
                                      GameConfig::FieldBottom - 30.0f)};
        }
        if (candidate.distanceTo(player_.state.pos) > 235.0f) {
            break;
        }
    }
    return candidate;
}

void Arena::addFeedback(const QColor &color, int alpha, float shake)
{
    if (alpha > 0) {
        feedback_.hasFlash = true;
        feedback_.color = color;
        feedback_.alpha = std::max(feedback_.alpha, alpha);
    }
    feedback_.shake = std::max(feedback_.shake, shake);
}

void Arena::drawBorder(QPainter &world) const
{
    const QRectF border(15, 15, GameConfig::ScreenWidth - 30.0f,
                        GameConfig::ScreenHeight - 30.0f);
    world.save();
    QPen pen(QColor(29, 65, 84));
    pen.setWidthF(1.0f);
    world.setPen(pen);
    world.setBrush(Qt::NoBrush);
    world.drawRect(border);
    const float length = 42.0f;
    const QColor color(48, 115, 140);
    const std::array<std::array<float, 4>, 4> corners = {{
        {15, 15, 1, 1},
        {GameConfig::ScreenWidth - 15.0f, 15, -1, 1},
        {15, GameConfig::ScreenHeight - 15.0f, 1, -1},
        {GameConfig::ScreenWidth - 15.0f, GameConfig::ScreenHeight - 15.0f, -1, -1},
    }};
    for (const auto &corner : corners) {
        Base::strokeLine(world, {corner[0], corner[1]},
                         {corner[0] + corner[2] * length, corner[1]}, color, 2.0f);
        Base::strokeLine(world, {corner[0], corner[1]},
                         {corner[0], corner[1] + corner[3] * length}, color, 2.0f);
    }
    for (int x = 80; x < GameConfig::ScreenWidth - 60; x += 160) {
        Base::strokeLine(world, {float(x), 16}, {float(x), 22},
                         QColor(25, 51, 67), 1.0f);
        Base::strokeLine(world, {float(x), GameConfig::ScreenHeight - 22.0f},
                         {float(x), GameConfig::ScreenHeight - 16.0f},
                         QColor(25, 51, 67), 1.0f);
    }
    world.restore();
}
