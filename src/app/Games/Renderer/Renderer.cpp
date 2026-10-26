#include "Games/Renderer/Renderer.h"

#include "Arena/Arena.h"
#include "Games/Games.h"
#include "Random.h"
#include "Games/Upgrades/Upgrades.h"
#include "Games/Waves/Waves.h"
#include "Base/Base.h"

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QLocale>
#include <QPainterPath>
#include <QStringList>

#include <algorithm>
#include <array>
#include <cmath>

namespace games {


Renderer::Renderer() = default;

void Renderer::initialize()
{
    background_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                         QImage::Format_ARGB32_Premultiplied);
    vignette_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                       QImage::Format_ARGB32_Premultiplied);
    glow_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                   QImage::Format_ARGB32_Premultiplied);
    frame_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                    QImage::Format_ARGB32_Premultiplied);
    dummyWorld_ = QImage(1, 1, QImage::Format_ARGB32_Premultiplied);
    dummyGlow_ = QImage(1, 1, QImage::Format_ARGB32_Premultiplied);
    dummyWorld_.fill(Qt::transparent);
    dummyGlow_.fill(Qt::transparent);
    flashColor_ = Base::White;

    buildBackground();
    buildVignette();

    stars_.reserve(130);
    for (int i = 0; i < 130; ++i) {
        Star star;
        const float depth = RandomService.real(0.0f, 1.0f);
        star.pos = {RandomService.real(10.0f, GameConfig::ScreenWidth - 10.0f),
                    RandomService.real(10.0f, GameConfig::ScreenHeight - 10.0f)};
        star.speed = 2.0f + depth * 9.0f;
        star.phase = RandomService.real(0.0f, Math::Tau);
        star.radius = depth < 0.72f ? 1 : 2;
        const std::array<QColor, 3> tints = {
            QColor(110, 170, 210), QColor(160, 130, 210), QColor(110, 210, 200)};
        star.tint = tints[size_t(RandomService.integer(0, 2))];
        stars_.push_back(star);
    }
}

void Renderer::shake(float amount)
{
    shake_ = std::max(shake_, amount);
}

void Renderer::flash(const QColor &color, int alpha)
{
    flashColor_ = color;
    flashAlpha_ = std::max(flashAlpha_, float(alpha));
}

void Renderer::showBanner(const QString &title, const QString &subtitle, float duration,
                          const QColor &color, const QString &kicker)
{
    bannerTitle_ = title;
    bannerSub_ = subtitle;
    bannerKicker_ = kicker;
    bannerDuration_ = duration;
    bannerTimer_ = duration;
    bannerColor_ = color;
}

void Renderer::burstConfetti()
{
    const std::array<QColor, 4> colors = {
        Base::Cyan, Base::CyanSoft, Base::Green, Base::White};
    for (int i = 0; i < 80; ++i) {
        const float angle = RandomService.real(0.0f, Math::Tau);
        const float speed = RandomService.real(40.0f, 230.0f);
        sparks_.push_back(Spark{
            {GameConfig::ScreenWidth / 2.0f + RandomService.real(-160.0f, 160.0f),
             GameConfig::ScreenHeight / 2.0f + RandomService.real(-110.0f, 110.0f)},
            Math::Vec2(std::cos(angle), std::sin(angle)) * speed,
            RandomService.real(0.8f, 2.0f), 2.0f, RandomService.real(1.0f, 3.5f), 1.0f, 25.0f,
            colors[size_t(RandomService.integer(0, 3))]});
    }
}

void Renderer::update(float dt)
{
    time_ += dt;
    updateStars(dt);
    updateSparks(dt);

    if (sparks_.size() > 300) {
        sparks_.erase(sparks_.begin(), sparks_.begin() + (sparks_.size() - 300));
    }

    shake_ *= std::exp(-7.0f * dt);
    if (shake_ < 0.25f) {
        shake_ = 0.0f;
    }
    flashAlpha_ = std::max(0.0f, flashAlpha_ - 260.0f * dt);
    bannerTimer_ = std::max(0.0f, bannerTimer_ - dt);
}

