#pragma once

// 测试专用代码：只在 -DDEBUG 构建中由 main.cpp 包含。
// 交付的 NeonCore.exe 不包含这里的任何逻辑。

#include "Menu/Menu.h"
#include "Base/Base.h"
#include "Client/GameView.h"
#include "Data/GameData.h"
#include "Net/Client.h"
#include "Net/Protocol.h"
#include "Net/Server.h"
#include "Net/Snapshot.h"
#include "Random.h"
#include "Arena/Bullets/StandardBullet.h"
#include "Arena/Combatants/Enemies/EnemyTypes.h"
#include "Arena/Combatants/Enemies/BossEnemy.h"
#include "Loadout/Loadout.h"
#include "Arena/Combatants/Enemies/DroneEnemy.h"
#include "Arena/Combatants/Enemies/MiniEnemy.h"
#include "Arena/Combatants/Enemies/SpitterEnemy.h"
#include "Arena/Combatants/Enemies/TankEnemy.h"

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>

#ifdef DEBUG

class SelfTest {
public:
    static void simulate(Games &game, float seconds, bool shoot)
    {
        const int steps = int(seconds * 60.0f);
        for (int i = 0; i < steps; ++i) {
            if (game.state() == Games::State::Playing && shoot
                && game.arena_.playerState().alive) {
                game.arena_.player().state.invuln =
                    std::max(game.arena_.player().state.invuln, 2.0f);
                arena::Enemy *target = nullptr;
                float best = 1e9f;
                for (const auto &enemy : game.arena_.enemies()) {
                    if (!enemy->alive) {
                        continue;
                    }
                    const float distance = enemy->pos.distanceSquaredTo(
                        game.arena_.playerState().pos);
                    if (distance < best) {
                        best = distance;
                        target = enemy.get();
                    }
                }
                if (target != nullptr) {
                    const Math::Vec2 aim = target->pos + target->vel * 0.15f;
                    BaseService.mouseMove(aim);
                    BaseService.mousePressLeft(aim);
                }
            }
            game.update(1.0f / 60.0f);
        }
    }

    static bool autoplay(Games &game, float maxSeconds)
    {
        const int maxSteps = int(maxSeconds * 60.0f);
        for (int i = 0; i < maxSteps; ++i) {
            if (game.state() == Games::State::Victory) {
                return true;
            }
            if (game.state() == Games::State::Defeat) {
                return false;
            }
            if (game.state() == Games::State::Upgrade) {
                game.selectUpgrade(0);
                continue;
            }
            if (game.state() == Games::State::Playing) {
                game.arena_.player().state.invuln = 999.0f;
                arena::Enemy *target = nullptr;
                float best = 1e9f;
                Math::Vec2 dodge;
                for (const auto &enemy : game.arena_.enemies()) {
                    if (!enemy->alive) {
                        continue;
                    }
                    const float distance = enemy->pos.distanceSquaredTo(
                        game.arena_.playerState().pos);
                    if (distance < best) {
                        best = distance;
                        target = enemy.get();
                    }
                    const float realDistance = std::sqrt(distance);
                    if (realDistance > 1.0f && realDistance < 250.0f) {
                        const Math::Vec2 away =
                            (game.arena_.playerState().pos - enemy->pos).normalized();
                        dodge += away * ((250.0f - realDistance) / 250.0f);
                    }
                }
                if (target != nullptr) {
                    const Math::Vec2 aim = target->pos + target->vel * 0.15f;
                    BaseService.mouseMove(aim);
                    BaseService.mousePressLeft(aim);
                }
                const Math::Vec2 center(GameConfig::ScreenWidth / 2.0f,
                                        GameConfig::ScreenHeight / 2.0f);
                dodge += (center - game.arena_.playerState().pos) * 0.02f;
                if (dodge.lengthSquared() < 0.01f) {
                    dodge = Math::Vec2(1.0f, 0.0f).rotated(game.runTime());
                }
                BaseService.keyRelease(Qt::Key_A);
                BaseService.keyRelease(Qt::Key_D);
                BaseService.keyRelease(Qt::Key_W);
                BaseService.keyRelease(Qt::Key_S);
                if (dodge.x < -0.12f) {
                    BaseService.keyPress(Qt::Key_A);
                } else if (dodge.x > 0.12f) {
                    BaseService.keyPress(Qt::Key_D);
                }
                if (dodge.y < -0.12f) {
                    BaseService.keyPress(Qt::Key_W);
                } else if (dodge.y > 0.12f) {
                    BaseService.keyPress(Qt::Key_S);
                }
                if (std::sqrt(best) < 95.0f && game.arena_.playerState().dashCd <= 0.0f) {
                    game.arena_.requestDash();
                }
            }
            game.update(1.0f / 60.0f);
        }
        return false;
    }

    static int run()
    {
        QFile logFile(QStringLiteral("smoke.log"));
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            std::fprintf(stderr, "Cannot open smoke.log\n");
        }
        QTextStream stream(&logFile);

        auto log = [&](const QString &message) {
            stream << message << Qt::endl;
            stream.flush();
            std::printf("%s\n", message.toUtf8().constData());
            std::fflush(stdout);
        };
        auto require = [&](bool condition, const QString &message) {
            if (!condition) {
                log(QStringLiteral("  FAIL  ") + message);
                return false;
            }
            log(QStringLiteral("  ok    ") + message);
            return true;
        };

