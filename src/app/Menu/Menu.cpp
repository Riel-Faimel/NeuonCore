#include "Menu/Menu.h"

#include "Base/Base.h"

#include <QLinearGradient>
#include <QLocale>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

Menu::Menu(bool enableAudio) : games_(enableAudio)
{
    frame_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                    QImage::Format_ARGB32_Premultiplied);
    levels_ = {
        {QStringLiteral("standard"), QStringLiteral("标准协议"),
         QStringLiteral("八波战役 · 守住星核"), 1},
        {QStringLiteral("boss"), QStringLiteral("核心突袭"),
         QStringLiteral("直接进入最终波次"), 8},
    };
}

Menu::~Menu() = default;

const QImage &Menu::frame() const
{
    return screen_ == Screen::Match ? games_.frame() : frame_;
}

void Menu::update(float dt)
{
    menuTime_ += dt;
    if (screen_ == Screen::Match) {
        games_.update(dt);
        if (games_.state() == Games::State::Victory
            || games_.state() == Games::State::Defeat) {
            screen_ = Screen::Result;
        }
        return;
    }
    if (screen_ == Screen::Result) {
        games_.update(dt);
        return;
    }

    if (mouseActive_) {
        for (int i = 0; i < int(levels_.size()); ++i) {
            if (levelRect(i).contains(QPointF(mouse_.x, mouse_.y))) {
                selected_ = i;
            }
        }
    }
}

void Menu::render()
{
    if (screen_ == Screen::Match) {
        games_.render();
        return;
    }
    if (screen_ == Screen::Result) {
        games_.render();
        frame_ = games_.frame().copy();
        drawResult();
        return;
    }
    drawMain();
}

QRectF Menu::levelRect(int index) const
{
    constexpr float width = 400.0f;
    constexpr float height = 156.0f;
    constexpr float gap = 28.0f;
    const float total = width * float(levels_.size())
                        + gap * float(levels_.size() - 1);
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + index * (width + gap), 402.0f, width, height);
}

QRectF Menu::retryRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 270.0f, 560.0f, 250, 48);
}

QRectF Menu::menuRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f + 20.0f, 560.0f, 250, 48);
}