void Renderer::updateStars(float dt)
{
    for (Star &star : stars_) {
        star.pos.y += star.speed * dt;
        star.pos.x += star.speed * 0.16f * dt;
        if (star.pos.y > GameConfig::ScreenHeight + 4.0f) {
            star.pos.y = -4.0f;
            star.pos.x = RandomService.real(0.0f, GameConfig::ScreenWidth);
        }
        if (star.pos.x > GameConfig::ScreenWidth + 4.0f) {
            star.pos.x = -4.0f;
        }
    }
}

void Renderer::updateSparks(float dt)
{
    for (Spark &spark : sparks_) {
        spark.life -= dt;
        spark.vel *= std::exp(-spark.drag * dt);
        spark.vel.y += spark.gravity * dt;
        spark.pos += spark.vel * dt;
    }
    std::erase_if(sparks_, [](const Spark &spark) { return spark.life <= 0.0f; });
}

void Renderer::buildBackground()
{
    background_.fill(QColor(2, 5, 13));
    QPainter painter(&background_);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient gradient(0.0, 0.0, 0.0, GameConfig::ScreenHeight);
    gradient.setColorAt(0.0, QColor(9, 15, 31));
    gradient.setColorAt(1.0, QColor(2, 5, 13));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight), gradient);

    QImage haze(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                QImage::Format_ARGB32_Premultiplied);
    haze.fill(Qt::transparent);
    {
        QPainter hazePainter(&haze);
        hazePainter.setPen(Qt::NoPen);
        struct Haze {
            float x;
            float y;
            float radius;
            QColor color;
        };
        const std::array<Haze, 3> hazes = {{
            {250.0f, 210.0f, 300.0f, QColor(30, 70, 120)},
            {1060.0f, 540.0f, 340.0f, QColor(68, 30, 92)},
            {760.0f, 120.0f, 240.0f, QColor(18, 90, 92)},
        }};
        for (const Haze &hazeDef : hazes) {
            for (int step = 26; step >= 1; --step) {
                const float radius = hazeDef.radius * step / 26.0f;
                const int alpha = int(3 + 8 * (1.0f - step / 26.0f));
                hazePainter.setBrush(QColor(hazeDef.color.red(), hazeDef.color.green(),
                                            hazeDef.color.blue(), alpha));
                hazePainter.drawEllipse(QPointF(hazeDef.x, hazeDef.y), radius, radius);
            }
        }
    }
    painter.drawImage(0, 0, haze);

    for (int x = 0; x <= GameConfig::ScreenWidth; x += 64) {
        const int alpha = (x % 256 == 0) ? 25 : 12;
        painter.setPen(QColor(72, 126, 155, alpha));
        painter.drawLine(QPointF(x, 0), QPointF(x, GameConfig::ScreenHeight));
    }
    for (int y = 0; y <= GameConfig::ScreenHeight; y += 64) {
        const int alpha = (y % 256 == 0) ? 25 : 12;
        painter.setPen(QColor(72, 126, 155, alpha));
        painter.drawLine(QPointF(0, y), QPointF(GameConfig::ScreenWidth, y));
    }
    for (int y = 0; y < GameConfig::ScreenHeight; y += 4) {
        painter.setPen(QColor(0, 0, 0, 9));
        painter.drawLine(QPointF(0, y), QPointF(GameConfig::ScreenWidth, y));
    }
}

void Renderer::buildVignette()
{
    vignette_.fill(Qt::transparent);
    QPainter painter(&vignette_);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setBrush(Qt::NoBrush);
    for (int i = 0; i < 110; ++i) {
        const int alpha = int(86.0f * std::pow(1.0f - i / 110.0f, 2.0f));
        painter.setPen(QColor(0, 0, 0, alpha));
        painter.drawRect(QRectF(i, i, GameConfig::ScreenWidth - i * 2,
                                GameConfig::ScreenHeight - i * 2));
    }
}

