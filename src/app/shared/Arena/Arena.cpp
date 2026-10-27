#include "Arena/Arena.h"

#include "Base/Base.h"
#include "GameConfig.h"
#include "Net/Snapshot.h"
#include "Random.h"
#include "Shop/Shop.h"

#include <algorithm>
#include <array>

// 组件是私有基类：构造函数里它们已经就绪，这里只做一轮 reset。
Arena::Arena()
{
    reset();
}

Arena::~Arena() = default;

int Arena::score() const
{
    return ShopService.score().total();
}

int Arena::kills() const
{
    return ShopService.score().kills();
}

// Context 只保存组件引用；const 方法（draw）也要把可写引用交给组件的 const 绘制，
// 组件不会修改状态，所以这里只做一次 const_cast。
arena::Context Arena::context() const
{
    Arena &self = const_cast<Arena &>(*this);
    return arena::Context{self, self, self, self, self, self};
}

void Arena::reset()
{
    Bullets::clear();
    Combatants::reset();
    Entities::reset();
    State::reset();
    feedback_ = Feedback{};
}

void Arena::configurePlayers(const std::vector<int> &playerIds)
{
    Bullets::clear();
    Combatants::configurePlayers(playerIds);
    Entities::reset();
    State::reset();
    feedback_ = Feedback{};
}

void Arena::setInput(const arena::InputState &input)
{
    // 单机路径：输入写给第一架飞机。
    for (Player &player : players()) {
        if (!player.state.active) {
            continue;
        }
        const bool dash = player.input.dashRequested;
        player.input = input;
        player.input.dashRequested = input.dashRequested || dash;
        return;
    }
}

void Arena::setInput(int playerId, const arena::InputState &input)
{
    if (playerId <= 0 || !hasPlayer(playerId)) {
        return;
    }
    Player &player = playerById(playerId);
    // 冲刺请求可能在两次输入快照之间由 requestDash() 单独置位，不能被覆盖掉。
    const bool dash = player.input.dashRequested;
    player.input = input;
    player.input.dashRequested = input.dashRequested || dash;
}

const Arena::PlayerState &Arena::playerState() const
{
    return localPlayer().state;
}

Arena::PlayerState &Arena::playerState()
{
    return const_cast<PlayerState &>(localPlayer().state);
}

const Arena::PlayerState &Arena::playerStateAt(int index) const
{
    return playerAt(index).state;
}

Arena::PlayerState &Arena::playerStateAt(int index)
{
    return playerAt(index).state;
}

int Arena::playerIdAt(int index) const
{
    return playerAt(index).state.playerId;
}

int Arena::playerCount() const
{
    return Combatants::playerCount();
}

int Arena::livingPlayerCount() const
{
    return Combatants::livingPlayerCount();
}

bool Arena::hasPlayer(int playerId) const
{
    return Combatants::hasPlayer(playerId);
}

int Arena::localPlayerId() const
{
    return Combatants::localPlayerId();
}

void Arena::setLocalPlayerId(int playerId)
{
    Combatants::setLocalPlayerId(playerId);
}

bool Arena::removePlayer(int playerId)
{
    return Combatants::removePlayer(playerId);
}

const Arena::Player &Arena::localPlayer() const
{
    const int id = Combatants::localPlayerId();
    if (id > 0) {
        for (const Player &player : players()) {
            if (player.state.active && player.state.playerId == id) {
                return player;
            }
        }
    }
    for (const Player &player : players()) {
        if (player.state.active) {
            return player;
        }
    }
    return player();
}

bool Arena::allPlayersDefeated() const
{
    return Combatants::livingPlayerCount() == 0;
}

void Arena::updateRespawns(const arena::Context &ctx, float dt)
{
    if (Combatants::livingPlayerCount() == 0) {
        return;   // 全灭由 update() 判负，不再复活。
    }
    for (Player &player : players()) {
        if (!player.state.active || player.state.alive || !player.respawnEnabled
            || player.state.respawnTimer <= 0.0f) {
            continue;
        }
        player.state.respawnTimer -= dt;
        if (player.state.respawnTimer <= 0.0f) {
            player.respawn(ctx);
        }
    }
}

void Arena::beginWave()
{
    const arena::Context ctx = context();
    Bullets::clear();
    Entities::clearMines();
    Combatants::beginWave();
    State::beginWave();
    for (const Player &player : players()) {
        if (!player.state.active || !player.state.alive) {
            continue;
        }
        Entities::spawnBurst(ctx, player.state.pos,
                             player.state.color.isValid() ? player.state.color
                                                          : Base::Cyan,
                             14, 90.0f, 0.5f);
    }
}

