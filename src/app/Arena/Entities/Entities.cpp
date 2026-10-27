#include "Entities.h"

#include "Arena/Context.h"
#include "Arena/Host.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemy.h"
#include "Data/GameData.h"
#include "GameConfig.h"
#include "Net/Snapshot.h"
#include "Random.h"
#include "Base/Base.h"

#include <algorithm>
#include <cmath>

namespace arena {

void Entities::reset()
{
    portals_.clear();
    pickups_.clear();
    mines_.clear();
    particles_.clear();
    floatingTexts_.clear();
    shockwaves_.clear();
    effectsTimer_ = 0.0f;
}

void Entities::updatePortals(const Context &ctx, float dt)
{
    for (Portal &portal : portals_) {
        portal.update(ctx, dt);
    }
    std::erase_if(portals_, [](const Portal &portal) {
        return !portal.alive;
    });
}

void Entities::updatePickups(const Context &ctx, float dt)
{
    for (Pickup &pickup : pickups_) {
        pickup.update(ctx, dt);
    }
    std::erase_if(pickups_, [](const Pickup &pickup) {
        return !pickup.alive;
    });
}

void Entities::updateEffects(const Context &ctx, float dt)
{
    effectsTimer_ = std::max(0.0f, effectsTimer_ - dt);
    Player &player = ctx.combatants.player();
    player.state.hpDisplay +=
        (player.state.hp - player.state.hpDisplay) * std::min(1.0f, dt * 3.0f);

    for (Particle &particle : particles_) {
        particle.update(dt);
    }
    std::erase_if(particles_, [](const Particle &particle) {
        return particle.life <= 0.0f;
    });
    if (particles_.size() > 850) {
        particles_.erase(particles_.begin(),
                         particles_.begin() + (particles_.size() - 850));
    }

    for (FloatingText &text : floatingTexts_) {
        text.update(dt);
    }
    std::erase_if(floatingTexts_, [](const FloatingText &text) {
        return text.life <= 0.0f;
    });

    for (Shockwave &wave : shockwaves_) {
        wave.update(dt);
    }
    std::erase_if(shockwaves_, [](const Shockwave &wave) {
        return wave.life <= 0.0f;
    });
}

void Entities::cleanup()
{
    std::erase_if(portals_, [](const Portal &portal) {
        return !portal.alive;
    });
    std::erase_if(pickups_, [](const Pickup &pickup) {
        return !pickup.alive;
    });
    std::erase_if(mines_, [](const Mine &mine) {
        return !mine.alive;
    });
}

void Entities::updateMines(float dt)
{
    for (Mine &mine : mines_) {
        mine.update(dt);
    }
}

void Entities::clearMines()
{
    mines_.clear();
}

void Entities::drawBackground(QPainter &world, QPainter &glow) const
{
    for (const Portal &portal : portals_) {
        portal.draw(world, glow);
    }
    for (const Mine &mine : mines_) {
        mine.draw(world, glow);
    }
    for (const Pickup &pickup : pickups_) {
        pickup.draw(world, glow);
    }
    for (const Particle &particle : particles_) {
        if (particle.layer == 0) {
            particle.draw(world, glow);
        }
    }
    for (const Shockwave &wave : shockwaves_) {
        wave.draw(world, glow);
    }
}

void Entities::drawForeground(QPainter &world, QPainter &glow) const
{
    for (const Particle &particle : particles_) {
        if (particle.layer == 1) {
            particle.draw(world, glow);
        }
    }
}

void Entities::drawTexts(QPainter &world) const
{
    for (const FloatingText &text : floatingTexts_) {
        text.draw(world);
    }
}

void Entities::spawnPortal(const Context &ctx, EnemyType type, int wave,
                              int enemyLevel)
{
    const Math::Vec2 position =
        Enemy::isBoss(type)
            ? Math::Vec2(GameConfig::ScreenWidth / 2.0f, 190.0f)
            : randomSpawnPosition(ctx);
    portals_.emplace_back(type, position, wave, enemyLevel);
    spawnBurst(ctx, position, Enemy::statsFor(type).color, 5, 50.0f, 0.35f);
}

void Entities::spawnPickup(const Context &ctx, const Math::Vec2 &pos, int amount)
{
    Pickup pickup(pos, amount);
    pickup.vel = Math::Vec2(RandomService.real(-25.0f, 25.0f),
                            RandomService.real(-25.0f, 25.0f));
    pickup.phase = RandomService.real(0.0f, Math::Tau);
    pickups_.push_back(pickup);
}

void Entities::spawnMine(const Context &ctx, const Math::Vec2 &pos)
{
    Mine mine;
    mine.pos = pos;
    mine.triggerRadius = data::kMine.triggerRadius;
    mine.blastRadius = data::kMine.blastRadius;
    mine.damage = data::kMine.damage;
    mine.armTime = data::kMine.armTime;
    mine.life = data::kMine.lifetime;
    mines_.push_back(mine);
    spawnBurst(ctx, pos, Base::Amber, 8, 55.0f, 0.3f);
    BaseService.playSound(QStringLiteral("ui"));
}

void Entities::spawnBurst(const Context &ctx, const Math::Vec2 &pos,
                          const QColor &color, int count, float speed, float life)
{
    for (int i = 0; i < count; ++i) {
        const float angle = RandomService.real(0.0f, Math::Tau);
        const Math::Vec2 velocity = Math::Vec2(std::cos(angle), std::sin(angle))
            * RandomService.real(speed * 0.18f, speed);
        Math::Vec2 origin = pos;
        if (velocity.lengthSquared() > 0.0f) {
            origin += velocity.normalized() * RandomService.real(0.0f, 10.0f);
        }
        particles_.push_back(Particle{
            origin, velocity, RandomService.real(life * 0.45f, life), life,
            RandomService.real(1.0f, 4.2f), color, RandomService.real(1.4f, 3.6f),
            RandomService.real(-8.0f, 8.0f), RandomService.integer(0, 2) == 2 ? 1 : 0});
    }
}

void Entities::addShockwave(const Math::Vec2 &pos, float radius, const QColor &color,
                            float duration, int width)
{
    shockwaves_.emplace_back(pos, radius, color, duration, width);
}

void Entities::writeSnapshot(net::Snapshot &out) const
{
    out.portals.clear();
    out.portals.reserve(portals_.size());
    for (const Portal &portal : portals_) {
        if (!portal.alive) {
            continue;
        }
        net::PortalSnapshot data;
        data.dataKey = Enemy::dataKeyFor(portal.type);
        data.boss = portal.boss;
        data.pos = portal.pos;
        data.wave = portal.wave;
        data.enemyLevel = portal.enemyLevel;
        data.color = portal.color;
        data.duration = portal.duration;
        data.timer = portal.timer;
        data.radius = portal.radius;
        data.alive = portal.alive;
        out.portals.push_back(std::move(data));
    }

    out.pickups.clear();
    out.pickups.reserve(pickups_.size());
    for (const Pickup &pickup : pickups_) {
        if (!pickup.alive) {
            continue;
        }
        net::PickupSnapshot data;
        data.pos = pickup.pos;
        data.vel = pickup.vel;
        data.amount = pickup.amount;
        data.age = pickup.age;
        data.life = pickup.life;
        data.phase = pickup.phase;
        data.radius = pickup.radius;
        data.alive = pickup.alive;
        out.pickups.push_back(data);
    }

    out.mines.clear();
    out.mines.reserve(mines_.size());
    for (const Mine &mine : mines_) {
        if (!mine.alive) {
            continue;
        }
        net::MineSnapshot data;
        data.pos = mine.pos;
        data.radius = mine.radius;
        data.triggerRadius = mine.triggerRadius;
        data.blastRadius = mine.blastRadius;
        data.damage = mine.damage;
        data.armTime = mine.armTime;
        data.life = mine.life;
        data.age = mine.age;
        data.alive = mine.alive;
        out.mines.push_back(data);
    }

    out.particles.clear();
    out.particles.reserve(particles_.size());
    for (const Particle &particle : particles_) {
        if (particle.life <= 0.0f) {
            continue;
        }
        net::ParticleSnapshot data;
        data.pos = particle.pos;
        data.vel = particle.vel;
        data.life = particle.life;
        data.maxLife = particle.maxLife;
        data.radius = particle.radius;
        data.color = particle.color;
        data.drag = particle.drag;
        data.gravity = particle.gravity;
        data.layer = particle.layer;
        out.particles.push_back(data);
    }

    out.floatingTexts.clear();
    out.floatingTexts.reserve(floatingTexts_.size());
    for (const FloatingText &text : floatingTexts_) {
        if (text.life <= 0.0f) {
            continue;
        }
        net::FloatingTextSnapshot data;
        data.pos = text.pos;
        data.text = text.text;
        data.color = text.color;
        data.life = text.life;
        data.maxLife = text.maxLife;
        data.size = text.size;
        data.crit = text.crit;
        out.floatingTexts.push_back(std::move(data));
    }

    out.shockwaves.clear();
    out.shockwaves.reserve(shockwaves_.size());
    for (const Shockwave &wave : shockwaves_) {
        if (wave.life <= 0.0f) {
            continue;
        }
        net::ShockwaveSnapshot data;
        data.pos = wave.pos;
        data.maxRadius = wave.maxRadius;
        data.color = wave.color;
        data.duration = wave.duration;
        data.life = wave.life;
        data.width = wave.width;
        out.shockwaves.push_back(data);
    }
}

void Entities::applySnapshot(const net::Snapshot &snapshot)
{
    portals_.clear();
    portals_.reserve(snapshot.portals.size());
    for (const net::PortalSnapshot &data : snapshot.portals) {
        const auto type = Enemy::typeForKey(data.dataKey);
        if (!type) {
            continue;
        }
        Portal portal(*type, data.pos, data.wave, data.enemyLevel);
        portal.boss = data.boss;
        portal.color = data.color;
        portal.duration = data.duration;
        portal.timer = data.timer;
        portal.radius = data.radius;
        portal.alive = data.alive;
        portals_.push_back(portal);
    }

    pickups_.clear();
    pickups_.reserve(snapshot.pickups.size());
    for (const net::PickupSnapshot &data : snapshot.pickups) {
        Pickup pickup(data.pos, data.amount);
        pickup.vel = data.vel;
        pickup.age = data.age;
        pickup.life = data.life;
        pickup.phase = data.phase;
        pickup.radius = data.radius;
        pickup.alive = data.alive;
        pickups_.push_back(pickup);
    }

    mines_.clear();
    mines_.reserve(snapshot.mines.size());
    for (const net::MineSnapshot &data : snapshot.mines) {
        Mine mine;
        mine.pos = data.pos;
        mine.radius = data.radius;
        mine.triggerRadius = data.triggerRadius;
        mine.blastRadius = data.blastRadius;
        mine.damage = data.damage;
        mine.armTime = data.armTime;
        mine.life = data.life;
        mine.age = data.age;
        mine.alive = data.alive;
        mines_.push_back(mine);
    }

    particles_.clear();
    particles_.reserve(snapshot.particles.size());
    for (const net::ParticleSnapshot &data : snapshot.particles) {
        Particle particle;
        particle.pos = data.pos;
        particle.vel = data.vel;
        particle.life = data.life;
        particle.maxLife = data.maxLife;
        particle.radius = data.radius;
        particle.color = data.color;
        particle.drag = data.drag;
        particle.gravity = data.gravity;
        particle.layer = data.layer;
        particles_.push_back(particle);
    }

    floatingTexts_.clear();
    floatingTexts_.reserve(snapshot.floatingTexts.size());
    for (const net::FloatingTextSnapshot &data : snapshot.floatingTexts) {
        FloatingText text;
        text.pos = data.pos;
        text.text = data.text;
        text.color = data.color;
        text.life = data.life;
        text.maxLife = data.maxLife;
        text.size = data.size;
        text.crit = data.crit;
        floatingTexts_.push_back(std::move(text));
    }

    shockwaves_.clear();
    shockwaves_.reserve(snapshot.shockwaves.size());
    for (const net::ShockwaveSnapshot &data : snapshot.shockwaves) {
        Shockwave wave;
        wave.pos = data.pos;
        wave.maxRadius = data.maxRadius;
        wave.color = data.color;
        wave.duration = data.duration;
        wave.life = data.life;
        wave.width = data.width;
        shockwaves_.push_back(wave);
    }
    effectsTimer_ = 0.0f;
}

Math::Vec2 Entities::randomSpawnPosition(const Context &ctx) const
{
    const Math::Vec2 playerPos = ctx.combatants.player().position();
    Math::Vec2 candidate(60.0f, 60.0f);
    for (int attempt = 0; attempt < 20; ++attempt) {
        const int edge = RandomService.integer(0, 3);
        if (edge == 0) {
            candidate = {RandomService.real(GameConfig::FieldLeft + 40.0f,
                                             GameConfig::FieldRight - 40.0f),
                         GameConfig::FieldTop + 18.0f};
        } else if (edge == 1) {
            candidate = {GameConfig::FieldRight - 18.0f,
                         RandomService.real(GameConfig::FieldTop + 40.0f,
                                            GameConfig::FieldBottom - 30.0f)};
        } else if (edge == 2) {
            candidate = {RandomService.real(GameConfig::FieldLeft + 40.0f,
                                             GameConfig::FieldRight - 40.0f),
                         GameConfig::FieldBottom - 18.0f};
        } else {
            candidate = {GameConfig::FieldLeft + 18.0f,
                         RandomService.real(GameConfig::FieldTop + 40.0f,
                                            GameConfig::FieldBottom - 30.0f)};
        }
        if (candidate.distanceTo(playerPos) > 235.0f) {
            break;
        }
    }
    return candidate;
}

} // namespace arena