void Renderer::drawStars(QPainter &world, QPainter &glow) const
{
    for (const Star &star : stars_) {
        const float twinkle = 0.58f + 0.42f * std::sin(time_ * 2.0f + star.phase);
        const float brightness = 0.42f + 0.58f * twinkle;
        const QColor color(int(star.tint.red() * brightness),
                           int(star.tint.green() * brightness),
                           int(star.tint.blue() * brightness));
        Base::fillCircle(world, star.pos, float(star.radius), color);
        if (star.radius > 1) {
            Base::fillCircle(glow, star.pos, 3.0f, Base::shade(color, 0.28f));
        }
    }
}

void Renderer::drawSparks(QPainter &world, QPainter &glow) const
{
    for (const Spark &spark : sparks_) {
        if (spark.life <= 0.0f) {
            continue;
        }
        const float t = Math::clamp(spark.life / spark.maxLife, 0.0f, 1.0f);
        const int alpha = int(210.0f * std::min(1.0f, t * 1.7f));
        const float radius = std::max(1.0f, spark.radius * (0.25f + 0.75f * t));
        Base::fillCircle(world, spark.pos, radius, Base::withAlpha(spark.color, alpha));
        Base::glow(glow, spark.pos, spark.radius * (0.8f + 0.9f * t),
                    spark.color, 0.8f + 0.8f * t);
    }
}

void Renderer::render(const Games &game, const Arena &arena, const Waves &waves,
                      const Upgrades &upgrades)
{
    glow_.fill(Qt::transparent);

    int offsetX = 0;
    int offsetY = 0;
    if (shake_ > 0.25f) {
        offsetX = int(std::lround(RandomService.real(-shake_, shake_)));
        offsetY = int(std::lround(RandomService.real(-shake_, shake_)));
    }

    // 主线程跑辉光通道（写 glow_），工作线程跑世界通道（写 frame_）。
    // 两条通道写不同 QImage、覆盖同一批绘制调用但互不重叠；字体绘制被排除在通道外。
    if (BaseService.parallelEnabled() && BaseService.parallelWorkers() > 0) {
        BaseService.parallelFor(2, [&](int task) {
            if (task == 0) {
                Base::beginGlowPass();
                drawGlowPass(arena);
                Base::endRenderPass();
            } else {
                Base::beginWorldPass();
                drawWorldPass(arena, offsetX, offsetY);
                Base::endRenderPass();
            }
        });
    } else {
        // 单线程回退：一次遍历同时画世界层和辉光层，避免两遍绘制的重复开销。
        Base::endRenderPass();
        drawCombinedPass(arena, offsetX, offsetY);
    }

    // 飘字留在主线程：画在世界层上、辉光叠加之前，与拆分前的顺序一致。
    {
        QPainter painter(&frame_);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.translate(float(offsetX), float(offsetY));
        arena.drawTexts(painter);
    }

    compositeFrame(offsetX, offsetY);

    QPainter painter(&frame_);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    drawUi(painter, game, arena, waves, upgrades);
}

void Renderer::drawWorldPass(const Arena &arena, int offsetX, int offsetY)
{
    if (offsetX != 0 || offsetY != 0) {
        frame_.fill(QColor(2, 3, 9));
    }
    QPainter worldPainter(&frame_);
    QPainter dummy(&dummyGlow_);
    worldPainter.setRenderHint(QPainter::Antialiasing, true);
    worldPainter.translate(float(offsetX), float(offsetY));
    // background_ 不透明且整幅覆盖，不需要先清 frame。
    worldPainter.drawImage(0, 0, background_);
    drawStars(worldPainter, dummy);
    arena.draw(worldPainter, dummy);
    drawSparks(worldPainter, dummy);
}

