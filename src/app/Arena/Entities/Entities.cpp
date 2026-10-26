#include "Entities.h"

#include "Arena/Context.h"
#include "Arena/Host.h"
#include "Arena/Combatants/Combatants.h"
#include "Arena/Combatants/Enemy.h"
#include "Data/GameData.h"
#include "GameConfig.h"
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

void Entities::spawnPortal(const Context &ctx, EnemyType type, int wave)
{
    const Math::Vec2 position =
        Enemy::isBoss(type)
            ? Math::Vec2(GameConfig::ScreenWidth / 2.0f, 190.0f)
            : randomSpawnPosition(ctx);
    portals_.emplace_back(type, position, wave);
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