void Menu::drawMain()
{
    frame_.fill(QColor(2, 3, 9));
    QPainter painter(&frame_);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient background(0, 0, 0, GameConfig::ScreenHeight);
    background.setColorAt(0.0, QColor(5, 15, 28));
    background.setColorAt(0.55, QColor(3, 8, 18));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     background);

    for (int i = 0; i < 120; ++i) {
        const float x = std::fmod(float(i) * 137.0f + 31.0f,
                                  float(GameConfig::ScreenWidth));
        const float y = std::fmod(float(i) * 83.0f + 17.0f,
                                  float(GameConfig::ScreenHeight));
        const float alpha = 35.0f + 45.0f * (0.5f + 0.5f * std::sin(menuTime_ + i));
        Base::fillCircle(painter, {x, y}, i % 5 == 0 ? 1.8f : 1.0f,
                         Base::withAlpha(Base::Cyan, int(alpha * 0.55f)));
    }

    painter.save();
    painter.setPen(QPen(QColor(25, 56, 76), 1.0f));
    for (int x = 0; x < GameConfig::ScreenWidth; x += 80) {
        painter.drawLine(x, 0, x, GameConfig::ScreenHeight);
    }
    for (int y = 0; y < GameConfig::ScreenHeight; y += 80) {
        painter.drawLine(0, y, GameConfig::ScreenWidth, y);
    }
    painter.restore();

    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("SECTOR 07  //  DEEP SPACE DEFENSE GRID"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 102}, QStringLiteral("center"), 185);
    Base::glowText(painter, Base::titleFont(), QStringLiteral("星核协议"),
                   Base::White,
                   {GameConfig::ScreenWidth / 2.0f, 204}, QStringLiteral("center"));
    Base::text(painter, Base::techFont(),
               QStringLiteral("N E O N   C O R E   P R O T O C O L"),
               Base::Cyan,
               {GameConfig::ScreenWidth / 2.0f, 278}, QStringLiteral("center"), 230);
    Base::text(painter, Base::bodyFont(),
               QStringLiteral("选择行动。每一次部署，都会改写星核的命运。"),
               Base::Text,
               {GameConfig::ScreenWidth / 2.0f, 335}, QStringLiteral("center"), 220);

    Base::text(painter, Base::techSmallFont(), QStringLiteral("SELECT OPERATION"),
               Base::Muted, {GameConfig::ScreenWidth / 2.0f, 375},
               QStringLiteral("center"), 190);
    const float pulse = 0.5f + 0.5f * std::sin(menuTime_ * 2.6f);
    for (int i = 0; i < int(levels_.size()); ++i) {
        const QRectF rect = levelRect(i);
        const bool active = selected_ == i;
        const QColor border = active ? Base::Cyan : QColor(48, 91, 112);
        Base::panel(painter, rect, border, active ? 235 : 190, 12,
                    active ? QColor(8, 22, 36) : QColor(6, 13, 24));
        if (active) {
            painter.save();
            painter.setBrush(Qt::NoBrush);
            QPen pen(Base::withAlpha(Base::Cyan, int(45 + 45 * pulse)));
            pen.setWidthF(3.0f);
            painter.setPen(pen);
            painter.drawRoundedRect(rect.adjusted(-7, -7, 7, 7), 16, 16);
            painter.restore();
        }
        Base::text(painter, Base::techTinyFont(),
                   QStringLiteral("0%1").arg(i + 1), border,
                   {float(rect.left()) + 24.0f, float(rect.top()) + 24.0f},
                   QStringLiteral("topleft"), 210);
        Base::text(painter, Base::mediumFont(), levels_[size_t(i)].name,
                   active ? Base::White : QColor(191, 216, 230),
                   {float(rect.left()) + 24.0f, float(rect.top()) + 54.0f},
                   QStringLiteral("topleft"));
        Base::text(painter, Base::smallFont(), levels_[size_t(i)].subtitle,
                   Base::Muted,
                   {float(rect.left()) + 24.0f, float(rect.top()) + 88.0f},
                   QStringLiteral("topleft"), 215);
        Base::text(painter, Base::techTinyFont(),
                   i == 0 ? QStringLiteral("CAMPAIGN") : QStringLiteral("BOSS RUSH"),
                   Base::shade(border, 0.85f),
                   {float(rect.left()) + 24.0f, float(rect.bottom()) - 20.0f},
                   QStringLiteral("midleft"), 190);
        Base::text(painter, Base::techSmallFont(),
                   active ? QStringLiteral("ENTER  DEPLOY") : QStringLiteral("READY"),
                   active ? Base::Cyan : Base::Muted,
                   {float(rect.right()) - 24.0f, float(rect.bottom()) - 22.0f},
                   QStringLiteral("midright"), 210);
    }

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("WASD 移动    LMB / J 射击    SPACE 冲刺"),
               QColor(105, 138, 158),
               {34, GameConfig::ScreenHeight - 34.0f}, QStringLiteral("midleft"), 150);
    Base::text(painter, Base::tinyFont(),
               QStringLiteral("↑ / ↓ 选择    ENTER 开始    M 静音"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth - 34.0f, GameConfig::ScreenHeight - 34.0f},
               QStringLiteral("midright"), 150);
}