void Renderer::drawCombinedPass(const Arena &arena, int offsetX, int offsetY)
{
    if (offsetX != 0 || offsetY != 0) {
        frame_.fill(QColor(2, 3, 9));
    }
    QPainter worldPainter(&frame_);
    QPainter glowPainter(&glow_);
    worldPainter.setRenderHint(QPainter::Antialiasing, true);
    glowPainter.setRenderHint(QPainter::Antialiasing, true);
    worldPainter.translate(float(offsetX), float(offsetY));
    worldPainter.drawImage(0, 0, background_);
    drawStars(worldPainter, glowPainter);
    arena.draw(worldPainter, glowPainter);
    drawSparks(worldPainter, glowPainter);
}

void Renderer::drawGlowPass(const Arena &arena)
{
    QPainter glowPainter(&glow_);
    QPainter dummy(&dummyWorld_);
    glowPainter.setRenderHint(QPainter::Antialiasing, true);
    drawStars(dummy, glowPainter);
    arena.draw(dummy, glowPainter);
    drawSparks(dummy, glowPainter);
}

void Renderer::drawUi(QPainter &painter, const Games &game, const Arena &arena,
                      const Waves &waves, const Upgrades &upgrades)
{
    switch (game.state()) {
    case Games::State::Playing:
        drawHUD(painter, arena, waves);
        drawBossBar(painter, arena);
        drawWaveBanner(painter);
        drawPlayHints(painter);
        break;
    case Games::State::Paused:
        drawHUD(painter, arena, waves);
        drawBossBar(painter, arena);
        drawPauseOverlay(painter, arena, waves);
        drawPlayHints(painter);
        break;
    case Games::State::Upgrade:
        drawUpgradeOverlay(painter, upgrades, waves);
        break;
    case Games::State::Victory:
    case Games::State::Defeat:
        break;
    }
}

void Renderer::compositeFrame(int offsetX, int offsetY)
{
    QPainter painter(&frame_);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    painter.drawImage(QPoint(offsetX, offsetY), glow_);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, vignette_);

    if (flashAlpha_ > 0.0f) {
        painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                         Base::withAlpha(flashColor_, int(flashAlpha_)));
    }
}