void Arena::update(float dt)
{
    const arena::Context ctx = context();
    if (allPlayersDefeated()) {
        State::markPlayerDefeated();
        Entities::updateEffects(ctx, dt);
        return;
    }

    updateRespawns(ctx, dt);
    Combatants::updatePlayers(ctx, dt);
    if (allPlayersDefeated()) {
        State::markPlayerDefeated();
        Entities::updateEffects(ctx, dt);
        return;
    }

    Entities::updatePortals(ctx, dt);
    Bullets::update(dt);
    Events::resolvePlayerBullets(ctx);
    if (allPlayersDefeated()) {
        State::markPlayerDefeated();
        Entities::updateEffects(ctx, dt);
        return;
    }

    Events::resolveEnemyBullets(ctx);
    if (allPlayersDefeated()) {
        State::markPlayerDefeated();
        Entities::updateEffects(ctx, dt);
        return;
    }

    Combatants::updateEnemies(ctx, dt);
    Events::separateEnemies(ctx);
    Events::contactDamage(ctx);
    if (allPlayersDefeated()) {
        State::markPlayerDefeated();
        Entities::updateEffects(ctx, dt);
        return;
    }

    Entities::updatePickups(ctx, dt);
    Entities::updateMines(dt);
    Events::resolveMines(ctx);
    Combatants::cleanup();
    Entities::cleanup();
    Entities::updateEffects(ctx, dt);
}

void Arena::draw(QPainter &world, QPainter &glow) const
{
    const arena::Context ctx = context();
    drawBorder(world);
    Entities::drawBackground(world, glow);
    Bullets::draw(world, glow);
    Combatants::draw(world, glow, ctx);
    Entities::drawForeground(world, glow);
}

void Arena::spawnPortal(arena::EnemyType type, int wave, int enemyLevel)
{
    Entities::spawnPortal(context(), type, wave, enemyLevel);
}

void Arena::placeMine(const Math::Vec2 &position)
{
    const Math::Vec2 clamped(
        Math::clamp(position.x, GameConfig::FieldLeft + 16.0f,
                    GameConfig::FieldRight - 16.0f),
        Math::clamp(position.y, GameConfig::FieldTop + 16.0f,
                    GameConfig::FieldBottom - 16.0f));
    Entities::spawnMine(context(), clamped);
}

void Arena::updateVisuals(float dt)
{
    Entities::updateEffects(context(), dt);
}

void Arena::clearEnemyBullets()
{
    Bullets::clearEnemy();
}

void Arena::clearProjectiles()
{
    Bullets::clear();
}

void Arena::clearForVictory()
{
    Bullets::clearEnemy();
    Entities::clearMines();
    Combatants::clearForVictory(context());
}

void Arena::requestDash(int playerId)
{
    if (playerId > 0) {
        if (hasPlayer(playerId)) {
            playerById(playerId).input.dashRequested = true;
        }
        return;
    }
    for (Player &player : players()) {
        if (player.state.active) {
            player.input.dashRequested = true;
            return;
        }
    }
}

void Arena::celebrateUpgrade()
{
    const arena::Context ctx = context();
    for (const Player &player : players()) {
        if (!player.state.active || !player.state.alive) {
            continue;
        }
        const Math::Vec2 pos = player.state.pos;
        const QColor color = player.state.color.isValid() ? player.state.color
                                                          : Base::Cyan;
        Entities::spawnBurst(ctx, pos, color, 26, 190.0f, 0.6f);
        Entities::addShockwave(pos, 110.0f, color, 0.55f, 2);
    }
}

void Arena::celebrateVictory()
{
    for (const Player &player : players()) {
        if (!player.state.active) {
            continue;
        }
        const Math::Vec2 pos = player.state.pos;
        Entities::addShockwave(pos, 150.0f, Base::Cyan, 0.7f, 2);
        Entities::addShockwave(pos, 250.0f, Base::Cyan, 0.9f, 2);
        Entities::addShockwave(pos, 380.0f, Base::Cyan, 1.1f, 2);
    }
}

Arena::Feedback Arena::takeFeedback()
{
    Feedback result = feedback_;
    feedback_ = Feedback{};
    return result;
}

void Arena::writeSnapshot(net::Snapshot &out) const
{
    Combatants::writeSnapshot(out);
    Bullets::writeSnapshot(out);
    Entities::writeSnapshot(out);
}

void Arena::applySnapshot(const net::Snapshot &snapshot)
{
    // 重建敌人/传送门会走构造函数，其中一部分会取随机数（攻击计时、走位相位等）。
    // 视图重建必须对权威模拟的随机流零影响，因此保存并恢复全局随机引擎状态。
    const Random::EngineState rngState = RandomService.engineState();
    Combatants::applySnapshot(snapshot);
    Bullets::applySnapshot(snapshot);
    Entities::applySnapshot(snapshot);
    RandomService.setEngineState(rngState);
    feedback_ = Feedback{};
}

// arena::Host 的实现：组件通过虚接口调用到这里。
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
