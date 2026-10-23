#include "Renderer.h"

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

Renderer::Renderer(Random &random, Base &base) : random_(random), base_(base) {}

void Renderer::initialize()
{
    background_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                         QImage::Format_ARGB32_Premultiplied);
    vignette_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                       QImage::Format_ARGB32_Premultiplied);
    world_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                    QImage::Format_ARGB32_Premultiplied);
    glow_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                   QImage::Format_ARGB32_Premultiplied);
    frame_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                    QImage::Format_ARGB32_Premultiplied);
    flashColor_ = Base::White;

    buildBackground();
    buildVignette();

    stars_.reserve(130);
    for (int i = 0; i < 130; ++i) {
        Star star;
        const float depth = random_.real(0.0f, 1.0f);
        star.pos = {random_.real(10.0f, GameConfig::ScreenWidth - 10.0f),
                    random_.real(10.0f, GameConfig::ScreenHeight - 10.0f)};
        star.speed = 2.0f + depth * 9.0f;
        star.phase = random_.real(0.0f, Math::Tau);
        star.radius = depth < 0.72f ? 1 : 2;
        const std::array<QColor, 3> tints = {
            QColor(110, 170, 210), QColor(160, 130, 210), QColor(110, 210, 200)};
        star.tint = tints[size_t(random_.integer(0, 2))];
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

void Renderer::resetTransient()
{
    shake_ = 0.0f;
    flashAlpha_ = 0.0f;
    bannerTimer_ = 0.0f;
    sparks_.clear();
    titleTime_ = 0.0f;
    sparkTimer_ = 0.0f;
}

void Renderer::burstConfetti()
{
    const std::array<QColor, 4> colors = {
        Base::Cyan, Base::CyanSoft, Base::Green, Base::White};
    for (int i = 0; i < 80; ++i) {
        const float angle = random_.real(0.0f, Math::Tau);
        const float speed = random_.real(40.0f, 230.0f);
        sparks_.push_back(Spark{
            {GameConfig::ScreenWidth / 2.0f + random_.real(-160.0f, 160.0f),
             GameConfig::ScreenHeight / 2.0f + random_.real(-110.0f, 110.0f)},
            Math::Vec2(std::cos(angle), std::sin(angle)) * speed,
            random_.real(0.8f, 2.0f), 2.0f, random_.real(1.0f, 3.5f), 1.0f, 25.0f,
            colors[size_t(random_.integer(0, 3))]});
    }
}

void Renderer::update(float dt, bool titleActive)
{
    time_ += dt;
    uiTime_ += dt;
    if (titleActive) {
        titleTime_ += dt;
    }
    updateStars(dt);
    updateSparks(dt);

    if (titleActive) {
        sparkTimer_ -= dt;
        if (sparkTimer_ <= 0.0f) {
            sparkTimer_ = random_.real(0.035f, 0.09f);
            const float angle = random_.real(0.0f, Math::Tau);
            const float radius = random_.real(185.0f, 310.0f);
            const Math::Vec2 center(GameConfig::ScreenWidth / 2.0f,
                                    GameConfig::ScreenHeight / 2.0f - 55.0f);
            const Math::Vec2 pos = center
                + Math::Vec2(std::cos(angle), std::sin(angle)) * radius;
            const Math::Vec2 tangent(-std::sin(angle), std::cos(angle));
            const Math::Vec2 velocity = tangent * random_.real(12.0f, 35.0f)
                                        + Math::Vec2(random_.real(-8.0f, 8.0f),
                                                     random_.real(-20.0f, -5.0f));
            const std::array<QColor, 3> colors = {
                Base::Cyan, Base::CyanSoft, QColor(154, 134, 255)};
            sparks_.push_back(Spark{pos, velocity, random_.real(0.6f, 1.4f), 1.4f,
                                    random_.real(1.0f, 2.8f), 0.8f, 0.0f,
                                    colors[size_t(random_.integer(0, 2))]});
        }
    }
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
            star.pos.x = random_.real(0.0f, GameConfig::ScreenWidth);
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

void Renderer::render(const Games &game, const Arena &arena, const games::Waves &waves,
                      const games::Upgrades &upgrades)
{
    world_.fill(Qt::transparent);
    glow_.fill(Qt::transparent);

    {
        QPainter worldPainter(&world_);
        QPainter glowPainter(&glow_);
        worldPainter.setRenderHint(QPainter::Antialiasing, true);
        glowPainter.setRenderHint(QPainter::Antialiasing, true);
        worldPainter.drawImage(0, 0, background_);
        drawStars(worldPainter, glowPainter);

        if (game.state() == Games::State::Title) {
            drawTitleArt(worldPainter, glowPainter);
        } else {
            arena.draw(worldPainter, glowPainter);
        }
        drawSparks(worldPainter, glowPainter);
    }

    frame_.fill(QColor(2, 3, 9));
    QPainter painter(&frame_);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    int offsetX = 0;
    int offsetY = 0;
    if (shake_ > 0.25f) {
        offsetX = int(std::lround(random_.real(-shake_, shake_)));
        offsetY = int(std::lround(random_.real(-shake_, shake_)));
    }
    painter.drawImage(QPoint(offsetX, offsetY), world_);
    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    painter.drawImage(QPoint(offsetX, offsetY), glow_);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, vignette_);

    if (flashAlpha_ > 0.0f) {
        painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                         Base::withAlpha(flashColor_, int(flashAlpha_)));
    }

    switch (game.state()) {
    case Games::State::Title:
        drawTitleUI(painter);
        break;
    case Games::State::Playing:
        drawHUD(painter, arena, waves);
        drawBossBar(painter, arena);
        drawWaveBanner(painter);
        drawReticle(painter);
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
    case Games::State::GameOver:
        drawGameOverOverlay(painter, game, arena, waves);
        break;
    case Games::State::Victory:
        drawVictoryOverlay(painter, game, arena, waves);
        break;
    }
}