void Renderer::drawHUD(QPainter &painter, const Arena &arena, const games::Waves &waves)
{
    const Arena::PlayerState &player = arena.playerState();
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, 100), QColor(3, 8, 17, 208));
    Base::strokeLine(painter, {0, 99}, {GameConfig::ScreenWidth, 99},
                      QColor(23, 50, 66), 1.0f);

    const float hpRatio = Math::clamp(player.hp / player.maxHp, 0.0f, 1.0f);
    const float ghostRatio = Math::clamp(player.hpDisplay / player.maxHp, 0.0f, 1.0f);
    const QColor hpColor = hpRatio > 0.5f ? Base::Cyan
                                          : (hpRatio > 0.25f ? Base::Amber : Base::Red);
    Base::text(painter, Base::tinyFont(), QStringLiteral("生命 / INTEGRITY"),
                Base::Muted, {30, 17}, QStringLiteral("topleft"), 220);
    Base::text(painter, Base::techSmallFont(),
                QStringLiteral("%1 / %2")
                    .arg(int(std::ceil(player.hp)), 3, 10, QChar('0'))
                    .arg(int(player.maxHp), 3, 10, QChar('0')),
                QColor(198, 224, 238), {356, 29}, QStringLiteral("midright"));
    Base::bar(painter, QRectF(30, 39, 326, 15), hpRatio, hpColor, ghostRatio);
    Base::text(painter, Base::tinyFont(), QStringLiteral("相位冲刺"), Base::Muted,
                {30, 68}, QStringLiteral("topleft"), 200);
    const float dashRatio = 1.0f - (player.dashCdMax > 0.0f
                                        ? player.dashCd / player.dashCdMax
                                        : 1.0f);
    const QColor dashColor = dashRatio >= 0.999f ? Base::Cyan : QColor(53, 105, 122);
    Base::bar(painter, QRectF(100, 72, 136, 6), dashRatio, dashColor, -1.0f,
               QColor(18, 30, 45));
    const QString dashLabel = dashRatio >= 0.999f
                                  ? QStringLiteral("READY")
                                  : QStringLiteral("%1%").arg(int(dashRatio * 100.0f), 2,
                                                              10, QChar('0'));
    Base::text(painter, Base::techTinyFont(), dashLabel, dashColor, {246, 72},
                QStringLiteral("midleft"), 220);

    Base::strokeLine(painter, {392, 20}, {392, 82}, QColor(31, 57, 72), 1.0f);
    Base::strokeLine(painter, {888, 20}, {888, 82}, QColor(31, 57, 72), 1.0f);

    Base::text(painter, Base::mediumFont(),
                QStringLiteral("波次  %1 / %2")
                    .arg(waves.currentWave(), 2, 10, QChar('0'))
                    .arg(waves.totalWaves(), 2, 10, QChar('0')),
                Base::Ink, {GameConfig::ScreenWidth / 2.0f, 25}, QStringLiteral("center"));
    const int remaining = waves.remainingThreats();
    const float progress = 1.0f - float(remaining) / std::max(1, waves.totalEnemies());
    Base::bar(painter, QRectF(GameConfig::ScreenWidth / 2.0f - 128.0f, 51, 256, 6),
               progress, Base::Cyan, -1.0f, QColor(17, 30, 43));
    Base::text(painter, Base::tinyFont(),
                QStringLiteral("剩余威胁  %1").arg(remaining, 2, 10, QChar('0')),
                Base::Muted, {GameConfig::ScreenWidth / 2.0f, 70},
                QStringLiteral("center"), 220);

    Base::text(painter, Base::tinyFont(), QStringLiteral("积分 / SCORE"), Base::Muted,
                {GameConfig::ScreenWidth - 30.0f, 17}, QStringLiteral("topright"), 220);
    static const QLocale english(QLocale::English);
    Base::text(painter, Base::largeFont(),
                english.toString(arena.score()), Base::White,
                {GameConfig::ScreenWidth - 30.0f, 35}, QStringLiteral("topright"));
    Base::text(painter, Base::tinyFont(),
                QStringLiteral("击破  %1").arg(arena.kills(), 3, 10, QChar('0')),
                Base::Muted, {GameConfig::ScreenWidth - 30.0f, 75},
                QStringLiteral("topright"), 205);
}

void Renderer::drawBossBar(QPainter &painter, const Arena &arena)
{
    const Arena::BossInfo boss = arena.bossInfo();
    if (!boss.active) {
        return;
    }
    const float ratio = Math::clamp(boss.hp / boss.maxHp, 0.0f, 1.0f);
    const float ghost = Math::clamp(boss.hpDisplay / boss.maxHp, 0.0f, 1.0f);
    Base::text(painter, Base::smallFont(),
                QStringLiteral("虚空吞噬者  //  VOID DEVOURER"),
                QColor(255, 157, 203),
                {GameConfig::ScreenWidth / 2.0f, 110}, QStringLiteral("center"), 235);
    const QRectF rect(GameConfig::ScreenWidth / 2.0f - 300.0f, 128, 600, 12);
    Base::bar(painter, rect, ratio, Base::Magenta, ghost, QColor(30, 12, 28));
    for (int i = 1; i < 8; ++i) {
        const float x = float(rect.left()) + float(rect.width()) * i / 8.0f;
        Base::strokeLine(painter, {x, float(rect.top())}, {x, float(rect.bottom())},
                          QColor(28, 9, 27), 1.0f);
    }
    painter.save();
    painter.setBrush(Qt::NoBrush);
    QPen pen(QColor(255, 150, 197));
    pen.setWidthF(1.0f);
    painter.setPen(pen);
    painter.drawRoundedRect(rect, 6, 6);
    painter.restore();
}