void Menu::drawResult()
{
    QPainter painter(&frame_);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const bool victory = games_.state() == Games::State::Victory;
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 3, 9, 210));
    const QColor accent = victory ? Base::Cyan : Base::Red;
    Base::strokeLine(painter, {GameConfig::ScreenWidth / 2.0f - 250, 118},
                     {GameConfig::ScreenWidth / 2.0f + 250, 118}, accent, 1.0f);
    Base::text(painter, Base::techSmallFont(),
               victory ? QStringLiteral("MISSION COMPLETE  //  CORE STABILIZED")
                       : QStringLiteral("MISSION FAILED  //  CORE SIGNAL LOST"),
               accent, {GameConfig::ScreenWidth / 2.0f, 152},
               QStringLiteral("center"), 230);
    Base::glowText(painter, Base::bigFont(),
                   victory ? QStringLiteral("星核已稳定") : QStringLiteral("防线失守"),
                   victory ? QColor(151, 255, 220) : QColor(255, 119, 133),
                   {GameConfig::ScreenWidth / 2.0f, 222}, QStringLiteral("center"));
    Base::text(painter, Base::bodyFont(),
               victory ? QStringLiteral("你守住了最后一道光。")
                       : QStringLiteral("星核在黑暗中熄灭。"),
               QColor(194, 224, 232),
               {GameConfig::ScreenWidth / 2.0f, 278}, QStringLiteral("center"), 225);
    Base::text(painter, Base::smallFont(), games_.level().name,
               Base::Muted, {GameConfig::ScreenWidth / 2.0f, 310},
               QStringLiteral("center"), 200);

    const int minutes = int(games_.runTime()) / 60;
    const int seconds = int(games_.runTime()) % 60;
    const QString timeText = QStringLiteral("%1:%2")
                                 .arg(minutes, 2, 10, QChar('0'))
                                 .arg(seconds, 2, 10, QChar('0'));
    const std::array<std::pair<QString, QString>, 4> stats = {{
        {QStringLiteral("最终得分"), QLocale(QLocale::English).toString(games_.score())},
        {QStringLiteral("到达波次"),
         QStringLiteral("%1 / %2").arg(games_.currentWave()).arg(games_.totalWaves())},
        {QStringLiteral("击破目标"), QString::number(games_.kills())},
        {QStringLiteral("存活时间"), timeText},
    }};
    constexpr float totalWidth = 760.0f;
    constexpr float cell = totalWidth / 4.0f;
    const float start = GameConfig::ScreenWidth / 2.0f - totalWidth / 2.0f;
    for (int i = 0; i < 4; ++i) {
        const float cx = start + cell * i + cell / 2.0f;
        if (i > 0) {
            Base::strokeLine(painter, {start + cell * i, 390},
                             {start + cell * i, 482}, QColor(44, 67, 83), 1.0f);
        }
        Base::text(painter, Base::tinyFont(), stats[size_t(i)].first, Base::Muted,
                   {cx, 390}, QStringLiteral("center"), 190);
        Base::text(painter, Base::largeFont(), stats[size_t(i)].second, Base::White,
                   {cx, 417}, QStringLiteral("center"));
    }

    Base::panel(painter, retryRect(), Base::Cyan, 230, 9, QColor(7, 18, 31));
    Base::panel(painter, menuRect(), QColor(65, 104, 124), 210, 9, QColor(7, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("R  重新部署"),
               Base::Cyan,
               {float(retryRect().center().x()), float(retryRect().center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回主菜单"),
               QColor(190, 215, 228),
               {float(menuRect().center().x()), float(menuRect().center().y())},
               QStringLiteral("center"));
}

void Menu::startLevel(int index)
{
    if (index < 0 || index >= int(levels_.size())) {
        return;
    }
    selected_ = index;
    screen_ = Screen::Match;
    games_.start(levels_[size_t(index)]);
    games_.keyRelease(Qt::Key_Escape);
}

void Menu::returnToMenu()
{
    screen_ = Screen::Main;
    games_.clearInput();
}

void Menu::keyPress(int key, bool autoRepeat)
{
    if (key == Qt::Key_M && !autoRepeat) {
        toggleMute();
        return;
    }
    if (screen_ == Screen::Main) {
        if (key == Qt::Key_Up || key == Qt::Key_W || key == Qt::Key_Left) {
            selected_ = std::max(0, selected_ - 1);
        } else if (key == Qt::Key_Down || key == Qt::Key_S || key == Qt::Key_Right) {
            selected_ = std::min(int(levels_.size()) - 1, selected_ + 1);
        } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
            startLevel(selected_);
        } else if (key == Qt::Key_1) {
            startLevel(0);
        } else if (key == Qt::Key_2) {
            startLevel(1);
        }
        return;
    }
    if (screen_ == Screen::Result) {
        if (key == Qt::Key_R || key == Qt::Key_Return || key == Qt::Key_Enter) {
            startLevel(selected_);
        } else if (key == Qt::Key_Escape || key == Qt::Key_Q) {
            returnToMenu();
        }
        return;
    }

    games_.keyPress(key, autoRepeat);
    if (key == Qt::Key_Q && games_.state() == Games::State::Paused) {
        returnToMenu();
    }
}

void Menu::keyRelease(int key)
{
    if (screen_ == Screen::Match) {
        games_.keyRelease(key);
    }
}

void Menu::mouseMove(const Math::Vec2 &position)
{
    mouse_ = position;
    mouseActive_ = true;
    if (screen_ == Screen::Match) {
        games_.mouseMove(position);
    }
}

void Menu::mousePressLeft(const Math::Vec2 &position)
{
    mouse_ = position;
    mouseActive_ = true;
    if (screen_ == Screen::Main) {
        for (int i = 0; i < int(levels_.size()); ++i) {
            if (levelRect(i).contains(QPointF(position.x, position.y))) {
                startLevel(i);
                return;
            }
        }
        return;
    }
    if (screen_ == Screen::Result) {
        if (retryRect().contains(QPointF(position.x, position.y))) {
            startLevel(selected_);
        } else if (menuRect().contains(QPointF(position.x, position.y))) {
            returnToMenu();
        }
        return;
    }
    games_.mousePressLeft(position);
}

void Menu::mouseReleaseLeft()
{
    if (screen_ == Screen::Match) {
        games_.mouseReleaseLeft();
    }
}

void Menu::clearInput()
{
    games_.clearInput();
}

void Menu::toggleMute()
{
    games_.toggleMute();
}