void Renderer::drawTitleArt(QPainter &world, QPainter &glow)
{
    const Math::Vec2 center(GameConfig::ScreenWidth / 2.0f,
                            GameConfig::ScreenHeight / 2.0f - 60.0f);

    struct RingDef {
        float radius;
        float speed;
        QColor color;
        float span;
    };
    const std::array<RingDef, 3> rings = {{
        {295.0f, 0.10f, Base::CyanSoft, 1.4f},
        {255.0f, -0.16f, QColor(126, 94, 190), 0.7f},
        {215.0f, 0.24f, Base::CyanSoft, 2.0f},
    }};
    for (size_t index = 0; index < rings.size(); ++index) {
        const RingDef &ring = rings[index];
        const float start = titleTime_ * ring.speed + float(index) * 1.7f;
        Base::arcRadians(world, center, ring.radius, start, ring.span,
                          Base::withAlpha(ring.color, 90), 1.0f);
        Base::arcRadians(glow, center, ring.radius, start, ring.span,
                          Base::shade(ring.color, 0.15f), 2.0f);
        const float dotAngle = start + ring.span;
        const Math::Vec2 dot = center
            + Math::Vec2(std::cos(dotAngle), std::sin(dotAngle)) * ring.radius;
        Base::fillCircle(world, dot, 2.0f, Base::withAlpha(ring.color, 160));
        Base::glow(glow, dot, 8.0f, ring.color, 0.7f);
    }

    for (int index = 0; index < 3; ++index) {
        const float radius = 160.0f + index * 38.0f;
        const float angle = -titleTime_ * (0.05f + index * 0.02f) + index;
        Base::arcRadians(world, center, radius, angle, angle + 2.3f,
                          QColor(25, 69, 85), 1.0f);
    }

    const Math::Vec2 core = center + Math::Vec2(0.0f, 5.0f);
    Base::glow(glow, core, 25.0f, Base::Cyan, 1.6f);
    Base::fillCircle(world, core, 3.0f, QColor(180, 249, 255));
}

