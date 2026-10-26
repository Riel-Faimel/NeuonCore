#include "Arena/Arena.h"

#include "GameConfig.h"
#include "Random.h"
#include "Score.h"
#include "Base/Base.h"

#include <algorithm>
#include <array>

Arena::Arena(Random &random, Base &base, Score &score)
    : random_(random), base_(base), score_(score)
{
    reset();
}

int Arena::score() const
{
    return score_.total();
}

int Arena::kills() const
{
    return score_.kills();
}

Arena::~Arena() = default;

void Arena::reset()
{
    bullets_.clear();
    combatants_.reset();
    entities_.reset();
    state_.reset();
    feedback_ = Feedback{};
}

void Arena::beginWave()
{
    bullets_.clear();
    entities_.clearMines();
    combatants_.beginWave();
    state_.beginWave();
    entities_.spawnBurst(*this, combatants_.player().state.pos, Base::Cyan,
                         14, 90.0f, 0.5f);
}

void Arena::update(float dt)
{
    if (!combatants_.player().isAlive()) {
        entities_.updateEffects(*this, dt);
        return;
    }

    combatants_.updatePlayer(*this, dt);
    if (state_.playerDefeated()) {
        entities_.updateEffects(*this, dt);
        return;
    }

    entities_.updatePortals(*this, dt);
    bullets_.update(dt);
    events_.resolvePlayerBullets(*this);
    if (state_.playerDefeated()) {
        entities_.updateEffects(*this, dt);
        return;
    }

    events_.resolveEnemyBullets(*this);
    if (state_.playerDefeated()) {
        entities_.updateEffects(*this, dt);
        return;
    }

    combatants_.updateEnemies(*this, dt);
    events_.separateEnemies(*this);
    events_.contactDamage(*this);
    if (state_.playerDefeated()) {
        entities_.updateEffects(*this, dt);
        return;
    }

    entities_.updatePickups(*this, dt);
    entities_.updateMines(dt);
    events_.resolveMines(*this);
    combatants_.cleanup();
    bullets_.cleanup();
    entities_.cleanup();
    entities_.updateEffects(*this, dt);
}

void Arena::draw(QPainter &world, QPainter &glow) const
{
    drawBorder(world);
    entities_.drawBackground(world, glow);
    bullets_.draw(world, glow);
    combatants_.draw(world, glow, *this);
    entities_.drawForeground(world, glow);
}

void Arena::spawnPortal(const QString &kind, int wave)
{
    entities_.spawnPortal(*this, kind, wave);
}

void Arena::placeMine(const Math::Vec2 &position)
{
    const Math::Vec2 clamped(
        Math::clamp(position.x, GameConfig::FieldLeft + 16.0f,
                    GameConfig::FieldRight - 16.0f),
        Math::clamp(position.y, GameConfig::FieldTop + 16.0f,
                    GameConfig::FieldBottom - 16.0f));
    entities_.spawnMine(*this, clamped);
}

void Arena::updateVisuals(float dt)
{
    entities_.updateEffects(*this, dt);
}

void Arena::clearEnemyBullets()
{
    bullets_.clearEnemy();
}

void Arena::clearProjectiles()
{
    bullets_.clear();
}

void Arena::clearForVictory()
{
    bullets_.clearEnemy();
    entities_.clearMines();
    combatants_.clearForVictory(*this);
}

void Arena::requestDash()
{
    combatants_.player().state.dashBuffer = 0.16f;
}

void Arena::celebrateUpgrade()
{
    const Math::Vec2 pos = combatants_.player().state.pos;
    entities_.spawnBurst(*this, pos, Base::Cyan, 26, 190.0f, 0.6f);
    entities_.addShockwave(pos, 110.0f, Base::Cyan, 0.55f, 2);
}

void Arena::celebrateVictory()
{
    const Math::Vec2 pos = combatants_.player().state.pos;
    entities_.addShockwave(pos, 150.0f, Base::Cyan, 0.7f, 2);
    entities_.addShockwave(pos, 250.0f, Base::Cyan, 0.9f, 2);
    entities_.addShockwave(pos, 380.0f, Base::Cyan, 1.1f, 2);
}

Arena::Feedback Arena::takeFeedback()
{
    Feedback result = feedback_;
    feedback_ = Feedback{};
    return result;
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
