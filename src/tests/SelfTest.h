#pragma once

// 测试专用代码：只在 -DDEBUG 构建中由 main.cpp 包含。
// 交付的 NeonCore.exe 不包含这里的任何逻辑。

#include "Menu/Menu.h"
#include "Base/Base.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QPixmap>

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
        game = menu.games_.get();
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
        game = menu.games_.get();
        success &= require(game->state() == Games::State::Playing,
                           QStringLiteral("run starts"));
        success &= require(game->waves_.currentWave() == 1,
                           QStringLiteral("starts at wave 1"));
        simulate(*game, 7.0f, true);
        game->render();

        game->waves_.queue_.clear();
        game->arena_.entities_.portals().clear();
        game->arena_.combatants_.enemies().clear();
        game->arena_.entities_.enemyBullets().clear();
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
        game = menu.games_.get();
        game->waves_.start(8);
        game->waves_.queue_.clear();
        game->arena_.beginWave();
        game->arena_.combatants_.spawnEnemy(game->arena_, QStringLiteral("boss"),
                               Math::Vec2(GameConfig::ScreenWidth / 2.0f, 220.0f), 8);
        success &= require(game->arena_.combatants_.boss() != nullptr, QStringLiteral("boss spawns"));
        game->arena_.combatants_.boss()->takeDamage(
            game->arena_, arena::Damage{99999.0f, false, arena::DamageSource::Bullet});
        success &= require(game->arena_.bossDefeated(),
                           QStringLiteral("boss death starts victory sequence"));
        simulate(*game, 2.0f, false);
        success &= require(game->state() == Games::State::Victory,
                           QStringLiteral("victory screen reached"));
        menu.update(0.0f);
        success &= require(menu.screen() == Menu::Screen::Result,
                           QStringLiteral("menu enters victory result"));
        menu.render();

        log(QStringLiteral("Game over"));
        menu.startLevel(0);
        game = menu.games_.get();
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
        game = menu.games_.get();
        success &= require(game->state() == Games::State::Playing
                               && game->waves_.currentWave() == 1,
                           QStringLiteral("restart works"));

        log(QStringLiteral("Autoplay full 8-wave run"));
        menu.startLevel(0);
        game = menu.games_.get();
        const bool autoplayVictory = autoplay(*game, 240.0f);
        success &= require(autoplayVictory,
                           QStringLiteral("autoplay clears all 8 waves and the boss"));

        menu.returnToMenu();
        menu.update(1.0f / 60.0f);
        menu.render();
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("returns to main menu"));

        menu.keyPress(Qt::Key_2);
        game = menu.games_.get();
        success &= require(menu.screen() == Menu::Screen::Match
                               && game->currentWave() == 8,
                           QStringLiteral("boss-rush level starts at wave 8"));
        menu.returnToMenu();
        success &= require(menu.screen() == Menu::Screen::Main,
                           QStringLiteral("level 2 returns to menu"));

        log(QStringLiteral("Custom level"));
        menu.keyPress(Qt::Key_Down);
        for (int i = 0; i < Games::waveCount() + 4; ++i) {
            menu.keyPress(Qt::Key_Right);
        }
        menu.keyPress(Qt::Key_Return);
        game = menu.games_.get();
        success &= require(game->state() == Games::State::Playing
                               && game->currentWave() == Games::waveCount(),
                           QStringLiteral("custom level clamps to the last wave"));
        menu.returnToMenu();

        menu.keyPress(Qt::Key_Left);
        menu.keyPress(Qt::Key_Left);
        menu.keyPress(Qt::Key_Return);
        game = menu.games_.get();
        success &= require(game->currentWave() == Games::waveCount() - 2,
                           QStringLiteral("custom level starts at the chosen wave"));
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
        auto save = [&](const QString &name) {
            menu.render();
            const QString path = dir.filePath(name);
            if (!menu.frame().save(path)) {
                std::fprintf(stderr, "Failed to save %s\n", path.toUtf8().constData());
            } else {
                std::printf("%s\n", path.toUtf8().constData());
            }
        };

        for (int i = 0; i < 120; ++i) {
            menu.update(1.0f / 60.0f);
        }
        menu.mouseMove({GameConfig::ScreenWidth / 2.0f, 454.0f});
        save(QStringLiteral("title.png"));

        const QPixmap cursorPixmap = Base::cursor().pixmap();
        if (!cursorPixmap.isNull()) {
            cursorPixmap.save(dir.filePath(QStringLiteral("cursor.png")));
            cursorPixmap.scaled(cursorPixmap.size() * 8, Qt::KeepAspectRatio,
                                Qt::SmoothTransformation)
                .save(dir.filePath(QStringLiteral("cursor_zoom.png")));
        }

        menu.mouseMove({1020.0f, 454.0f});
        menu.update(1.0f / 60.0f);
        for (int i = 0; i < 3; ++i) {
            menu.keyPress(Qt::Key_Right);
        }
        save(QStringLiteral("custom_level.png"));

        menu.startLevel(0);
        game = menu.games_.get();
        game->state_ = Games::State::Playing;
        game->runTime_ = 112.0f;
        game->waves_.start(4);
        game->waves_.queue_ = {QStringLiteral("drone"), QStringLiteral("spitter")};
        game->waves_.waveTotal_ = 17;
        game->arena_.combatants_.enemies().clear();
        game->arena_.entities_.portals().clear();
        game->arena_.entities_.pickups().clear();
        game->arena_.entities_.playerBullets().clear();
        game->arena_.entities_.enemyBullets().clear();
        game->arena_.entities_.particles().clear();
        game->arena_.entities_.floatingTexts().clear();
        game->arena_.entities_.shockwaves().clear();
        game->arena_.state_.total_ = 2840;
        game->arena_.state_.kills_ = 31;
        Arena::PlayerState &player = game->arena_.combatants_.player().state;
        player.pos = {475, 430};
        player.hp = 74;
        player.hpDisplay = 84;
        player.aim = -0.55f;
        player.invuln = 99.0f;
        game->base_.mouseMove({1010, 265});

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
        game->arena_.entities_.playerBullets().emplace_back(
            Math::Vec2(650, 360), Math::Vec2(900, -250), 12.5f, Base::Cyan, true, 3.2f);
        game->arena_.entities_.playerBullets().emplace_back(
            Math::Vec2(590, 325), Math::Vec2(950, -180), 12.5f, Base::Cyan, true, 3.2f);
        game->arena_.entities_.playerBullets().emplace_back(
            Math::Vec2(720, 400), Math::Vec2(880, -60), 12.5f, Base::Cyan, true, 3.2f);
        game->arena_.entities_.enemyBullets().emplace_back(
            Math::Vec2(850, 300), Math::Vec2(-420, 150), 11.0f, Base::Magenta, false, 4.0f);
        game->arena_.entities_.enemyBullets().emplace_back(
            Math::Vec2(1010, 510), Math::Vec2(-380, -90), 11.0f, Base::Magenta, false, 4.0f);
        game->arena_.entities_.enemyBullets().emplace_back(
            Math::Vec2(1090, 420), Math::Vec2(-440, -30), 13.0f,
            QColor(255, 112, 190), false, 5.0f);
        game->arena_.entities_.spawnBurst(game->arena_, {860, 290}, Base::Coral, 14, 130.0f, 0.7f);
        game->arena_.entities_.spawnBurst(game->arena_, {715, 400}, Base::Cyan, 8, 100.0f, 0.5f);
        game->arena_.entities_.addShockwave({860, 290}, 48.0f, Base::Coral, 0.6f, 2);
        game->renderer_.showBanner(QStringLiteral("第 4 波"),
                                  QStringLiteral("敌方信号正在接近"), 1.8f, Base::Cyan);
        game->renderer_.bannerTimer_ = 1.3f;
        save(QStringLiteral("gameplay.png"));

        game->arena_.combatants_.enemies().clear();
        game->arena_.entities_.portals().clear();
        game->arena_.entities_.pickups().clear();
        game->arena_.entities_.playerBullets().clear();
        game->arena_.entities_.enemyBullets().clear();
        game->arena_.entities_.particles().clear();
        game->arena_.entities_.floatingTexts().clear();
        game->arena_.entities_.shockwaves().clear();
        game->arena_.combatants_.player().state.pos = {585, 420};
        game->waves_.start(4);
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 15;
        game->enterUpgrade();
        game->base_.mouseMove({640, 390});
        save(QStringLiteral("upgrade.png"));

        menu.startLevel(0);
        game = menu.games_.get();
        game->state_ = Games::State::Playing;
        game->waves_.start(8);
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 16;
        game->arena_.beginWave();
        game->runTime_ = 304.0f;
        game->arena_.state_.total_ = 9870;
        game->arena_.state_.kills_ = 63;
        game->arena_.combatants_.enemies().clear();
        game->arena_.entities_.playerBullets().clear();
        game->arena_.entities_.enemyBullets().clear();
        game->arena_.entities_.particles().clear();
        game->arena_.entities_.shockwaves().clear();
        Arena::PlayerState &bossPlayer = game->arena_.combatants_.player().state;
        bossPlayer.pos = {430, 435};
        bossPlayer.hp = 58;
        bossPlayer.hpDisplay = 71;
        bossPlayer.aim = -0.25f;
        bossPlayer.invuln = 99.0f;
        game->base_.mouseMove({850, 380});
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
            game->arena_.entities_.enemyBullets().emplace_back(
                game->arena_.combatants_.boss()->pos + direction * 58.0f,
                direction * 285.0f, 13.0f, Base::Magenta, false, 5.0f);
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
        } else {
            game->triggerVictory();
        }
        for (int i = 0; i < 120; ++i) {
            game->update(1.0f / 60.0f);
        }
        menu.update(0.0f);
        save(QStringLiteral("victory.png"));

        menu.startLevel(0);
        game = menu.games_.get();
        game->state_ = Games::State::Playing;
        game->waves_.start(3);
        game->waves_.queue_.clear();
        game->waves_.waveTotal_ = 12;
        game->runTime_ = 86.0f;
        game->arena_.state_.total_ = 1240;
        game->arena_.state_.kills_ = 17;
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