void Renderer::drawTitleUI(QPainter &painter)
{
    Base::text(painter, Base::techTinyFont(),
                QStringLiteral("SECTOR 07  //  DEEP SPACE DEFENSE GRID"),
                QColor(121, 176, 194),
                {GameConfig::ScreenWidth / 2.0f, 118}, QStringLiteral("center"), 185);
    Base::glowText(painter, Base::titleFont(), QStringLiteral("星核协议"),
                    Base::White,
                    {GameConfig::ScreenWidth / 2.0f, 218}, QStringLiteral("center"));
    Base::text(painter, Base::techFont(),
                QStringLiteral("N E O N   C O R E   P R O T O C O L"),
                Base::Cyan,
                {GameConfig::ScreenWidth / 2.0f, 290}, QStringLiteral("center"), 230);

    const float lineY = 330.0f;
    Base::strokeLine(painter, {GameConfig::ScreenWidth / 2.0f - 220.0f, lineY},
                      {GameConfig::ScreenWidth / 2.0f - 34.0f, lineY},
                      QColor(44, 105, 123), 1.0f);
    Base::strokeLine(painter, {GameConfig::ScreenWidth / 2.0f + 34.0f, lineY},
                      {GameConfig::ScreenWidth / 2.0f + 220.0f, lineY},
                      QColor(44, 105, 123), 1.0f);
    Base::text(painter, Base::smallFont(), QStringLiteral("07"), Base::Muted,
                {GameConfig::ScreenWidth / 2.0f, lineY}, QStringLiteral("center"), 180);
    Base::text(painter, Base::bodyFont(),
                QStringLiteral("你是最后一位守夜人。守住这片光。"), Base::Text,
                {GameConfig::ScreenWidth / 2.0f, 368}, QStringLiteral("center"), 220);

    const float pulse = 0.5f + 0.5f * std::sin(uiTime_ * 2.4f);
    const QRectF button(GameConfig::ScreenWidth / 2.0f - 168.0f, 425.0f, 336.0f, 58.0f);
    const Math::Vec2 mouse = base_.mousePosition();
    const bool hovered = button.contains(QPointF(mouse.x, mouse.y));
    Base::panel(painter, button, hovered ? Base::Cyan : QColor(67, 158, 180),
                 hovered ? 210 : 155, 10, QColor(6, 18, 30));
    if (hovered) {
        painter.save();
        painter.setBrush(Qt::NoBrush);
        QPen pen(Base::withAlpha(Base::Cyan, 34));
        pen.setWidthF(2.0f);
        painter.setPen(pen);
        painter.drawRoundedRect(button.adjusted(-9, -9, 9, 9), 14, 14);
        painter.restore();
    }
    Base::text(painter, Base::mediumFont(), QStringLiteral("按 ENTER 开始部署"),
                hovered ? Base::White : QColor(215, 243, 250),
                {GameConfig::ScreenWidth / 2.0f, button.center().y()},
                QStringLiteral("center"));
    Base::text(painter, Base::tinyFont(), QStringLiteral("或点击任意位置"), Base::Muted,
                {GameConfig::ScreenWidth / 2.0f, float(button.bottom()) + 22.0f},
                QStringLiteral("center"), 170 + int(45 * pulse));

    struct ControlDef {
        QString key;
        QString label;
        float width;
    };
    const std::array<ControlDef, 3> controls = {{
        {QStringLiteral("W A S D"), QStringLiteral("移动"), 170.0f},
        {QStringLiteral("鼠标 / J"), QStringLiteral("射击"), 190.0f},
        {QStringLiteral("SPACE"), QStringLiteral("冲刺"), 170.0f},
    }};
    float total = 24.0f;
    for (const ControlDef &control : controls) {
        total += control.width;
    }
    float x = GameConfig::ScreenWidth / 2.0f - total / 2.0f;
    for (const ControlDef &control : controls) {
        const QRectF rect(x, 566, control.width, 42);
        Base::panel(painter, rect, QColor(39, 75, 91), 130, 8, QColor(6, 12, 22));
        const bool chineseKey = control.key.contains(QChar(0x9F20))
                                || control.key.contains(QChar(0x6807));
        Base::text(painter,
                    chineseKey ? Base::smallFont() : Base::techSmallFont(),
                    control.key, Base::Cyan,
                    {float(rect.center().x()), float(rect.center().y()) - 7.0f},
                    QStringLiteral("center"));
        Base::text(painter, Base::tinyFont(), control.label, Base::Muted,
                    {float(rect.center().x()), float(rect.center().y()) + 10.0f},
                    QStringLiteral("center"));
        x += control.width + 12.0f;
    }

    Base::text(painter, Base::techTinyFont(),
                QStringLiteral("V1.0  //  PROCEDURAL AUDIO + VISUALS"),
                QColor(66, 97, 118),
                {36, GameConfig::ScreenHeight - 30.0f}, QStringLiteral("midleft"), 165);
    const QString muteText = base_.muted() ? QStringLiteral("M  已静音")
                                           : QStringLiteral("M  音效开");
    Base::text(painter, Base::tinyFont(), muteText, QColor(91, 128, 149),
                {GameConfig::ScreenWidth - 36.0f, GameConfig::ScreenHeight - 30.0f},
                QStringLiteral("midright"), 165);
}

