#pragma once

// 测试专用代码：只在 -DDEBUG 构建中由 main.cpp 包含。
// 交付的 NeonCore.exe 不包含这里的任何逻辑。

#include "Menu/Menu.h"
#include "Base/Base.h"
#include "Pointer.h"
#include "Data/GameData.h"
#include "Arena/Bullets/StandardBullet.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <cstdio>

#ifdef DEBUG

class SelfTest {
public:
    static void simulate(Games &game, float seconds, bool shoot)
    {
        const int steps = int(seconds * 60.0f);
        for (int i = 0; i < steps; ++i) {
            if (game.state() == Games::State::Playing && shoot
                && game.arena_.player().alive) {
                game.arena_.combatants_.player().state.invuln =
                    std::max(game.arena_.combatants_.player().state.invuln, 2.0f);
                arena::Enemy *target = nullptr;
                float best = 1e9f;
                for (const auto &enemy : game.arena_.combatants_.enemies()) {
                    if (!enemy->alive) {
                        continue;
                    }
                    const float distance = enemy->pos.distanceSquaredTo(
                        game.arena_.player().pos);
                    if (distance < best) {
                        best = distance;
                        target = enemy.get();
                    }
                }
                if (target != nullptr) {
                    game.base_.mouseMove(target->pos);
                    game.base_.mousePressLeft(target->pos);
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
                game.arena_.combatants_.player().state.invuln = 999.0f;
                arena::Enemy *target = nullptr;
                float best = 1e9f;
                Math::Vec2 dodge;
                for (const auto &enemy : game.arena_.combatants_.enemies()) {
                    if (!enemy->alive) {
                        continue;
                    }
                    const float distance = enemy->pos.distanceSquaredTo(
                        game.arena_.player().pos);
                    if (distance < best) {
                        best = distance;
                        target = enemy.get();
                    }
                    const float realDistance = std::sqrt(distance);
                    if (realDistance > 1.0f && realDistance < 330.0f) {
                        const Math::Vec2 away =
                            (game.arena_.player().pos - enemy->pos).normalized();
                        dodge += away * ((330.0f - realDistance) / 330.0f);
                    }
                }
                if (target != nullptr) {
                    game.base_.mouseMove(target->pos);
                    game.base_.mousePressLeft(target->pos);
                }
                const Math::Vec2 center(GameConfig::ScreenWidth / 2.0f,
                                        GameConfig::ScreenHeight / 2.0f);
                dodge += (center - game.arena_.player().pos) * 0.0025f;
                if (dodge.lengthSquared() < 0.01f) {
                    dodge = Math::Vec2(1.0f, 0.0f).rotated(game.runTime());
                }
                game.base_.keyRelease(Qt::Key_A);
                game.base_.keyRelease(Qt::Key_D);
                game.base_.keyRelease(Qt::Key_W);
                game.base_.keyRelease(Qt::Key_S);
                if (dodge.x < -0.12f) {
                    game.base_.keyPress(Qt::Key_A);
                } else if (dodge.x > 0.12f) {
                    game.base_.keyPress(Qt::Key_D);
                }
                if (dodge.y < -0.12f) {
                    game.base_.keyPress(Qt::Key_W);
                } else if (dodge.y > 0.12f) {
                    game.base_.keyPress(Qt::Key_S);
                }
                if (std::sqrt(best) < 95.0f && game.arena_.player().dashCd <= 0.0f) {
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
        Menu menu(audioRequested);
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
        game = menu.match_.game();
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("enter starts the run"));
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
        game->arena_.entities_.portals().clear();
        game->arena_.combatants_.enemies().clear();
        game->arena_.bullets_.clearEnemy();
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

        const Arena::PlayerState before = game->arena_.player();
        game->selectUpgrade(0);
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("upgrade resumes play"));
        success &= require(game->waves_.currentWave() == 2,
                           QStringLiteral("advances to wave 2"));
        success &= require(before.damage != game->arena_.player().damage
                               || before.fireCd != game->arena_.player().fireCd
                               || before.maxHp != game->arena_.player().maxHp
                               || before.hp != game->arena_.player().hp
                               || before.speed != game->arena_.player().speed
                               || before.multishot != game->arena_.player().multishot
                               || before.pierce != game->arena_.player().pierce
                               || before.critChance != game->arena_.player().critChance
                               || before.shockLevel != game->arena_.player().shockLevel,
                           QStringLiteral("upgrade changes a stat"));

        log(QStringLiteral("Boss and victory"));
        menu.startLevel(0);
        game = menu.match_.game();
        game->waves_.start(8);
        game->waves_.queue_.clear();
        game->arena_.beginWave();
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("boss"),
                               Math::Vec2(GameConfig::ScreenWidth / 2.0f, 220.0f), 8);
        success &= require(game->arena_.combatants_.boss() != nullptr, QStringLiteral("boss spawns"));
        game->arena_.combatants_.boss()->takeDamage(
            game->arena_, arena::Damage{99999.0f, false, arena::DamageSource::Bullet});
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
        bossMid.push_back(games::Wave{{games::EnemySpawn{QStringLiteral("drone"), 1}}});
        bossMid.push_back(games::Wave{{games::EnemySpawn{QStringLiteral("boss"), 1}}});
        bossMid.push_back(games::Wave{{games::EnemySpawn{QStringLiteral("drone"), 1}}});
        menu.levels_.push_back({QStringLiteral("boss-mid"), QStringLiteral("中局 Boss"),
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
        game->arena_.combatants_.enemies().clear();
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("boss"),
                               Math::Vec2(GameConfig::ScreenWidth / 2.0f, 220.0f), 2);
        game->arena_.combatants_.boss()->takeDamage(
            game->arena_, arena::Damage{99999.0f, false, arena::DamageSource::Bullet});
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
        game->arena_.combatants_.player().state.invuln = 0.0f;
        game->arena_.combatants_.player().takeDamage(
            game->arena_, arena::Damage{9999.0f, false, arena::DamageSource::Bullet});
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
        menu.keyPress(Qt::Key_Return);
        game = menu.match_.game();
        success &= require(menu.screen() == Menu::Screen::Match
                               && game->totalWaves() == editedWaves + 2
                               && game->currentWave() == 1,
                           QStringLiteral("edited script drives the run from wave 1"));
        menu.returnToMenu();

        log(QStringLiteral("Script editor"));
        menu.keyPress(Qt::Key_2);
        for (int i = 0; i < 6; ++i) {
            menu.keyPress(Qt::Key_Right);
        }
        menu.keyPress(Qt::Key_Plus);
        success &= require(menu.editor_.waveHasBoss(menu.editor_.row()),
                           QStringLiteral("editor toggles the boss wave"));
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
        menu.base_.setSoundVolume(0.5f);
        menu.base_.setMusicVolume(0.5f);
        menu.keyPress(Qt::Key_Down);
        menu.keyPress(Qt::Key_Left);
        success &= require(menu.base_.musicVolume() < 0.5f,
                           QStringLiteral("left lowers the music volume"));
        menu.keyPress(Qt::Key_Right);
        success &= require(std::abs(menu.base_.musicVolume() - 0.5f) < 0.001f,
                           QStringLiteral("right raises the music volume"));
        menu.keyPress(Qt::Key_Up);
        menu.keyPress(Qt::Key_Left);
        success &= require(menu.base_.soundVolume() < 0.5f,
                           QStringLiteral("left lowers the sound volume"));
        const QRectF musicTrack = menu.settings_.trackRect(1);
        menu.keyPress(Qt::Key_Down);
        menu.mousePressLeft({float(musicTrack.left()) + float(musicTrack.width()) * 0.25f,
                             float(musicTrack.center().y())});
        menu.mouseReleaseLeft();
        success &= require(std::abs(menu.base_.musicVolume() - 0.25f) < 0.02f,
                           QStringLiteral("clicking the track sets the volume"));
        menu.keyPress(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves settings"));

        log(QStringLiteral("Score"));
        const int wallet = menu.shop_.score().total();
        menu.shop_.score().beginRun();
        menu.shop_.score().addKill(120);
        success &= require(menu.shop_.score().total() == wallet + 120
                               && menu.shop_.score().run() == 120 && menu.shop_.score().kills() == 1,
                           QStringLiteral("kill adds to global wallet and run total"));
        success &= require(menu.shop_.score().spend(50) && menu.shop_.score().total() == wallet + 70,
                           QStringLiteral("spend deducts from the wallet"));
        success &= require(!menu.shop_.score().spend(999999),
                           QStringLiteral("overdraft is rejected"));
        menu.shop_.score().beginRun();
        success &= require(menu.shop_.score().kills() == 0 && menu.shop_.score().run() == 0
                               && menu.shop_.score().total() == wallet + 70,
                           QStringLiteral("beginRun resets only the run stats"));
        menu.shop_.score().total_ = wallet;   // 还原，避免测试改动持久化余额

        log(QStringLiteral("Shop"));
        menu.shop_.score().total_ = 500;
        const int startMines = menu.shop_.inventory().mines();
        const QRectF shopButton = menu.main_.shopButtonRect();
        menu.mousePressLeft({float(shopButton.center().x()),
                             float(shopButton.center().y())});
        success &= require(menu.screen() == Menu::Screen::Shop,
                           QStringLiteral("shop button opens shop"));
        const QRectF buyButton = menu.depot_.buyRect();
        menu.mousePressLeft({float(buyButton.center().x()),
                             float(buyButton.center().y())});
        success &= require(menu.shop_.score().total() == 500 - data::kMine.price
                               && menu.shop_.inventory().mines() == startMines + 1,
                           QStringLiteral("buying a mine spends score and adds stock"));
        menu.keyPress(Qt::Key_Escape);
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("escape leaves shop"));

        log(QStringLiteral("Mine"));
        menu.startLevel(0);
        Games *mineGame = menu.match_.game();
        success &= require(mineGame != nullptr, QStringLiteral("mine test starts a run"));
        if (mineGame != nullptr) {
            for (int i = 0; i < 240 && mineGame->arena_.combatants_.enemies().empty(); ++i) {
                mineGame->update(1.0f / 60.0f);
            }
            success &= require(!mineGame->arena_.combatants_.enemies().empty(),
                               QStringLiteral("wave spawns enemies for the mine test"));
            if (!mineGame->arena_.combatants_.enemies().empty()) {
                arena::Enemy *victim = mineGame->arena_.combatants_.enemies().front().get();
                const Math::Vec2 spot = victim->pos;
                const int stockBefore = menu.shop_.inventory().mines();
                menu.mousePressRight(spot);
                success &= require(mineGame->arena_.mineCount() == 1
                                       && menu.shop_.inventory().mines() == stockBefore - 1,
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

        log(QStringLiteral("Submitted script"));
        games::Script shortScript;
        shortScript.push_back(games::Wave{{games::EnemySpawn{QStringLiteral("drone"), 2},
                                           games::EnemySpawn{QStringLiteral("tank"), 1}}});
        shortScript.push_back(games::Wave{{games::EnemySpawn{QStringLiteral("spitter"), 3}}});
        menu.levels_.push_back({QStringLiteral("script-test"), QStringLiteral("测试台本"),
                                QStringLiteral("运行时提交"), 1, shortScript});
        menu.startLevel(int(menu.levels_.size()) - 1);
        game = menu.match_.game();
        success &= require(game->totalWaves() == 2 && game->currentWave() == 1
                               && game->waves_.queue_.size() == 3,
                           QStringLiteral("submitted script drives the run"));
        menu.returnToMenu();

        log(success ? QStringLiteral("ALL C++ SMOKE TESTS PASSED")
                    : QStringLiteral("SMOKE TESTS FAILED"));
        logFile.close();
        return success ? 0 : 1;
    }

    static int capture(const QString &directory)
    {
        QDir dir(directory);
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            return 1;
        }
        Menu menu(false);
        Games *game = nullptr;
        // 截图模拟窗口的合成顺序：画面 + 全局指针。
        Pointer pointer;
        auto movePointer = [&](const Math::Vec2 &position) {
            menu.mouseMove(position);
            pointer.moveTo(position);
        };
        auto save = [&](const QString &name) {
            menu.render();
            const QString path = dir.filePath(name);
            QImage frame = menu.frame().copy();
            {
                QPainter overlay(&frame);
                pointer.draw(overlay);
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

        // 商店
        menu.shop_.score().total_ = 2400;
        menu.shop_.inventory().addMine(2);
        const QRectF shopEntry = menu.main_.shopButtonRect();
        movePointer({float(shopEntry.center().x()), float(shopEntry.center().y())});
        menu.mousePressLeft({float(shopEntry.center().x()),
                             float(shopEntry.center().y())});
        save(QStringLiteral("shop.png"));
        menu.keyPress(Qt::Key_Escape);

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
        save(QStringLiteral("editor.png"));
        menu.keyPress(Qt::Key_Escape);

        menu.startLevel(0);
        game = menu.match_.game();
        game->state_ = Games::State::Playing;
        game->runTime_ = 112.0f;
        game->waves_.start(4);
        game->waves_.queue_ = {QStringLiteral("drone"), QStringLiteral("spitter")};
        game->waves_.waveTotal_ = 17;
        game->arena_.combatants_.enemies().clear();
        game->arena_.entities_.portals().clear();
        game->arena_.entities_.pickups().clear();
        game->arena_.bullets_.player().clear();
        game->arena_.bullets_.clearEnemy();
        game->arena_.entities_.particles().clear();
        game->arena_.entities_.floatingTexts().clear();
        game->arena_.entities_.shockwaves().clear();
        menu.shop_.score().total_ = 2840;
        menu.shop_.score().kills_ = 31;
        Arena::PlayerState &player = game->arena_.combatants_.player().state;
        player.pos = {475, 430};
        player.hp = 74;
        player.hpDisplay = 84;
        player.aim = -0.55f;
        player.invuln = 99.0f;
        movePointer({1010, 265});

        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("drone"), {820, 285}, 4);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("drone"), {960, 520}, 4);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("drone"), {700, 165}, 4);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("drone"), {1080, 360}, 4);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("spitter"), {950, 210}, 4);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("spitter"), {1120, 470}, 4);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("tank"), {760, 445}, 4);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("splitter"), {600, 210}, 4);
        int enemyIndex = 0;
        for (const auto &enemy : game->arena_.combatants_.enemies()) {
            enemy->age = 2.0f + float(enemyIndex) * 0.3f;
            if (enemyIndex < 3) {
                enemy->hp = enemy->maxHp * 0.55f;
            }
            ++enemyIndex;
        }
        game->arena_.entities_.portals().emplace_back(QStringLiteral("spitter"),
                                                     Math::Vec2(1160, 615), 4);
        game->arena_.entities_.spawnPickup(game->arena_, {360, 315}, 16);
        game->arena_.combatants_.player().shoot(game->arena_);
        game->arena_.bullets_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(650, 360), Math::Vec2(900, -250), 12.5f, Base::Cyan, Bullet::Standpoint::Player, 3.2f));
        game->arena_.bullets_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(590, 325), Math::Vec2(950, -180), 12.5f, Base::Cyan, Bullet::Standpoint::Player, 3.2f));
        game->arena_.bullets_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(720, 400), Math::Vec2(880, -60), 12.5f, Base::Cyan, Bullet::Standpoint::Player, 3.2f));
        game->arena_.bullets_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(850, 300), Math::Vec2(-420, 150), 11.0f, Base::Magenta, Bullet::Standpoint::Enemy, 4.0f));
        game->arena_.bullets_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(1010, 510), Math::Vec2(-380, -90), 11.0f, Base::Magenta, Bullet::Standpoint::Enemy, 4.0f));
        game->arena_.bullets_.submit(std::make_unique<arena::StandardBullet>(Math::Vec2(1090, 420), Math::Vec2(-440, -30), 13.0f, QColor(255, 112, 190), Bullet::Standpoint::Enemy, 5.0f));
        game->arena_.entities_.spawnBurst(game->arena_, {860, 290}, Base::Coral, 14, 130.0f, 0.7f);
        game->arena_.entities_.spawnBurst(game->arena_, {715, 400}, Base::Cyan, 8, 100.0f, 0.5f);
        game->arena_.entities_.addShockwave({860, 290}, 48.0f, Base::Coral, 0.6f, 2);
        game->renderer_.showBanner(QStringLiteral("第 4 波"),
                                  QStringLiteral("敌方信号正在接近"), 1.8f, Base::Cyan);
        game->renderer_.bannerTimer_ = 1.3f;
        game->arena_.placeMine(Math::Vec2(520, 430));
        save(QStringLiteral("gameplay.png"));

        game->arena_.combatants_.enemies().clear();
        game->arena_.entities_.portals().clear();
        game->arena_.entities_.pickups().clear();
        game->arena_.bullets_.player().clear();
        game->arena_.bullets_.clearEnemy();
        game->arena_.entities_.particles().clear();
        game->arena_.entities_.floatingTexts().clear();
        game->arena_.entities_.shockwaves().clear();
        game->arena_.combatants_.player().state.pos = {585, 420};
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
        menu.shop_.score().total_ = 9870;
        menu.shop_.score().kills_ = 63;
        game->arena_.combatants_.enemies().clear();
        game->arena_.bullets_.player().clear();
        game->arena_.bullets_.clearEnemy();
        game->arena_.entities_.particles().clear();
        game->arena_.entities_.shockwaves().clear();
        Arena::PlayerState &bossPlayer = game->arena_.combatants_.player().state;
        bossPlayer.pos = {430, 435};
        bossPlayer.hp = 58;
        bossPlayer.hpDisplay = 71;
        bossPlayer.aim = -0.25f;
        bossPlayer.invuln = 99.0f;
        movePointer({850, 380});
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("boss"), {870, 365}, 8);
        game->arena_.combatants_.boss()->hp =
            game->arena_.combatants_.boss()->maxHp * 0.63f;
        game->arena_.combatants_.boss()->hpDisplay =
            game->arena_.combatants_.boss()->maxHp * 0.74f;
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("drone"), {680, 250}, 8);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("drone"), {740, 540}, 8);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("tank"), {1020, 500}, 8);
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("spitter"), {1100, 300}, 8);
        game->arena_.entities_.spawnPickup(game->arena_, {330, 285}, 16);
        for (int i = 0; i < 12; ++i) {
            const float angle = i * Math::Tau / 12.0f;
            const Math::Vec2 direction(std::cos(angle), std::sin(angle));
            game->arena_.bullets_.submit(std::make_unique<arena::StandardBullet>(game->arena_.combatants_.boss()->pos + direction * 58.0f,
                direction * 285.0f, 13.0f, Base::Magenta, Bullet::Standpoint::Enemy, 5.0f));
        }
        bossPlayer.fireFlash = 0.04f;
        game->arena_.combatants_.player().shoot(game->arena_);
        game->arena_.entities_.spawnBurst(game->arena_, game->arena_.combatants_.boss()->pos,
                                         Base::Magenta, 12, 150.0f, 0.8f);
        game->renderer_.showBanner(QStringLiteral("最终波次"),
                                  QStringLiteral("警告：检测到「虚空吞噬者」"),
                                  2.8f, Base::Magenta, QStringLiteral("FINAL WAVE"));
        game->renderer_.bannerTimer_ = 0.0f;
        save(QStringLiteral("boss.png"));

        if (game->arena_.combatants_.boss() != nullptr) {
            game->arena_.combatants_.boss()->takeDamage(
                game->arena_, arena::Damage{999999.0f, false, arena::DamageSource::Bullet});
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
        menu.shop_.score().total_ = 1240;
        menu.shop_.score().kills_ = 17;
        game->arena_.combatants_.player().state.hp = 8;
        game->arena_.combatants_.player().state.invuln = 0.0f;
        game->arena_.combatants_.player().takeDamage(
            game->arena_, arena::Damage{20.0f, false, arena::DamageSource::Bullet});
        for (int i = 0; i < 30; ++i) {
            game->update(1.0f / 60.0f);
        }
        menu.update(0.0f);
        save(QStringLiteral("game_over.png"));
        return 0;
    }
};

#endif // DEBUG