        bool success = true;
        const bool audioRequested = qEnvironmentVariableIsSet("NEON_AUDIO_TEST");
        log(QStringLiteral("Creating game (offscreen, audio %1)")
                .arg(audioRequested ? QStringLiteral("enabled") : QStringLiteral("disabled")));
        LoadoutService.select(0);
        Menu menu(audioRequested, true);
        Games *game = nullptr;
        log(QStringLiteral("Audio initialized: %1")
                .arg(menu.audioEnabled() ? QStringLiteral("yes") : QStringLiteral("no")));
        if (audioRequested) {
            menu.toggleMute();
        }

        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("boots into main menu"));
        menu.update(1.0f / 60.0f);
        menu.render();

        log(QStringLiteral("Input flow"));
        menu.keyPress(Qt::Key_Return);
        menu.keyRelease(Qt::Key_Return);
        success &= require(menu.screen() == Menu::Screen::Map,
                           QStringLiteral("enter opens the sector map"));
        menu.keyPress(Qt::Key_Right);
        menu.keyRelease(Qt::Key_Right);
        success &= require(menu.map_.selected() == 1,
                           QStringLiteral("map arrows move the selection"));
        success &= require(data::kCampaigns[0].count == 8
                               && data::kCampaigns[1].count == 10
                               && data::kCampaigns[2].count == 12
                               && data::kCampaigns[3].count == 12,
                           QStringLiteral("later campaigns get longer"));
        menu.keyPress(Qt::Key_Return);
        menu.keyRelease(Qt::Key_Return);
        game = menu.match_.game();
        success &= require(game != nullptr && game->totalWaves() == 10,
                           QStringLiteral("second region starts the level 2 campaign"));
        success &= require(game != nullptr && game->waves_.enemyLevel_ == 2,
                           QStringLiteral("level 2 enemies use level 2 scaling"));
        menu.startCampaign(3);
        game = menu.match_.game();
        success &= require(game != nullptr && game->totalWaves() == 12
                               && game->waves_.enemyLevel_ == 4,
                           QStringLiteral("final region uses the level 4 campaign"));
        menu.returnToMenu();
        menu.startLevel(0);
        game = menu.match_.game();
        success &= require(game != nullptr && game->state() == Games::State::Playing,
                           QStringLiteral("enter on the first node starts the run"));
        menu.keyPress(Qt::Key_Escape);
        menu.keyRelease(Qt::Key_Escape);
        success &= require(game->state() == Games::State::Paused,
                           QStringLiteral("escape pauses"));
        menu.keyPress(Qt::Key_P);
        menu.keyRelease(Qt::Key_P);
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("p resumes"));

        game->upgrades_.open();
        game->state_ = Games::State::Upgrade;
        game->renderer_.upgradeIndexAt(Math::Vec2(300, 400));
        menu.mousePressLeft(Math::Vec2(300, 400));
        menu.mouseReleaseLeft();
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("mouse click selects an upgrade"));

        menu.returnToMenu();
        log(QStringLiteral("Starting run"));
        menu.startLevel(0);
        game = menu.match_.game();
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("run starts"));
        success &= require(game->waves_.currentWave() == 1,
                           QStringLiteral("starts at wave 1"));
        simulate(*game, 7.0f, true);
        game->render();

        game->waves_.queue_.clear();
        game->arena_.portals().clear();
        game->arena_.enemies().clear();
        game->arena_.clearEnemy();
        for (int i = 0; i < 70; ++i) {
            game->update(1.0f / 60.0f);
            if (game->state() == Games::State::Upgrade) {
                break;
            }
        }
        success &= require(game->state() == Games::State::Upgrade,
                           QStringLiteral("wave clear opens upgrade screen"));
        success &= require(game->upgrades_.offers().size() == 3,
                           QStringLiteral("offers three upgrades"));
        bool rolled = true;
        for (const games::Upgrades::Offer &offer : game->upgrades_.offers()) {
            rolled = rolled && !offer.valueText.isEmpty();
            for (const games::Upgrades::Definition &definition
                 : games::Upgrades::catalog()) {
                if (definition.key != offer.key) {
                    continue;
                }
                rolled = rolled && offer.value >= definition.minValue - 0.001f
                         && offer.value <= definition.maxValue + 0.001f;
            }
        }
        success &= require(rolled,
                           QStringLiteral("upgrade values roll inside their ranges"));
        game->render();

        const Arena::PlayerState before = game->arena_.playerState();
        game->selectUpgrade(0);
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("upgrade resumes play"));
        success &= require(game->waves_.currentWave() == 2,
                           QStringLiteral("advances to wave 2"));
        success &= require(before.damage != game->arena_.playerState().damage
                               || before.fireCd != game->arena_.playerState().fireCd
                               || before.maxHp != game->arena_.playerState().maxHp
                               || before.hp != game->arena_.playerState().hp
                               || before.speed != game->arena_.playerState().speed
                               || before.multishot != game->arena_.playerState().multishot
                               || before.pierce != game->arena_.playerState().pierce
                               || before.critChance != game->arena_.playerState().critChance
                               || before.shockLevel != game->arena_.playerState().shockLevel,
                           QStringLiteral("upgrade changes a stat"));

        log(QStringLiteral("Support enemies"));
        menu.startLevel(0);
        game = menu.match_.game();
        game->state_ = Games::State::Playing;
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 1;
        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        const arena::Context supportCtx = game->arena_.context();
        supportCtx.combatants.player().state.pos = {640, 420};
        supportCtx.combatants.player().state.invuln = 99.0f;
        auto spawn = [&](const QString &kind, const Math::Vec2 &at) {
            game->arena_.spawnEnemy(supportCtx, kind, at, 1);
            return game->arena_.enemies().back().get();
        };

        arena::Enemy *amplifier = spawn(QStringLiteral("amplifier"), {500, 400});
        arena::Enemy *drone = spawn(QStringLiteral("drone"), {545, 400});
        for (int i = 0; i < 8; ++i) {
            game->arena_.update(1.0f / 60.0f);
        }
        success &= require(drone->damageBuffTimer > 0.0f
                               && drone->contactDamage()
                                      > arena::Enemy::statsFor(QStringLiteral("drone")).damage,
                           QStringLiteral("amplifier boosts nearby ally damage"));

        arena::Enemy *linker = spawn(QStringLiteral("linker"), {600, 400});
        drone->pos = linker->pos + Math::Vec2(40.0f, 0.0f);
        const float linkerHp = linker->hp;
        drone->takeDamage(supportCtx,
                          arena::Damage{10.0f, false, arena::DamageSource::Bullet});
        success &= require(linker->hp < linkerHp,
                           QStringLiteral("linker shares damage for nearby allies"));

        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        menu.clearInput();
        arena::Enemy *mender = spawn(QStringLiteral("mender"), {600, 470});
        arena::Enemy *tank = spawn(QStringLiteral("tank"), {640, 470});
        tank->hp = tank->maxHp * 0.3f;
        for (int i = 0; i < 60; ++i) {
            game->arena_.update(1.0f / 60.0f);
        }
        success &= require(tank->hp > tank->maxHp * 0.3f + 1.0f,
                           QStringLiteral("mender heals nearby allies"));

        arena::Enemy *rusher = spawn(QStringLiteral("rusher"), {800, 300});
        success &= require(
            rusher->speed > arena::Enemy::statsFor(QStringLiteral("drone")).speed * 1.8f
                && rusher->lungeInterval > 0.0f
                && rusher->contactDamage()
                       > arena::Enemy::statsFor(QStringLiteral("drone")).damage,
            QStringLiteral("rusher is fast and aggressive"));

        arena::Enemy *necromancer = spawn(QStringLiteral("necromancer"), {900, 400});
        necromancer->supportRadius = 400.0f;
        necromancer->supportValue = 1.1f;
        arena::Enemy *victim = spawn(QStringLiteral("drone"), {920, 400});
        victim->hp = 1.0f;
        victim->takeDamage(supportCtx,
                           arena::Damage{10.0f, false, arena::DamageSource::Bullet});
        bool revived = false;
        for (const auto &other : game->arena_.enemies()) {
            if (other->revived) {
                revived = true;
            }
        }
        success &= require(revived,
                           QStringLiteral("necromancer revives a fallen ally"));

        necromancer->supportRadius = 0.0f;   // 避免复活干扰召唤计数
        arena::Enemy *summoner = spawn(QStringLiteral("summoner"), {1080, 560});
        summoner->charge = 0;
        summoner->chargeRequired = 3;
        summoner->summonCount = 2;
        int minisBefore = 0;
        for (const auto &other : game->arena_.enemies()) {
            if (other->type() == typeid(arena::MiniEnemy)) {
                ++minisBefore;
            }
        }
        for (int i = 0; i < 3; ++i) {
            const auto chargedVictim =
                arena::Enemy::create(QStringLiteral("drone"), summoner->pos, 1);
            supportCtx.events.handleKill(supportCtx, *chargedVictim);
        }
        int minisAfter = 0;
        for (const auto &other : game->arena_.enemies()) {
            if (other->type() == typeid(arena::MiniEnemy)) {
                ++minisAfter;
            }
        }
        success &= require(minisAfter == minisBefore + 2 && summoner->charge == 0,
                           QStringLiteral("summoner summons allies after enough deaths"));

        summoner->charge = 0;
        for (int i = 0; i < 3; ++i) {
            const auto miniVictim =
                arena::Enemy::create(QStringLiteral("mini"), summoner->pos, 1);
            supportCtx.events.handleKill(supportCtx, *miniVictim);
        }
        int minisAfterMiniDeaths = 0;
        for (const auto &other : game->arena_.enemies()) {
            if (other->type() == typeid(arena::MiniEnemy)) {
                ++minisAfterMiniDeaths;
            }
        }
        success &= require(summoner->charge == 0
                               && minisAfterMiniDeaths == minisAfter,
                           QStringLiteral("minion deaths do not charge the summoner"));

        success &= require(
            dynamic_cast<arena::OffensiveEnemy *>(
                arena::Enemy::create(QStringLiteral("drone"), {0, 0}, 1).get())
                    != nullptr
                && dynamic_cast<arena::DefensiveEnemy *>(
                       arena::Enemy::create(QStringLiteral("tank"), {0, 0}, 1).get())
                       != nullptr,
            QStringLiteral("vtable: offensive and defensive enemies"));
        success &= require(
            dynamic_cast<arena::RangedEnemy *>(
                arena::Enemy::create(QStringLiteral("spitter"), {0, 0}, 1).get())
                    != nullptr
                && dynamic_cast<arena::BossEnemy *>(
                       arena::Enemy::create(QStringLiteral("boss"), {0, 0}, 1).get())
                       != nullptr,
            QStringLiteral("vtable: ranged enemies and boss"));
        success &= require(
            dynamic_cast<arena::SupportEnemy *>(
                arena::Enemy::create(QStringLiteral("amplifier"), {0, 0}, 1).get())
                    != nullptr
                && dynamic_cast<arena::SupportEnemy *>(
                       arena::Enemy::create(QStringLiteral("necromancer"), {0, 0}, 1).get())
                       != nullptr,
            QStringLiteral("vtable: support enemies"));
        success &= require(
            dynamic_cast<arena::SelfDestructEnemy *>(
                arena::Enemy::create(QStringLiteral("splitter"), {0, 0}, 1).get())
                != nullptr,
            QStringLiteral("vtable: self-destruct enemies"));

        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        spawn(QStringLiteral("drone"), {480, 400});
        spawn(QStringLiteral("drone"), {500, 420});
        arena::Enemy *clusterSupport = spawn(QStringLiteral("amplifier"), {920, 400});
        const float supportStartX = clusterSupport->pos.x;
        for (int i = 0; i < 45; ++i) {
            game->arena_.update(1.0f / 60.0f);
        }
        success &= require(clusterSupport->pos.x < supportStartX - 8.0f,
                           QStringLiteral("support enemies drift toward the ally cluster"));

        // 防御线：防御者站到后排与玩家之间，远程躲到防御者背后，辅助跟后排。
        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        menu.clearInput();
        supportCtx.combatants.player().state.invuln = 99.0f;
        const Math::Vec2 formationPlayer = supportCtx.combatants.player().state.pos;
        arena::Enemy *frontTank = spawn(QStringLiteral("tank"), {700, 420});
        arena::Enemy *backSpitter = spawn(QStringLiteral("spitter"), {900, 420});
        arena::Enemy *backMender = spawn(QStringLiteral("mender"), {900, 520});
        spawn(QStringLiteral("splitter"), {500, 420});
        for (int i = 0; i < 60; ++i) {
            game->arena_.update(1.0f / 60.0f);
        }
        success &= require(
            frontTank->pos.x > formationPlayer.x && frontTank->pos.x < backSpitter->pos.x,
            QStringLiteral("defensive enemies stand between backline and player"));
        success &= require(
            backSpitter->pos.x > frontTank->pos.x + 10.0f
                && backSpitter->pos.distanceTo(formationPlayer)
                       > frontTank->pos.distanceTo(formationPlayer),
            QStringLiteral("ranged enemies hide behind the defensive line"));
        success &= require(backMender->pos.x > 850.0f,
                           QStringLiteral("support enemies stay with the backline"));

        log(QStringLiteral("Boss and victory"));
        menu.startLevel(0);
        game = menu.match_.game();
        game->waves_.start(8);
        game->waves_.queue_.clear();
        game->arena_.beginWave();
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("boss"),
                               Math::Vec2(GameConfig::ScreenWidth / 2.0f, 220.0f), 8);
        success &= require(game->arena_.boss() != nullptr, QStringLiteral("boss spawns"));
        game->arena_.boss()->takeDamage(
            game->arena_.context(), arena::Damage{99999.0f, false, arena::DamageSource::Bullet});
        success &= require(game->arena_.bossDefeated(),
                           QStringLiteral("boss death is recorded"));
        simulate(*game, 3.0f, false);
        success &= require(game->state() == Games::State::Victory,
                           QStringLiteral("victory screen reached"));
        menu.update(0.0f);
        success &= require(menu.screen() == Menu::Screen::Result,
                           QStringLiteral("menu enters victory result"));
        menu.render();

        log(QStringLiteral("Mid-script boss"));
        games::Script bossMid;
        bossMid.push_back(games::Wave{{games::EnemySpawn{
            arena::EnemyType(typeid(arena::DroneEnemy)), 1}}});
        bossMid.push_back(games::Wave{{games::EnemySpawn{
            arena::EnemyType(typeid(arena::BossEnemy)), 1}}});
        bossMid.push_back(games::Wave{{games::EnemySpawn{
            arena::EnemyType(typeid(arena::DroneEnemy)), 1}}});
        menu.levels_.push_back({QStringLiteral("中局 Boss"),
                                QStringLiteral("测试台本"), 2, bossMid});
        menu.startLevel(int(menu.levels_.size()) - 1);
        game = menu.match_.game();
        success &= require(game->currentWave() == 2 && game->waves_.isBossWave(),
                           QStringLiteral("boss can sit in a middle wave"));
        success &= require(game->renderer_.bannerTitle_ == QStringLiteral("第 2 波")
                               && game->renderer_.bannerKicker_
                                      == QStringLiteral("BOSS SIGNAL"),
                           QStringLiteral("middle boss banner is not the final wave"));
        game->waves_.queue_.clear();
        game->arena_.enemies().clear();
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("boss"),
                               Math::Vec2(GameConfig::ScreenWidth / 2.0f, 220.0f), 2);
        game->arena_.boss()->takeDamage(
            game->arena_.context(), arena::Damage{99999.0f, false, arena::DamageSource::Bullet});
        game->update(1.0f / 60.0f);
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("a boss in a middle wave does not end the run"));
        simulate(*game, 1.2f, false);
        success &= require(game->state() == Games::State::Upgrade,
                           QStringLiteral("cleared boss wave opens the upgrade screen"));
        menu.returnToMenu();

        log(QStringLiteral("Game over"));
        menu.startLevel(0);
        game = menu.match_.game();
        game->arena_.player().state.invuln = 0.0f;
        game->arena_.player().takeDamage(
            game->arena_.context(), arena::Damage{9999.0f, false, arena::DamageSource::Bullet});
        game->update(1.0f / 60.0f);
        success &= require(game->state() == Games::State::Defeat,
                           QStringLiteral("lethal damage ends run"));
        menu.update(0.0f);
        success &= require(menu.screen() == Menu::Screen::Result,
                           QStringLiteral("menu enters defeat result"));
        menu.render();
        menu.startLevel(0);
        game = menu.match_.game();
        success &= require(game->state() == Games::State::Playing
                               && game->waves_.currentWave() == 1,
                           QStringLiteral("restart works"));

        log(QStringLiteral("Autoplay full 8-wave run"));
        menu.startLevel(0);
        game = menu.match_.game();
        const bool autoplayVictory = autoplay(*game, 240.0f);
        success &= require(autoplayVictory,
                           QStringLiteral("autoplay clears all 8 waves and the boss"));

        menu.returnToMenu();
        menu.update(1.0f / 60.0f);
        menu.render();
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("returns to main menu"));

        log(QStringLiteral("Custom level"));
        menu.keyPress(Qt::Key_2);
        success &= require(menu.screen() == Menu::Screen::Editor,
                           QStringLiteral("custom card opens the script editor"));
        const int editedWaves = int(menu.editor_.script().size());
        menu.keyPress(Qt::Key_N);
        menu.keyPress(Qt::Key_N);
        success &= require(int(menu.editor_.script().size()) == editedWaves + 2,
                           QStringLiteral("N appends waves"));
        menu.keyPress(Qt::Key_Right);
        const int beforeCount = menu.editor_.enemyCount(menu.editor_.row(), menu.editor_.column());
        for (int i = 0; i < 3; ++i) {
            menu.keyPress(Qt::Key_Plus);
        }
        success &= require(menu.editor_.enemyCount(menu.editor_.row(), menu.editor_.column())
                               == beforeCount + 3,
                           QStringLiteral("+ raises a cell count"));
        const int beforeHold = menu.editor_.enemyCount(menu.editor_.row(),
                                                      menu.editor_.column());
        menu.keyPress(Qt::Key_Plus);
        for (int i = 0; i < 40; ++i) {
            menu.update(1.0f / 60.0f);
        }
        menu.keyRelease(Qt::Key_Plus);
        success &= require(menu.editor_.enemyCount(menu.editor_.row(), menu.editor_.column())
                               > beforeHold + 2,
                           QStringLiteral("holding + repeats the adjustment"));
        menu.keyPress(Qt::Key_Minus);
        for (int i = 0; i < 40; ++i) {
            menu.update(1.0f / 60.0f);
        }
        menu.keyRelease(Qt::Key_Minus);

        const QRectF spitterChip = menu.editor_.paletteChipRect(1);
        const QRectF dropRow = menu.editor_.rowRect(1);
        const int beforeDragTotal = menu.editor_.totalEnemies();
        menu.mousePressLeft({float(spitterChip.center().x()),
                             float(spitterChip.center().y())});
        menu.mouseMove({float(dropRow.center().x()), float(dropRow.center().y())});
        menu.mouseReleaseLeft();
        success &= require(menu.editor_.columnCount() >= 2
                               && menu.editor_.totalEnemies() == beforeDragTotal + 1,
                           QStringLiteral("dragging from the palette adds an enemy column"));

        menu.keyPress(Qt::Key_Return);
        success &= require(menu.screen() == Menu::Screen::ServerSetup,
                           QStringLiteral("custom deploy opens server setup"));
        const menu::ServerSetup::Config defaultSetup = menu.serverSetup_.config();
        success &= require(defaultSetup.mode == menu::ServerSetup::Mode::Single
                               && defaultSetup.port == 0
                               && defaultSetup.host.isEmpty(),
                           QStringLiteral("custom setup defaults to no server"));

        menu.serverSetup_.setMode(menu::ServerSetup::Mode::Host);
        menu.serverSetup_.setPort(8899);
        const menu::ServerSetup::Config hostSetup = menu.serverSetup_.config();
        success &= require(hostSetup.mode == menu::ServerSetup::Mode::Host
                               && hostSetup.port == 8899
                               && hostSetup.host.isEmpty(),
                           QStringLiteral("custom setup exposes the host port"));
        menu.serverSetup_.setMode(menu::ServerSetup::Mode::Join);
        menu.serverSetup_.setHost(QStringLiteral("192.168.1.9"));
        menu.serverSetup_.setPort(8899);
        const menu::ServerSetup::Config joinSetup = menu.serverSetup_.config();
        success &= require(joinSetup.mode == menu::ServerSetup::Mode::Join
                               && joinSetup.port == 8899
                               && joinSetup.host == QStringLiteral("192.168.1.9"),
                           QStringLiteral("custom setup exposes join host and port"));

        menu.keyPress(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Editor,
                           QStringLiteral("escape leaves server setup"));
        menu.keyPress(Qt::Key_Return);
        menu.serverSetup_.setMode(menu::ServerSetup::Mode::Single);
        menu.keyPress(Qt::Key_Return);
        game = menu.match_.game();
        success &= require(menu.screen() == Menu::Screen::Match
                               && game->totalWaves() == editedWaves + 2
                               && game->currentWave() == 1,
                           QStringLiteral("edited script drives the run from wave 1"));
        menu.returnToMenu();

        QTcpServer probe;
        success &= require(probe.listen(QHostAddress::LocalHost, 0),
                           QStringLiteral("custom host can reserve a test port"));
        const quint16 customHostPort = probe.serverPort();
        probe.close();
        menu.serverSetup_.setMode(menu::ServerSetup::Mode::Host);
        menu.serverSetup_.setPort(customHostPort);
        menu.startCustom(menu.serverSetup_.config());
        success &= require(menu.screen() == Menu::Screen::Match
                               && menu.match_.localServer() != nullptr
                               && menu.match_.localServer()->isListening()
                               && menu.match_.localServer()->port() == customHostPort,
                           QStringLiteral("custom host binds the configured port"));
        menu.returnToMenu();

        log(QStringLiteral("Script editor"));
        menu.keyPress(Qt::Key_2);
        success &= require(menu.editor_.columnCount() < 5,
                           QStringLiteral("editor shows only used columns"));
        menu.keyPress(Qt::Key_5);
        success &= require(menu.editor_.waveHasBoss(menu.editor_.row()),
                           QStringLiteral("editor adds the boss column"));
        menu.keyPress(Qt::Key_Minus);
        success &= require(!menu.editor_.waveHasBoss(menu.editor_.row()),
                           QStringLiteral("editor clears the boss wave"));
        menu.keyPress(Qt::Key_X);
        success &= require(int(menu.editor_.script().size()) == editedWaves + 1,
                           QStringLiteral("X removes the last wave"));
        menu.keyPress(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves the editor"));

        const QRectF customCard = menu.main_.levelRect(Menu::CustomLevelIndex, int(menu.levels_.size()));
        menu.mousePressLeft({float(customCard.center().x()),
                             float(customCard.center().y())});
        success &= require(menu.screen() == Menu::Screen::Editor,
                           QStringLiteral("clicking the custom card opens the editor"));
        menu.keyPress(Qt::Key_Escape);

        log(QStringLiteral("Settings"));
        const QRectF gear = menu.main_.settingsButtonRect();
        menu.mousePressLeft({float(gear.center().x()), float(gear.center().y())});
        menu.mouseReleaseLeft();
        success &= require(menu.screen() == Menu::Screen::Settings,
                           QStringLiteral("settings button opens settings"));
        BaseService.setSoundVolume(0.5f);
        BaseService.setMusicVolume(0.5f);
        menu.keyPress(Qt::Key_Down);
        menu.keyPress(Qt::Key_Left);
        success &= require(BaseService.musicVolume() < 0.5f,
                           QStringLiteral("left lowers the music volume"));
        menu.keyPress(Qt::Key_Right);
        success &= require(std::abs(BaseService.musicVolume() - 0.5f) < 0.001f,
                           QStringLiteral("right raises the music volume"));
        menu.keyPress(Qt::Key_Up);
        menu.keyPress(Qt::Key_Left);
        success &= require(BaseService.soundVolume() < 0.5f,
                           QStringLiteral("left lowers the sound volume"));
        const QRectF musicTrack = menu.settings_.trackRect(1);
        menu.keyPress(Qt::Key_Down);
        menu.mousePressLeft({float(musicTrack.left()) + float(musicTrack.width()) * 0.25f,
                             float(musicTrack.center().y())});
        menu.mouseReleaseLeft();
        success &= require(std::abs(BaseService.musicVolume() - 0.25f) < 0.02f,
                           QStringLiteral("clicking the track sets the volume"));
        menu.keyPress(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves settings"));

        log(QStringLiteral("Armory"));
        LoadoutService.select(0);
        const QRectF armoryEntry = menu.main_.armoryButtonRect();
        menu.mousePressLeft({float(armoryEntry.center().x()),
                             float(armoryEntry.center().y())});
        menu.mouseReleaseLeft();
        success &= require(menu.screen() == Menu::Screen::Armory,
                           QStringLiteral("armory button opens war prep"));
        menu.keyPress(Qt::Key_Right);
        menu.keyRelease(Qt::Key_Right);
        menu.keyPress(Qt::Key_Return);
        menu.keyRelease(Qt::Key_Return);
        success &= require(LoadoutService.selected() == 1,
                           QStringLiteral("armory equips the scatter launcher"));
        LoadoutService.select(2);
        arena::Player railPlayer;
        railPlayer.reset();
        success &= require(railPlayer.state.damage > data::kPlayer.damage
                               && railPlayer.state.fireCd > data::kPlayer.fireCd,
                           QStringLiteral("launcher changes player damage and fire rate"));
        LoadoutService.select(0);
        menu.keyPress(Qt::Key_Escape);
        menu.keyRelease(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves war prep"));

        log(QStringLiteral("Enemy levels"));
        const auto levelOne = arena::Enemy::create(
            QStringLiteral("drone"), {0, 0}, 1);
        const auto levelThree = arena::Enemy::create(
            QStringLiteral("drone"), {0, 0}, 1);
        levelThree->setLevel(3);
        success &= require(levelThree->maxHp > levelOne->maxHp * 1.3f
                               && levelThree->damage > levelOne->damage,
                           QStringLiteral("enemy levels scale stats without a level readout"));

        log(QStringLiteral("Score"));
        const int wallet = ShopService.score().total();
        ShopService.score().beginRun();
        ShopService.score().addKill(120);
        success &= require(ShopService.score().total() == wallet + 120
                               && ShopService.score().run() == 120 && ShopService.score().kills() == 1,
                           QStringLiteral("kill adds to global wallet and run total"));
        success &= require(ShopService.score().spend(50) && ShopService.score().total() == wallet + 70,
                           QStringLiteral("spend deducts from the wallet"));
        success &= require(!ShopService.score().spend(999999),
                           QStringLiteral("overdraft is rejected"));
        ShopService.score().beginRun();
        success &= require(ShopService.score().kills() == 0 && ShopService.score().run() == 0
                               && ShopService.score().total() == wallet + 70,
                           QStringLiteral("beginRun resets only the run stats"));
        ShopService.score().total_ = wallet;   // 还原，避免测试改动持久化余额

        log(QStringLiteral("Shop"));
        ShopService.score().total_ = 500;
        const int startMines = ShopService.inventory().mines();
        const QRectF shopButton = menu.main_.shopButtonRect();
        menu.mousePressLeft({float(shopButton.center().x()),
                             float(shopButton.center().y())});
        success &= require(menu.screen() == Menu::Screen::Shop,
                           QStringLiteral("shop button opens shop"));
        const QRectF buyButton = menu.depot_.buyRect();
        menu.mousePressLeft({float(buyButton.center().x()),
                             float(buyButton.center().y())});
        success &= require(ShopService.score().total() == 500 - data::kMine.price
                               && ShopService.inventory().mines() == startMines + 1,
                           QStringLiteral("buying a mine spends score and adds stock"));
        menu.keyPress(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves shop"));

        log(QStringLiteral("Mine"));
        menu.startLevel(0);
        Games *mineGame = menu.match_.game();
        success &= require(mineGame != nullptr, QStringLiteral("mine test starts a run"));
        if (mineGame != nullptr) {
            for (int i = 0; i < 240 && mineGame->arena_.enemies().empty(); ++i) {
                mineGame->update(1.0f / 60.0f);
            }
            success &= require(!mineGame->arena_.enemies().empty(),
                               QStringLiteral("wave spawns enemies for the mine test"));
            if (!mineGame->arena_.enemies().empty()) {
                arena::Enemy *victim = mineGame->arena_.enemies().front().get();
                const Math::Vec2 spot = victim->pos;
                const int stockBefore = ShopService.inventory().mines();
                menu.mousePressRight(spot);
                success &= require(mineGame->arena_.mineCount() == 1
                                       && ShopService.inventory().mines() == stockBefore - 1,
                                   QStringLiteral("right click places a mine"));
                const int killsBefore = mineGame->kills();
                for (int i = 0; i < 60 && mineGame->arena_.mineCount() > 0; ++i) {
                    victim->pos = spot;
                    mineGame->update(1.0f / 60.0f);
                }
                success &= require(mineGame->arena_.mineCount() == 0
                                       && mineGame->kills() > killsBefore,
                                   QStringLiteral("enemy contact detonates the mine"));
            }
            menu.returnToMenu();
        }

        log(QStringLiteral("Bullet pool"));
        menu.startLevel(0);
        game = menu.match_.game();
        if (game != nullptr) {
            game->arena_.playerState().invuln = 999.0f;
            const int beforePool = game->arena_.pooledBulletCount();
            game->arena_.spawnStandard({100.0f, 100.0f}, {0.0f, 0.0f}, 1.0f,
                                       Base::Cyan, Bullet::Standpoint::Player, 3.0f);
            for (int i = 0; i < 160; ++i) {
                game->arena_.playerState().invuln = 999.0f;
                game->arena_.update(1.0f / 60.0f);
            }
            success &= require(game->arena_.pooledBulletCount() > beforePool,
                               QStringLiteral("dead standard bullets return to the pool"));
            menu.returnToMenu();
        }

        log(QStringLiteral("Submitted script"));
        games::Script shortScript;
        shortScript.push_back(games::Wave{{games::EnemySpawn{
                                               arena::EnemyType(typeid(arena::DroneEnemy)), 2},
                                           games::EnemySpawn{
                                               arena::EnemyType(typeid(arena::TankEnemy)), 1}}});
        shortScript.push_back(games::Wave{{games::EnemySpawn{
            arena::EnemyType(typeid(arena::SpitterEnemy)), 3}}});
        menu.levels_.push_back({QStringLiteral("测试台本"),
                                QStringLiteral("运行时提交"), 1, shortScript});
        menu.startLevel(int(menu.levels_.size()) - 1);
        game = menu.match_.game();
        success &= require(game->totalWaves() == 2 && game->currentWave() == 1
                               && game->waves_.queue_.size() == 3,
                           QStringLiteral("submitted script drives the run"));
        menu.returnToMenu();

        log(QStringLiteral("Snapshot roundtrip and GameView"));
        menu.startLevel(0);
        game = menu.match_.game();
        success &= require(game != nullptr, QStringLiteral("snapshot test starts a run"));
        if (game != nullptr) {
            const int savedWallet = ShopService.score().total();
            const int savedKills = ShopService.score().kills();
            ShopService.score().total_ = 2840;
            ShopService.score().kills_ = 31;
            game->state_ = Games::State::Upgrade;
            game->runTime_ = 42.5f;
            game->waves_.start(3);
            game->waves_.queue_ = {
                arena::EnemyType(typeid(arena::DroneEnemy)),
                arena::EnemyType(typeid(arena::SpitterEnemy))};
            game->waves_.waveTotal_ = 12;
            game->arena_.enemies().clear();
            game->arena_.portals().clear();
            game->arena_.playerBullets().clear();
            game->arena_.clearEnemy();
            game->arena_.particles().clear();
            game->arena_.floatingTexts().clear();
            game->arena_.shockwaves().clear();
            game->arena_.pickups().clear();
            game->arena_.mines().clear();

            const arena::Context snapshotCtx = game->arena_.context();
            game->arena_.playerState().pos = {475.0f, 430.0f};
            game->arena_.playerState().hp = 74.0f;
            game->arena_.playerState().hpDisplay = 84.0f;
            game->arena_.playerState().aim = -0.55f;
            game->arena_.playerState().multishot = 2;
            game->arena_.spawnEnemy(snapshotCtx, QStringLiteral("drone"), {820.0f, 285.0f}, 3);
            game->arena_.spawnEnemy(snapshotCtx, QStringLiteral("amplifier"),
                                    {860.0f, 340.0f}, 3);
            game->arena_.spawnEnemy(snapshotCtx, QStringLiteral("boss"),
                                    {870.0f, 365.0f}, 3);
            if (game->arena_.boss() != nullptr) {
                game->arena_.boss()->hp = game->arena_.boss()->maxHp * 0.63f;
                game->arena_.boss()->hpDisplay = game->arena_.boss()->maxHp * 0.74f;
            }
            game->arena_.enemies().front()->damageBuffTimer = 0.4f;
            game->arena_.enemies().front()->damageBuffBonus = 0.3f;
            game->arena_.portals().emplace_back(
                arena::EnemyType(typeid(arena::SpitterEnemy)), Math::Vec2(1160, 615), 3);
            game->arena_.spawnPickup(snapshotCtx, {360.0f, 315.0f}, 16);
            game->arena_.placeMine(Math::Vec2(520.0f, 430.0f));
            game->arena_.spawnBurst(snapshotCtx, {860.0f, 290.0f}, Base::Coral,
                                    14, 130.0f, 0.7f);
            game->arena_.addShockwave({860.0f, 290.0f}, 48.0f, Base::Coral, 0.6f, 2);
            game->arena_.floatingTexts().push_back(arena::FloatingText{
                {700.0f, 300.0f}, QStringLiteral("-17"), Base::Red,
                0.62f, 0.62f, 15, true});
            game->arena_.spawnStandard({650.0f, 360.0f}, {900.0f, -250.0f}, 12.5f,
                                       Base::Cyan, Bullet::Standpoint::Player, 3.2f);
            game->arena_.spawnStandard({850.0f, 300.0f}, {-420.0f, 150.0f}, 11.0f,
                                       Base::Magenta, Bullet::Standpoint::Enemy, 4.0f);
            game->upgrades_.open();

            const net::Snapshot original = game->snapshot();
            success &= require(original.state == net::RunState::Upgrade
                                   && original.runTime == game->runTime()
                                   && original.score == 2840 && original.kills == 31,
                               QStringLiteral("snapshot carries match state, time and score"));
            success &= require(original.enemies.size() == 3
                                   && original.enemies.front().dataKey
                                          == QStringLiteral("drone")
                                   && original.bullets.size() == 2
                                   && original.portals.size() == 1
                                   && original.pickups.size() == 1
                                   && original.mines.size() == 1
                                   && !original.particles.empty()
                                   && original.floatingTexts.size() == 1
                                   && original.shockwaves.size() == 1,
                               QStringLiteral("snapshot includes enemies, bullets and entities"));
            success &= require(original.waves.currentWave == 3
                                   && original.waves.totalWaves == 8
                                   && original.waves.remainingThreats > 0
                                   && original.upgrades.size() == 3
                                   && original.boss.active,
                               QStringLiteral("snapshot includes waves, upgrade offers and boss"));

            const QByteArray payload = net::serializeSnapshot(original);
            net::Snapshot decoded;
            QString snapshotError;
            const bool decodedOk =
                net::deserializeSnapshot(payload, &decoded, &snapshotError);
            success &= require(decodedOk,
                               QStringLiteral("snapshot deserializes (%1)").arg(snapshotError));
            success &= require(decodedOk && decoded == original,
                               QStringLiteral("snapshot roundtrip is field-exact"));
            net::Snapshot mutated = decoded;
            if (!mutated.enemies.empty()) {
                mutated.enemies.front().hp += 1.0f;
            }
            success &= require(!(mutated == decoded),
                               QStringLiteral("snapshot comparison notices field changes"));

            const QByteArray frame = net::encodeSnapshotMessage(original);
            net::Snapshot framed;
            snapshotError.clear();
            const bool framedOk =
                net::decodeSnapshotMessage(frame, &framed, &snapshotError);
            success &= require(framedOk && framed == original,
                               QStringLiteral("snapshot message roundtrips with framing"));
            const QByteArray wrongVersion = net::frameMessage(
                net::MessageType::Snapshot, net::serializeSnapshot(original),
                quint16(net::kProtocolVersion + 1));
            net::Snapshot rejected;
            snapshotError.clear();
            success &= require(!net::decodeSnapshotMessage(wrongVersion, &rejected,
                                                          &snapshotError)
                                   && !snapshotError.isEmpty(),
                               QStringLiteral("protocol version mismatch is rejected"));

            client::GameView view;
            const Random::EngineState rngBeforeView = RandomService.engineState();
            view.apply(decoded);
            success &= require(RandomService.engineState() == rngBeforeView,
                               QStringLiteral("view rebuild leaves the simulation RNG untouched"));
            success &= require(view.state() == original.state
                                   && view.runTime() == original.runTime
                                   && view.score() == original.score
                                   && view.kills() == original.kills,
                               QStringLiteral("view restores match state and score"));
            success &= require(view.playerState().hp == original.player.hp
                                   && view.playerState().pos == original.player.pos
                                   && view.world().aliveEnemyCount()
                                          == int(original.enemies.size()),
                               QStringLiteral("view restores player and enemies from snapshot"));
            success &= require(view.waves().currentWave == original.waves.currentWave
                                   && view.waves().remainingThreats
                                          == original.waves.remainingThreats
                                   && view.upgrades().size() == original.upgrades.size()
                                   && view.bossInfo().active == original.boss.active,
                               QStringLiteral("view restores HUD, offers and boss bar"));
            success &= require(view.world().portals().size() == original.portals.size()
                                   && view.world().pickups().size() == original.pickups.size()
                                   && view.world().mines().size() == original.mines.size()
                                   && view.world().particles().size()
                                          == original.particles.size()
                                   && view.world().floatingTexts().size()
                                          == original.floatingTexts.size()
                                   && view.world().shockwaves().size()
                                          == original.shockwaves.size(),
                               QStringLiteral("view restores portals, pickups, mines and effects"));

            games::Renderer viewRenderer;
            viewRenderer.initialize();
            viewRenderer.render(view);
            success &= require(viewRenderer.frame().size()
                                   == QSize(GameConfig::ScreenWidth,
                                            GameConfig::ScreenHeight),
                               QStringLiteral("snapshot-built GameView renders a frame"));

            game->render();
            success &= require(game->view().playerState().pos
                                   == game->arena_.playerState().pos
                                   && game->view().world().aliveEnemyCount()
                                          == game->arena_.aliveEnemyCount()
                                   && game->view().score() == 2840,
                               QStringLiteral("local render path goes Simulation -> Snapshot -> View"));
            ShopService.score().total_ = savedWallet;
            ShopService.score().kills_ = savedKills;
            menu.returnToMenu();
        }

        log(QStringLiteral("TCP loopback"));
        // 真实 QTcpServer/QTcpSocket 回环：握手、StartMatch、输入、快照广播。
        auto pumpUntil = [](const std::function<bool()> &predicate, int timeoutMs) {
            QElapsedTimer timer;
            timer.start();
            while (!predicate() && timer.elapsed() < timeoutMs) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
                QThread::msleep(1);
            }
            return predicate();
        };

        server::GameServer tcpServer;
        QString tcpError;
        const bool tcpListening = tcpServer.listen(0, true, &tcpError);
        success &= require(tcpListening && tcpServer.port() != 0,
                           QStringLiteral("TCP server listens on an automatic loopback port (%1)")
                               .arg(tcpError));
        if (tcpListening) {
            Games::Level tcpLevel;
            tcpLevel.name = QStringLiteral("TCP 回环测试");
            tcpLevel.startWave = 1;
            tcpLevel.enemyLevel = 1;
            tcpLevel.script.push_back(games::Wave{{games::EnemySpawn{
                arena::EnemyType(typeid(arena::DroneEnemy)), 1}}});

            client::GameClient tcpClient;
            success &= require(tcpClient.connectToHost(QStringLiteral("127.0.0.1"),
                                                       tcpServer.port()),
                               QStringLiteral("TCP client starts connecting"));
            success &= require(pumpUntil([&] { return tcpClient.welcomed(); }, 3000),
                               QStringLiteral("TCP client completes Hello/Welcome handshake"));
            tcpClient.startMatch(tcpLevel);
            success &= require(pumpUntil([&] { return tcpClient.snapshotCount() > 0; }, 3000),
                               QStringLiteral("TCP client receives snapshots"));
            success &= require(tcpServer.clientCount() == 1
                                   && tcpServer.snapshot().waves.totalWaves == 1,
                               QStringLiteral("server owns the authoritative match"));
            success &= require(tcpClient.lastSnapshot().waves.totalWaves == 1
                                   && tcpClient.view().playerState().pos
                                          == tcpClient.lastSnapshot().player.pos
                                   && tcpClient.view().waves().currentWave
                                          == tcpClient.lastSnapshot().waves.currentWave,
                               QStringLiteral("TCP snapshot rebuilds the client GameView"));

            const float beforeX = tcpClient.view().playerState().pos.x;
            net::InputCommand move;
            move.moveX = 1.0f;
            move.mouseActive = true;
            move.mouseX = beforeX + 220.0f;
            move.mouseY = tcpClient.view().playerState().pos.y;
            success &= require(tcpClient.sendInput(move),
                               QStringLiteral("input command is written to the TCP socket"));
            success &= require(pumpUntil([&] {
                                   return tcpClient.view().playerState().pos.x > beforeX + 5.0f;
                               }, 3000),
                               QStringLiteral("server applies the input command and broadcasts the result"));
            success &= require(tcpServer.snapshot().player.pos.x > beforeX + 5.0f,
                               QStringLiteral("authoritative simulation moved the player"));

            client::GameClient watcher;
            success &= require(watcher.connectToHost(QStringLiteral("127.0.0.1"),
                                                     tcpServer.port()),
                               QStringLiteral("a second client starts connecting"));
            success &= require(pumpUntil([&] {
                                   return watcher.welcomed() && watcher.snapshotCount() > 0;
                               }, 3000),
                               QStringLiteral("second client receives broadcast snapshots"));
            success &= require(tcpServer.clientCount() == 2,
                               QStringLiteral("server accepts two clients at once"));

            server::GameServer conflict;
            QString conflictError;
            success &= require(!conflict.listen(tcpServer.port(), true, &conflictError)
                                   && !conflictError.isEmpty(),
                               QStringLiteral("port in use is reported instead of crashing"));

            QTcpSocket rawClient;
            rawClient.connectToHost(QStringLiteral("127.0.0.1"), tcpServer.port());
            pumpUntil([&] {
                return rawClient.state() == QAbstractSocket::ConnectedState;
            }, 2000);
            const QByteArray wrongHello = net::frameMessage(
                net::MessageType::Hello, QByteArray(),
                quint16(net::kProtocolVersion + 1));
            rawClient.write(wrongHello);
            rawClient.flush();
            success &= require(pumpUntil([&] {
                                   return rawClient.state()
                                          == QAbstractSocket::UnconnectedState;
                               }, 3000),
                               QStringLiteral("server closes a client with a mismatched protocol"));
            success &= require(tcpServer.rejectedClients() >= 1,
                               QStringLiteral("protocol mismatch is rejected without crashing"));

            tcpClient.close();
            watcher.close();
            success &= require(pumpUntil([&] { return tcpServer.clientCount() == 0; }, 3000),
                               QStringLiteral("server drops sessions when clients disconnect"));

            const quint16 closedPort = tcpServer.port();
            tcpServer.close();
            pumpUntil([&] { return !tcpServer.isListening(); }, 1000);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            client::GameClient deadClient;
            deadClient.setAutoReconnect(false);
            success &= require(deadClient.connectToHost(QStringLiteral("127.0.0.1"),
                                                        closedPort),
                               QStringLiteral("client can attempt a closed-port connection"));
            success &= require(pumpUntil([&] { return deadClient.failed(); }, 8000),
                               QStringLiteral("connection failure is reported without crashing"));
            success &= require(!deadClient.lastError().isEmpty(),
                               QStringLiteral("connection failure carries an error message"));
        }

        log(QStringLiteral("Reconnect path"));
        {
            server::GameServer reconnectServer;
            QString reconnectError;
            const bool reconnectListening = reconnectServer.listen(0, true, &reconnectError);
            success &= require(reconnectListening,
                               QStringLiteral("reconnect test server listens (%1)")
                                   .arg(reconnectError));
            if (reconnectListening) {
                client::GameClient reconnectClient;
                reconnectClient.setAutoReconnect(true);
                success &= require(reconnectClient.connectToHost(
                                       QStringLiteral("127.0.0.1"),
                                       reconnectServer.port()),
                                   QStringLiteral("reconnect test client starts connecting"));
                success &= require(pumpUntil([&] { return reconnectClient.welcomed(); }, 3000),
                                   QStringLiteral("reconnect test client completes handshake"));
                reconnectServer.close();
                success &= require(pumpUntil([&] {
                                       return reconnectClient.reconnectAttempts() >= 1;
                                   }, 3000),
                                   QStringLiteral("disconnect schedules a bounded reconnect"));
                reconnectClient.close();
            }
        }

        log(QStringLiteral("Embedded single-player network path"));
        Menu networkMenu(false, false);
        networkMenu.startLevel(0);
        client::GameClient *netClient = networkMenu.match_.gameClient();
        success &= require(netClient != nullptr && networkMenu.match_.localServer() != nullptr,
                           QStringLiteral("default menu path starts an embedded loopback server"));
        success &= require(pumpUntil([&] {
                               return netClient != nullptr && netClient->snapshotCount() > 0;
                           }, 4000),
                           QStringLiteral("embedded server and client exchange snapshots"));
        if (netClient != nullptr) {
            success &= require(netClient->view().waves().totalWaves == 8,
                               QStringLiteral("embedded match runs the submitted campaign script"));
            networkMenu.update(1.0f / 60.0f);
            networkMenu.render();
            success &= require(networkMenu.match_.frame().size()
                                   == QSize(GameConfig::ScreenWidth,
                                            GameConfig::ScreenHeight),
                               QStringLiteral("embedded network match renders a frame"));
        }
        networkMenu.returnToMenu();

        log(success ? QStringLiteral("ALL C++ SMOKE TESTS PASSED")
                    : QStringLiteral("SMOKE TESTS FAILED"));
        logFile.close();
        return success ? 0 : 1;
    }

    // 性能基准（--bench）：固定场景下测 render / update 的每帧耗时。
    // 用于对比优化前后（配合 NEON_SINGLE_THREAD=1 做单线程对照）。
    static int bench()
    {
        Menu menu(false, true);
        menu.startLevel(0);
        Games *game = menu.match_.game();
        if (game == nullptr) {
            return 1;
        }

        Arena &arena = game->arena_;
        auto ctx = arena.context();
        game->state_ = Games::State::Playing;
        game->runTime_ = 112.0f;
        game->waves_.start(4);
        game->waves_.queue_ = {arena::EnemyType(typeid(arena::DroneEnemy)),
                               arena::EnemyType(typeid(arena::SpitterEnemy))};
        game->waves_.waveTotal_ = 17;

        arena.enemies().clear();
        arena.portals().clear();
        arena.pickups().clear();
        arena.playerBullets().clear();
        arena.clearEnemy();
        arena.particles().clear();
        arena.floatingTexts().clear();
        arena.shockwaves().clear();

        Arena::PlayerState &player = arena.playerState();
        player.pos = {475.0f, 430.0f};
        player.hp = 74.0f;
        player.hpDisplay = 84.0f;
        player.aim = -0.55f;
        player.invuln = 99.0f;
        BaseService.mouseMove({1010.0f, 265.0f});

        const std::array<const char *, 12> kinds = {
            "drone", "drone", "drone", "drone", "spitter", "spitter",
            "tank", "splitter", "amplifier", "mender", "linker", "summoner"};
        const std::array<Math::Vec2, 12> spots = {{
            {820, 285}, {960, 520}, {700, 165}, {1080, 360},
            {950, 210}, {1120, 470}, {760, 445}, {600, 210},
            {860, 340}, {930, 470}, {790, 520}, {690, 300}}};
        for (int i = 0; i < 12; ++i) {
            arena.spawnEnemy(ctx, QString::fromLatin1(kinds[size_t(i)]), spots[size_t(i)], 4);
        }
        arena.portals().emplace_back(arena::EnemyType(typeid(arena::SpitterEnemy)),
                                     Math::Vec2(1160, 615), 4);
        arena.spawnPickup(ctx, {360, 315}, 16);
        arena.player().shoot(ctx);
        arena.spawnBurst(ctx, {860, 290}, Base::Coral, 14, 130.0f, 0.7f);
        arena.spawnBurst(ctx, {715, 400}, Base::Cyan, 8, 100.0f, 0.5f);
        arena.addShockwave({860, 290}, 48.0f, Base::Coral, 0.6f, 2);
        for (int i = 0; i < 6; ++i) {
            arena.submit(std::make_unique<arena::StandardBullet>(
                Math::Vec2(650.0f + float(i) * 60.0f, 360.0f),
                Math::Vec2(900.0f, -250.0f + float(i) * 40.0f), 12.5f, Base::Cyan,
                Bullet::Standpoint::Player, 3.2f));
        }
        game->renderer_.showBanner(QStringLiteral("第 4 波"),
                                   QStringLiteral("敌方信号正在接近"), 2.0f, Base::Cyan);
        game->renderer_.bannerTimer_ = 1.6f;

        for (int i = 0; i < 30; ++i) {
            game->render();
        }

        QElapsedTimer timer;
        constexpr int frames = 300;
        timer.start();
        for (int i = 0; i < frames; ++i) {
            game->render();
        }
        const double renderMs = double(timer.nsecsElapsed()) / 1e6 / frames;

        BaseService.mousePressLeft({1010.0f, 265.0f});
        constexpr int steps = 600;
        timer.restart();
        for (int i = 0; i < steps; ++i) {
            arena.playerState().invuln = 999.0f;
            arena.update(1.0f / 60.0f);
        }
        const double updateMs = double(timer.nsecsElapsed()) / 1e6 / steps;
        BaseService.mouseReleaseLeft();

        std::printf("BENCH frames=%d render=%.3f ms/frame update=%.3f ms/frame workers=%d pooled=%d\n",
                    frames, renderMs, updateMs, BaseService.parallelWorkers(),
                    arena.pooledBulletCount());
        std::fflush(stdout);
        return 0;
    }

    static int capture(const QString &directory)
    {
        QDir dir(directory);
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            return 1;
        }
        Menu menu(false, true);
        Games *game = nullptr;
        // 截图模拟窗口的合成顺序：画面 + 全局指针（BaseService 的 base::Pointer 组件）。
        auto movePointer = [&](const Math::Vec2 &position) {
            menu.mouseMove(position);
            BaseService.pointerMoveTo(position);
        };
        auto save = [&](const QString &name) {
            menu.render();
            const QString path = dir.filePath(name);
            QImage frame = menu.frame().copy();
            {
                QPainter overlay(&frame);
                BaseService.pointerDraw(overlay);
            }
            if (!frame.save(path)) {
                std::fprintf(stderr, "Failed to save %s\n", path.toUtf8().constData());
            } else {
                std::printf("%s\n", path.toUtf8().constData());
            }
        };

        for (int i = 0; i < 120; ++i) {
            menu.update(1.0f / 60.0f);
        }
        movePointer({GameConfig::ScreenWidth / 2.0f, 454.0f});
        save(QStringLiteral("title.png"));

        // 星区地图
        menu.keyPress(Qt::Key_Return);
        menu.keyRelease(Qt::Key_Return);
        save(QStringLiteral("map.png"));
        menu.keyPress(Qt::Key_Escape);
        menu.keyRelease(Qt::Key_Escape);

        // 商店
        ShopService.score().total_ = 2400;
        ShopService.inventory().addMine(2);
        const QRectF shopEntry = menu.main_.shopButtonRect();
        movePointer({float(shopEntry.center().x()), float(shopEntry.center().y())});
        menu.mousePressLeft({float(shopEntry.center().x()),
                             float(shopEntry.center().y())});
        save(QStringLiteral("shop.png"));
        menu.keyPress(Qt::Key_Escape);

        // 战备
        LoadoutService.select(1);
        const QRectF armoryEntry = menu.main_.armoryButtonRect();
        movePointer({float(armoryEntry.center().x()),
                     float(armoryEntry.center().y())});
        menu.mousePressLeft({float(armoryEntry.center().x()),
                             float(armoryEntry.center().y())});
        menu.mouseReleaseLeft();
        save(QStringLiteral("armory.png"));
        menu.keyPress(Qt::Key_Escape);
        LoadoutService.select(0);

        // 应用图标（程序化绘制）
        Base::appIcon(256).save(dir.filePath(QStringLiteral("icon.png")));
        const QRectF settingsEntry = menu.main_.settingsButtonRect();
        menu.mousePressLeft({float(settingsEntry.center().x()),
                             float(settingsEntry.center().y())});
        menu.mouseReleaseLeft();
        save(QStringLiteral("settings.png"));
        menu.keyPress(Qt::Key_Escape);

        const QRectF customCard = menu.main_.levelRect(Menu::CustomLevelIndex, int(menu.levels_.size()));
        movePointer({float(customCard.center().x()), float(customCard.center().y())});
        menu.update(1.0f / 60.0f);
        save(QStringLiteral("custom_level.png"));

        menu.keyPress(Qt::Key_Return);
        for (int i = 0; i < 2; ++i) {
            menu.keyPress(Qt::Key_N);
        }
        menu.keyPress(Qt::Key_Down);
        menu.keyPress(Qt::Key_Right);
        for (int i = 0; i < 3; ++i) {
            menu.keyPress(Qt::Key_Plus);
        }
        const QRectF spitterChip = menu.editor_.paletteChipRect(1);
        const QRectF dropRow = menu.editor_.rowRect(1);
        menu.mousePressLeft({float(spitterChip.center().x()),
                             float(spitterChip.center().y())});
        menu.mouseMove({float(dropRow.center().x()), float(dropRow.center().y())});
        menu.mouseReleaseLeft();
        save(QStringLiteral("editor.png"));
        menu.keyPress(Qt::Key_Return);
        menu.serverSetup_.setMode(menu::ServerSetup::Mode::Host);
        menu.serverSetup_.setPort(8899);
        movePointer({640, 378});
        save(QStringLiteral("server_setup.png"));
        menu.keyPress(Qt::Key_Escape);
        menu.keyPress(Qt::Key_Escape);

        menu.startLevel(0);
        game = menu.match_.game();
        game->state_ = Games::State::Playing;
        game->runTime_ = 112.0f;
        game->waves_.start(4);
        game->waves_.queue_ = {arena::EnemyType(typeid(arena::DroneEnemy)),
                               arena::EnemyType(typeid(arena::SpitterEnemy))};
        game->waves_.waveTotal_ = 17;
        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.pickups().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        game->arena_.particles().clear();
        game->arena_.floatingTexts().clear();
        game->arena_.shockwaves().clear();
        ShopService.score().total_ = 2840;
        ShopService.score().kills_ = 31;
        Arena::PlayerState &player = game->arena_.player().state;
        player.pos = {475, 430};
        player.hp = 74;
        player.hpDisplay = 84;
        player.aim = -0.55f;
        player.invuln = 99.0f;
        movePointer({1010, 265});

        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {820, 285}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {960, 520}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {700, 165}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {1080, 360}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("spitter"), {950, 210}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("spitter"), {1120, 470}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("tank"), {760, 445}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("splitter"), {600, 210}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("amplifier"), {860, 340}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("mender"), {930, 470}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("linker"), {790, 520}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("rusher"), {1050, 250}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("necromancer"), {1150, 560}, 4);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("summoner"), {690, 300}, 4);
        int enemyIndex = 0;
        for (const auto &enemy : game->arena_.enemies()) {
            enemy->age = 2.0f + float(enemyIndex) * 0.3f;
            if (enemyIndex < 3) {
                enemy->hp = enemy->maxHp * 0.55f;
            }
            ++enemyIndex;
        }
        game->arena_.portals().emplace_back(
            arena::EnemyType(typeid(arena::SpitterEnemy)), Math::Vec2(1160, 615), 4);
        game->arena_.spawnPickup(game->arena_.context(), {360, 315}, 16);
        game->arena_.player().shoot(game->arena_.context());
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(650, 360), Math::Vec2(900, -250), 12.5f, Base::Cyan, Bullet::Standpoint::Player, 3.2f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(590, 325), Math::Vec2(950, -180), 12.5f, Base::Cyan, Bullet::Standpoint::Player, 3.2f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(720, 400), Math::Vec2(880, -60), 12.5f, Base::Cyan, Bullet::Standpoint::Player, 3.2f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(850, 300), Math::Vec2(-420, 150), 11.0f, Base::Magenta, Bullet::Standpoint::Enemy, 4.0f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(1010, 510), Math::Vec2(-380, -90), 11.0f, Base::Magenta, Bullet::Standpoint::Enemy, 4.0f));
        game->arena_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(1090, 420), Math::Vec2(-440, -30), 13.0f, QColor(255, 112, 190), Bullet::Standpoint::Enemy, 5.0f));
        game->arena_.spawnBurst(game->arena_.context(), {860, 290}, Base::Coral, 14, 130.0f, 0.7f);
        game->arena_.spawnBurst(game->arena_.context(), {715, 400}, Base::Cyan, 8, 100.0f, 0.5f);
        game->arena_.addShockwave({860, 290}, 48.0f, Base::Coral, 0.6f, 2);
        game->renderer_.showBanner(QStringLiteral("第 4 波"),
                                  QStringLiteral("敌方信号正在接近"), 1.8f, Base::Cyan);
        game->renderer_.bannerTimer_ = 1.3f;
        game->arena_.placeMine(Math::Vec2(520, 430));
        save(QStringLiteral("gameplay.png"));

        game->arena_.enemies().clear();
        game->arena_.portals().clear();
        game->arena_.pickups().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        game->arena_.particles().clear();
        game->arena_.floatingTexts().clear();
        game->arena_.shockwaves().clear();
        game->arena_.player().state.pos = {585, 420};
        game->waves_.start(4);
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 15;
        game->enterUpgrade();
        movePointer({640, 390});
        save(QStringLiteral("upgrade.png"));

        menu.startLevel(0);
        game = menu.match_.game();
        game->state_ = Games::State::Playing;
        game->waves_.start(8);
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 16;
        game->arena_.beginWave();
        game->runTime_ = 304.0f;
        ShopService.score().total_ = 9870;
        ShopService.score().kills_ = 63;
        game->arena_.enemies().clear();
        game->arena_.playerBullets().clear();
        game->arena_.clearEnemy();
        game->arena_.particles().clear();
        game->arena_.shockwaves().clear();
        Arena::PlayerState &bossPlayer = game->arena_.player().state;
        bossPlayer.pos = {430, 435};
        bossPlayer.hp = 58;
        bossPlayer.hpDisplay = 71;
        bossPlayer.aim = -0.25f;
        bossPlayer.invuln = 99.0f;
        movePointer({850, 380});
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("boss"), {870, 365}, 8);
        game->arena_.boss()->hp =
            game->arena_.boss()->maxHp * 0.63f;
        game->arena_.boss()->hpDisplay =
            game->arena_.boss()->maxHp * 0.74f;
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {680, 250}, 8);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("drone"), {740, 540}, 8);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("tank"), {1020, 500}, 8);
        game->arena_.spawnEnemy(game->arena_.context(), QStringLiteral("spitter"), {1100, 300}, 8);
        game->arena_.spawnPickup(game->arena_.context(), {330, 285}, 16);
        for (int i = 0; i < 12; ++i) {
            const float angle = i * Math::Tau / 12.0f;
            const Math::Vec2 direction(std::cos(angle), std::sin(angle));
            game->arena_.submit(std::make_unique<arena::StandardBullet>(game->arena_.boss()->pos + direction * 58.0f,
                direction * 285.0f, 13.0f, Base::Magenta, Bullet::Standpoint::Enemy, 5.0f));
        }
        bossPlayer.fireFlash = 0.04f;
        game->arena_.player().shoot(game->arena_.context());
        game->arena_.spawnBurst(game->arena_.context(), game->arena_.boss()->pos,
                                         Base::Magenta, 12, 150.0f, 0.8f);
        game->renderer_.showBanner(QStringLiteral("最终波次"),
                                  QStringLiteral("警告：检测到「虚空吞噬者」"),
                                  2.8f, Base::Magenta, QStringLiteral("FINAL WAVE"));
        game->renderer_.bannerTimer_ = 0.0f;
        save(QStringLiteral("boss.png"));

        if (game->arena_.boss() != nullptr) {
            game->arena_.boss()->takeDamage(
                game->arena_.context(), arena::Damage{999999.0f, false, arena::DamageSource::Bullet});
        }
        // 胜负由台本决定：Boss 被毁只是战果，最后一波清空才进入结算。
        game->triggerVictory();
        for (int i = 0; i < 120; ++i) {
            game->update(1.0f / 60.0f);
        }
        menu.update(0.0f);
        save(QStringLiteral("victory.png"));

        menu.startLevel(0);
        game = menu.match_.game();
        game->state_ = Games::State::Playing;
        game->waves_.start(3);
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 12;
        game->runTime_ = 86.0f;
        ShopService.score().total_ = 1240;
        ShopService.score().kills_ = 17;
        game->arena_.player().state.hp = 8;
        game->arena_.player().state.invuln = 0.0f;
        game->arena_.player().takeDamage(
            game->arena_.context(), arena::Damage{20.0f, false, arena::DamageSource::Bullet});
        for (int i = 0; i < 30; ++i) {
            game->update(1.0f / 60.0f);
        }
        menu.update(0.0f);
        save(QStringLiteral("game_over.png"));
        return 0;
    }
};

#endif // DEBUG