void Renderer::drawWaveBanner(QPainter &painter)
{
    if (bannerTimer_ <= 0.0f) {
        return;
    }
    const float elapsed = bannerDuration_ - bannerTimer_;
    const float appear = Math::clamp(elapsed / 0.22f, 0.0f, 1.0f);
    const float disappear = Math::clamp(bannerTimer_ / 0.5f, 0.0f, 1.0f);
    const int alpha = int(255.0f * std::min(appear, disappear));
    if (alpha <= 2) {
        return;
    }
    const float y = 198.0f;
    const QRectF plate(GameConfig::ScreenWidth / 2.0f - 310.0f, y - 54.0f, 620, 118);
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(3, 8, 18, int(alpha * 0.34f)));
    painter.drawRoundedRect(plate, 12, 12);
    painter.setBrush(Qt::NoBrush);
    QPen pen(Base::withAlpha(bannerColor_, int(alpha * 0.12f)));
    pen.setWidthF(1.0f);
    painter.setPen(pen);
    painter.drawRoundedRect(plate, 12, 12);
    painter.restore();

    Base::text(painter, Base::techSmallFont(), bannerKicker_, bannerColor_,
                {GameConfig::ScreenWidth / 2.0f, y - 48.0f},
                QStringLiteral("center"), int(alpha * 0.72f));
    Base::text(painter, Base::bigFont(), bannerTitle_, bannerColor_,
                {GameConfig::ScreenWidth / 2.0f, y}, QStringLiteral("center"), alpha);
    Base::text(painter, Base::smallFont(), bannerSub_, QColor(187, 211, 226),
                {GameConfig::ScreenWidth / 2.0f, y + 42.0f},
                QStringLiteral("center"), int(alpha * 0.85f));
    const float half = 180.0f;
    const QColor lineColor = Base::shade(bannerColor_, 0.36f);
    Base::strokeLine(painter, {GameConfig::ScreenWidth / 2.0f - half - 46.0f, y},
                      {GameConfig::ScreenWidth / 2.0f - half, y}, lineColor, 1.0f);
    Base::strokeLine(painter, {GameConfig::ScreenWidth / 2.0f + half, y},
                      {GameConfig::ScreenWidth / 2.0f + half + 46.0f, y},
                      lineColor, 1.0f);
}

void Renderer::drawPlayHints(QPainter &painter)
{
    const int alpha = 122;
    Base::text(painter, Base::tinyFont(),
                QStringLiteral("WASD 移动    LMB / J 射击    SPACE 冲刺"),
                QColor(113, 146, 168),
                {32, GameConfig::ScreenHeight - 26.0f}, QStringLiteral("midleft"), alpha);
    const QString right = BaseService.muted() ? QStringLiteral("M 已静音    ESC 暂停")
                                        : QStringLiteral("M 静音    ESC 暂停");
    Base::text(painter, Base::tinyFont(), right, QColor(113, 146, 168),
                {GameConfig::ScreenWidth - 32.0f, GameConfig::ScreenHeight - 26.0f},
                QStringLiteral("midright"), alpha);
}

QRectF Renderer::cardRect(int index) const
{
    constexpr float width = 320.0f;
    constexpr float height = 350.0f;
    constexpr float gap = 25.0f;
    const float left = (GameConfig::ScreenWidth - (width * 3.0f + gap * 2.0f)) / 2.0f;
    return QRectF(left + index * (width + gap), 230, width, height);
}

int Renderer::upgradeIndexAt(const Math::Vec2 &pos) const
{
    for (int i = 0; i < 3; ++i) {
        if (cardRect(i).contains(QPointF(pos.x, pos.y))) {
            return i;
        }
    }
    return -1;
}