void Renderer::drawHUD(QPainter &painter, const Arena &arena, const games::Waves &waves)
{
    const Arena::PlayerState &player = arena.player();
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

    Base::text(painter, Base::tinyFont(), QStringLiteral("得分 / SCORE"), Base::Muted,
                {GameConfig::ScreenWidth - 30.0f, 17}, QStringLiteral("topright"), 220);
    Base::text(painter, Base::largeFont(),
                QLocale(QLocale::English).toString(arena.score()), Base::White,
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
    const QString right = base_.muted() ? QStringLiteral("M 已静音    ESC 暂停")
                                        : QStringLiteral("M 静音    ESC 暂停");
    Base::text(painter, Base::tinyFont(), right, QColor(113, 146, 168),
                {GameConfig::ScreenWidth - 32.0f, GameConfig::ScreenHeight - 26.0f},
                QStringLiteral("midright"), alpha);
}

void Renderer::drawReticle(QPainter &painter) const
{
    if (!base_.mouseActive()) {
        return;
    }
    const Math::Vec2 mouse = base_.mousePosition();
    Base::fillCircle(painter, mouse, 2.0f, Base::Cyan);
    for (int i = 0; i < 4; ++i) {
        const float angle = Math::Pi * 0.5f * i;
        const Math::Vec2 direction(std::cos(angle), std::sin(angle));
        Base::strokeLine(painter, {mouse.x + direction.x * 8.0f,
                                    mouse.y + direction.y * 8.0f},
                          {mouse.x + direction.x * 15.0f,
                           mouse.y + direction.y * 15.0f},
                          Base::Cyan, 1.0f);
    }
    Base::arcRadians(painter, mouse, 22.0f, uiTime_ * 0.8f, 1.2f,
                      QColor(81, 169, 190), 1.0f);
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

    const Math::Vec2 mouse = base_.mousePosition();
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

void Renderer::drawGameOverOverlay(QPainter &painter, const Games &game,
                                   const Arena &arena, const games::Waves &waves)
{
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 3, 9, 212));
    Base::strokeLine(painter, {GameConfig::ScreenWidth / 2.0f - 250, 118},
                      {GameConfig::ScreenWidth / 2.0f + 250, 118}, Base::Red, 1.0f);
    Base::text(painter, Base::techSmallFont(),
                QStringLiteral("MISSION FAILED  //  CORE SIGNAL LOST"), Base::Red,
                {GameConfig::ScreenWidth / 2.0f, 152}, QStringLiteral("center"), 220);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("防线失守"),
                    QColor(255, 119, 133),
                    {GameConfig::ScreenWidth / 2.0f, 222}, QStringLiteral("center"));
    Base::text(painter, Base::bodyFont(), QStringLiteral("星核在黑暗中熄灭。"),
                QColor(183, 205, 219),
                {GameConfig::ScreenWidth / 2.0f, 278}, QStringLiteral("center"), 215);
    drawRunStats(painter, game, arena, waves, 390.0f);
    Base::text(painter, Base::mediumFont(),
                QStringLiteral("按  R  重新部署        ESC  返回标题"),
                QColor(235, 244, 250),
                {GameConfig::ScreenWidth / 2.0f, 574}, QStringLiteral("center"), 235);
    Base::text(painter, Base::techTinyFont(),
                QStringLiteral("TRY AGAIN  //  THE CORE STILL NEEDS YOU"),
                QColor(113, 143, 160),
                {GameConfig::ScreenWidth / 2.0f, 618}, QStringLiteral("center"), 170);
}