void Renderer::drawUpgradeOverlay(QPainter &painter, const games::Upgrades &upgrades,
                                  const games::Waves &waves)
{
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 5, 12, 208));
    Base::text(painter, Base::techSmallFont(),
                QStringLiteral("WAVE %1 CLEARED  //  LOADOUT")
                    .arg(waves.currentWave(), 2, 10, QChar('0')),
                Base::Green, {GameConfig::ScreenWidth / 2.0f, 92},
                QStringLiteral("center"), 235);
    Base::text(painter, Base::bigFont(), QStringLiteral("强化协议"), Base::White,
                {GameConfig::ScreenWidth / 2.0f, 140}, QStringLiteral("center"));
    Base::text(painter, Base::smallFont(),
                QStringLiteral("选择一项强化，准备迎接第 %1 波")
                    .arg(waves.currentWave() + 1),
                Base::Muted,
                {GameConfig::ScreenWidth / 2.0f, 184}, QStringLiteral("center"), 225);

    const Math::Vec2 mouse = BaseService.mousePosition();
    for (int i = 0; i < 3 && i < int(upgrades.offers().size()); ++i) {
        const QRectF rect = cardRect(i);
        const bool hovered = rect.contains(QPointF(mouse.x, mouse.y));
        drawUpgradeCard(painter, i, upgrades.offers().at(size_t(i)), rect, hovered);
    }

    Base::text(painter, Base::tinyFont(),
                QStringLiteral("按  1 / 2 / 3  选择   ·   也可以直接点击卡片"),
                QColor(117, 150, 171),
                {GameConfig::ScreenWidth / 2.0f, 630}, QStringLiteral("center"), 190);
}

void Renderer::drawUpgradeCard(QPainter &painter, int index,
                               const games::Upgrades::Offer &offer,
                               const QRectF &rect, bool hovered)
{
    const QColor color = offer.color;
    const QRectF drawRect = rect.translated(0.0f, hovered ? -9.0f : 0.0f);
    if (hovered) {
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(Base::withAlpha(color, 45));
        painter.drawRoundedRect(drawRect.adjusted(-12, -12, 12, 12), 18, 18);
        painter.restore();
    }

    Base::panel(painter, drawRect, hovered ? color : Base::shade(color, 0.55f),
                 232, 12, QColor(7, 14, 26));
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(drawRect.left() + 1, drawRect.top() + 1,
                                   drawRect.width() - 2, 4),
                            2, 2);
    painter.restore();

    Base::text(painter, Base::techTinyFont(), QStringLiteral("UPGRADE PROTOCOL"),
                Base::shade(color, 0.78f),
                {float(drawRect.left()) + 26.0f, float(drawRect.top()) + 24.0f},
                QStringLiteral("topleft"), 190);
    Base::text(painter, Base::techSmallFont(),
                QStringLiteral("0%1").arg(index + 1), Base::shade(color, 0.66f),
                {float(drawRect.right()) - 24.0f, float(drawRect.top()) + 22.0f},
                QStringLiteral("topright"), 195);
    // 本轮掷出的实际数值
    Base::text(painter, Base::techFont(), offer.valueText,
                Base::withAlpha(color, 240),
                {float(drawRect.right()) - 24.0f, float(drawRect.top()) + 52.0f},
                QStringLiteral("topright"), 240);

    const Math::Vec2 iconCenter(float(drawRect.left()) + 52.0f,
                                float(drawRect.top()) + 77.0f);
    Base::fillCircle(painter, iconCenter, 27.0f, Base::shade(color, 0.16f));
    Base::strokeCircle(painter, iconCenter, 27.0f, color, 1.0f);
    Base::text(painter, Base::techTinyFont(), offer.tag, color, iconCenter,
                QStringLiteral("center"), 245);
    Base::text(painter, Base::mediumFont(), offer.name, Base::White,
                {float(drawRect.left()) + 92.0f, float(drawRect.top()) + 62.0f},
                QStringLiteral("midleft"));
    Base::text(painter, Base::techTinyFont(), QStringLiteral("ENHANCEMENT"),
                Base::Muted,
                {float(drawRect.left()) + 93.0f, float(drawRect.top()) + 87.0f},
                QStringLiteral("midleft"), 170);
    Base::strokeLine(painter,
                      {float(drawRect.left()) + 25.0f, float(drawRect.top()) + 126.0f},
                      {float(drawRect.right()) - 25.0f, float(drawRect.top()) + 126.0f},
                      Base::shade(color, 0.35f), 1.0f);

    const QStringList lines = Base::wrapText(offer.desc, Base::bodyFont(),
                                              float(drawRect.width()) - 52.0f);
    for (int lineIndex = 0; lineIndex < lines.size() && lineIndex < 5; ++lineIndex) {
        Base::text(painter, Base::bodyFont(), lines.at(lineIndex),
                    QColor(198, 219, 234),
                    {float(drawRect.left()) + 26.0f,
                     float(drawRect.top()) + 148.0f + lineIndex * 29.0f},
                    QStringLiteral("topleft"), 225);
    }

    const QString stackText = offer.level == 0
                                  ? QStringLiteral("全新协议")
                                  : QStringLiteral("已叠加  ×%1").arg(offer.level);
    Base::text(painter, Base::tinyFont(), stackText, Base::shade(color, 0.82f),
                {float(drawRect.left()) + 26.0f, float(drawRect.bottom()) - 30.0f},
                QStringLiteral("midleft"), 200);

    const QRectF button(float(drawRect.right()) - 91.0f,
                        float(drawRect.bottom()) - 44.0f, 65, 27);
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(13, 24, 38));
    painter.drawRoundedRect(button, 6, 6);
    painter.setBrush(Qt::NoBrush);
    QPen pen(Base::shade(color, 0.75f));
    pen.setWidthF(1.0f);
    painter.setPen(pen);
    painter.drawRoundedRect(button, 6, 6);
    painter.restore();
    Base::text(painter, Base::techTinyFont(), QStringLiteral("SELECT"), color,
                {float(button.center().x()), float(button.center().y())},
                QStringLiteral("center"), 235);
}