void Renderer::drawVictoryOverlay(QPainter &painter, const Games &game,
                                  const Arena &arena, const games::Waves &waves)
{
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 7, 13, 190));
    Base::strokeLine(painter, {GameConfig::ScreenWidth / 2.0f - 250, 118},
                      {GameConfig::ScreenWidth / 2.0f + 250, 118}, Base::Cyan, 1.0f);
    Base::text(painter, Base::techSmallFont(),
                QStringLiteral("MISSION COMPLETE  //  CORE STABILIZED"), Base::Green,
                {GameConfig::ScreenWidth / 2.0f, 152}, QStringLiteral("center"), 230);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("星核已稳定"),
                    QColor(151, 255, 220),
                    {GameConfig::ScreenWidth / 2.0f, 222}, QStringLiteral("center"));
    Base::text(painter, Base::bodyFont(), QStringLiteral("你守住了最后一道光。"),
                QColor(194, 232, 232),
                {GameConfig::ScreenWidth / 2.0f, 278}, QStringLiteral("center"), 225);
    drawRunStats(painter, game, arena, waves, 390.0f);
    Base::text(painter, Base::mediumFont(),
                QStringLiteral("按  R  再次出击        ESC  返回标题"),
                QColor(235, 250, 250),
                {GameConfig::ScreenWidth / 2.0f, 574}, QStringLiteral("center"), 235);
    Base::text(painter, Base::techTinyFont(),
                QStringLiteral("ALL HOSTILES CLEARED  //  SIGNAL RESTORED"),
                QColor(126, 185, 177),
                {GameConfig::ScreenWidth / 2.0f, 618}, QStringLiteral("center"), 180);
}

void Renderer::drawRunStats(QPainter &painter, const Games &game,
                            const Arena &arena, const games::Waves &waves, float top)
{
    const int minutes = int(game.runTime()) / 60;
    const int seconds = int(game.runTime()) % 60;
    const QString timeText = QStringLiteral("%1:%2")
                                 .arg(minutes, 2, 10, QChar('0'))
                                 .arg(seconds, 2, 10, QChar('0'));
    const std::array<std::pair<QString, QString>, 4> stats = {{
        {QStringLiteral("最终得分"), QLocale(QLocale::English).toString(arena.score())},
        {QStringLiteral("到达波次"),
         QStringLiteral("%1 / %2").arg(waves.currentWave()).arg(waves.totalWaves())},
        {QStringLiteral("击破目标"), QString::number(arena.kills())},
        {QStringLiteral("存活时间"), timeText},
    }};
    constexpr float totalWidth = 760.0f;
    constexpr float cell = totalWidth / 4.0f;
    const float start = GameConfig::ScreenWidth / 2.0f - totalWidth / 2.0f;
    for (int i = 0; i < 4; ++i) {
        const float cx = start + cell * i + cell / 2.0f;
        if (i > 0) {
            Base::strokeLine(painter, {start + cell * i, top + 4},
                              {start + cell * i, top + 92}, QColor(44, 67, 83), 1.0f);
        }
        Base::text(painter, Base::tinyFont(), stats[size_t(i)].first, Base::Muted,
                    {cx, top}, QStringLiteral("center"), 190);
        Base::text(painter, Base::largeFont(), stats[size_t(i)].second, Base::White,
                    {cx, top + 27}, QStringLiteral("center"));
    }
}