void Renderer::drawPauseOverlay(QPainter &painter, const Arena &arena, const games::Waves &waves)
{
    Q_UNUSED(arena);
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 5, 11, 172));
    const QRectF panel(GameConfig::ScreenWidth / 2.0f - 280.0f,
                       GameConfig::ScreenHeight / 2.0f - 142.0f, 560, 284);
    Base::panel(painter, panel, QColor(63, 136, 158), 228, 16, QColor(5, 12, 23));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("SYSTEM SUSPENDED"),
                Base::Cyan, {GameConfig::ScreenWidth / 2.0f, float(panel.top()) + 38.0f},
                QStringLiteral("center"), 210);
    Base::text(painter, Base::bigFont(), QStringLiteral("已暂停"), Base::White,
                {GameConfig::ScreenWidth / 2.0f, float(panel.top()) + 92.0f},
                QStringLiteral("center"));
    Base::text(painter, Base::bodyFont(),
                QStringLiteral("第 %1 波  ·  剩余威胁 %2")
                    .arg(waves.currentWave())
                    .arg(waves.remainingThreats()),
                Base::Muted,
                {GameConfig::ScreenWidth / 2.0f, float(panel.top()) + 143.0f},
                QStringLiteral("center"));
    Base::text(painter, Base::smallFont(), QStringLiteral("按  P / ESC  继续"),
                QColor(204, 231, 242),
                {GameConfig::ScreenWidth / 2.0f, float(panel.top()) + 194.0f},
                QStringLiteral("center"));
    Base::text(painter, Base::tinyFont(),
                QStringLiteral("Q 返回标题    ·    M 静音"), Base::Muted,
                {GameConfig::ScreenWidth / 2.0f, float(panel.top()) + 234.0f},
                QStringLiteral("center"), 190);
}

} // namespace games

