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
#include <initializer_list>
#include <utility>

Menu::Menu(bool enableAudio)
{
    Base::initialize();
    if (enableAudio) {
        base_.initAudio();
    }
    frame_ = QImage(GameConfig::ScreenWidth, GameConfig::ScreenHeight,
                    QImage::Format_ARGB32_Premultiplied);
    const games::Script campaign = standardScript();
    levels_ = {
        {QStringLiteral("standard"), QStringLiteral("标准协议"),
         QStringLiteral("八波战役 · 守住星核"), 1, campaign},
        {QStringLiteral("boss"), QStringLiteral("核心突袭"),
         QStringLiteral("直接进入最终波次"), int(campaign.size()), campaign},
        {QStringLiteral("custom"), QStringLiteral("自定义协议"),
         QStringLiteral("自选起始波次"), 1, campaign},
    };
    customScript_ = campaign;
}

// 台本是数据：每波列出敌人种类和数量，最后一波带 Boss。
// 想改战役节奏只改这里，Waves / Games 不需要动。
games::Script Menu::standardScript()
{
    const auto wave = [](std::initializer_list<games::EnemySpawn> enemies) {
        games::Wave value;
        value.enemies.assign(enemies.begin(), enemies.end());
        return value;
    };
    return {
        wave({{QStringLiteral("drone"), 6}}),
        wave({{QStringLiteral("drone"), 6}, {QStringLiteral("spitter"), 3}}),
        wave({{QStringLiteral("drone"), 7}, {QStringLiteral("spitter"), 3},
              {QStringLiteral("tank"), 2}}),
        wave({{QStringLiteral("drone"), 7}, {QStringLiteral("spitter"), 4},
              {QStringLiteral("tank"), 2}, {QStringLiteral("splitter"), 2}}),
        wave({{QStringLiteral("drone"), 8}, {QStringLiteral("spitter"), 4},
              {QStringLiteral("tank"), 3}, {QStringLiteral("splitter"), 3}}),
        wave({{QStringLiteral("drone"), 9}, {QStringLiteral("spitter"), 5},
              {QStringLiteral("tank"), 3}, {QStringLiteral("splitter"), 4}}),
        wave({{QStringLiteral("drone"), 10}, {QStringLiteral("spitter"), 5},
              {QStringLiteral("tank"), 4}, {QStringLiteral("splitter"), 4}}),
        wave({{QStringLiteral("drone"), 6}, {QStringLiteral("spitter"), 4},
              {QStringLiteral("tank"), 2}, {QStringLiteral("splitter"), 3},
              {QStringLiteral("boss"), 1}}),
    };
}

int Menu::maxCustomWave() const
{
    return std::max(1, customWaveCount());
}

Menu::~Menu() = default;

const QImage &Menu::frame() const
{
    return screen_ == Screen::Match && games_ ? games_->frame() : frame_;
}

void Menu::update(float dt)
{
    menuTime_ += dt;
    if (screen_ == Screen::Match && games_) {
        games_->update(dt);
        if (games_->state() == Games::State::Victory
            || games_->state() == Games::State::Defeat) {
            screen_ = Screen::Result;
        }
        return;
    }
    if (screen_ == Screen::Result && games_) {
        games_->update(dt);
        return;
    }

    if (screen_ == Screen::Main && mouseActive_) {
        for (int i = 0; i < int(levels_.size()); ++i) {
            if (levelRect(i).contains(QPointF(mouse_.x, mouse_.y))) {
                selected_ = i;
            }
        }
    }
}

void Menu::render()
{
    if (screen_ == Screen::Match && games_) {
        games_->render();
        return;
    }
    if (screen_ == Screen::Result && games_) {
        games_->render();
        frame_ = games_->frame().copy();
        drawResult();
        return;
    }
    if (screen_ == Screen::Editor) {
        drawEditor();
        return;
    }
    drawMain();
}

QRectF Menu::levelRect(int index) const
{
    constexpr float width = 360.0f;
    constexpr float height = 156.0f;
    constexpr float gap = 26.0f;
    const float total = width * float(levels_.size())
                        + gap * float(levels_.size() - 1);
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + index * (width + gap), 402.0f, width, height);
}

QRectF Menu::customMinusRect() const
{
    const QRectF rect = levelRect(CustomLevelIndex);
    return QRectF(float(rect.center().x()) - 126.0f, float(rect.top()) + 84.0f,
                  30.0f, 28.0f);
}

QRectF Menu::customPlusRect() const
{
    const QRectF rect = levelRect(CustomLevelIndex);
    return QRectF(float(rect.center().x()) + 96.0f, float(rect.top()) + 84.0f,
                  30.0f, 28.0f);
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
        if (isCustomLevel(i)) {
            drawCustomPicker(painter, rect, active);
        } else {
            Base::text(painter, Base::smallFont(), levels_[size_t(i)].subtitle,
                       Base::Muted,
                       {float(rect.left()) + 24.0f, float(rect.top()) + 88.0f},
                       QStringLiteral("topleft"), 215);
        }
        const QString tag = i == 0 ? QStringLiteral("CAMPAIGN")
                            : i == 1 ? QStringLiteral("BOSS RUSH")
                                     : QStringLiteral("E  编辑台本");
        Base::text(painter, Base::techTinyFont(), tag,
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
               isCustomLevel(selected_)
                   ? QStringLiteral("← / → 调整起始波次    E 编辑台本    ENTER 开始    M 静音")
                   : QStringLiteral("↑ / ↓ 选择    ENTER 开始    M 静音"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth - 34.0f, GameConfig::ScreenHeight - 34.0f},
               QStringLiteral("midright"), 150);
}

void Menu::drawCustomPicker(QPainter &painter, const QRectF &rect, bool active)
{
    const QRectF minus = customMinusRect();
    const QRectF plus = customPlusRect();
    const QColor border = active ? Base::Cyan : QColor(48, 91, 112);
    const QColor glyph = active ? Base::White : QColor(191, 216, 230);

    Base::panel(painter, minus, border, active ? 230 : 190, 7, QColor(8, 20, 34));
    Base::panel(painter, plus, border, active ? 230 : 190, 7, QColor(8, 20, 34));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("-"), glyph,
               {float(minus.center().x()), float(minus.center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("+"), glyph,
               {float(plus.center().x()), float(plus.center().y())},
               QStringLiteral("center"));

    const QString value = QStringLiteral("起始波次  %1 / %2")
                              .arg(customWave_, 2, 10, QChar('0'))
                              .arg(maxCustomWave(), 2, 10, QChar('0'));
    Base::text(painter, Base::smallFont(), value,
               active ? Base::Cyan : QColor(191, 216, 230),
               {float(rect.center().x()), float(minus.center().y())},
               QStringLiteral("center"), 225);
}

void Menu::adjustCustomWave(int delta)
{
    const int next = std::clamp(customWave_ + delta, 1, maxCustomWave());
    if (next == customWave_) {
        return;
    }
    customWave_ = next;
    base_.playSound(QStringLiteral("ui"));
}

void Menu::drawResult()
{
    if (!games_) {
        return;
    }
    QPainter painter(&frame_);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const bool victory = games_->state() == Games::State::Victory;
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
    Base::text(painter, Base::smallFont(),
               selected_ >= 0 && selected_ < int(levels_.size())
                   ? levels_[size_t(selected_)].name
                   : QString(),
               Base::Muted, {GameConfig::ScreenWidth / 2.0f, 310},
               QStringLiteral("center"), 200);

    const int minutes = int(games_->runTime()) / 60;
    const int seconds = int(games_->runTime()) % 60;
    const QString timeText = QStringLiteral("%1:%2")
                                 .arg(minutes, 2, 10, QChar('0'))
                                 .arg(seconds, 2, 10, QChar('0'));
    const std::array<std::pair<QString, QString>, 4> stats = {{
        {QStringLiteral("最终得分"), QLocale(QLocale::English).toString(games_->score())},
        {QStringLiteral("到达波次"),
         QStringLiteral("%1 / %2").arg(games_->currentWave()).arg(games_->totalWaves())},
        {QStringLiteral("击破目标"), QString::number(games_->kills())},
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
    Games::Level level = levels_[size_t(index)];
    if (isCustomLevel(index)) {
        level.script = customScript_;
        level.startWave = std::clamp(customWave_, 1, maxCustomWave());
    }
    games_ = std::make_unique<Games>(level, base_);
    screen_ = Screen::Match;
    base_.keyRelease(Qt::Key_Escape);
}

void Menu::returnToMenu()
{
    screen_ = Screen::Main;
    games_.reset();
    base_.clear();
}

void Menu::keyPress(int key, bool autoRepeat)
{
    if (key == Qt::Key_M && !autoRepeat) {
        toggleMute();
        return;
    }
    if (screen_ == Screen::Main) {
        const bool custom = isCustomLevel(selected_);
        if (key == Qt::Key_Up || key == Qt::Key_W) {
            selected_ = std::max(0, selected_ - 1);
        } else if (key == Qt::Key_Down || key == Qt::Key_S) {
            selected_ = std::min(int(levels_.size()) - 1, selected_ + 1);
        } else if (key == Qt::Key_Left || key == Qt::Key_A) {
            if (custom) {
                adjustCustomWave(-1);
            } else if (key == Qt::Key_Left) {
                selected_ = std::max(0, selected_ - 1);
            }
        } else if (key == Qt::Key_Right || key == Qt::Key_D) {
            if (custom) {
                adjustCustomWave(1);
            } else if (key == Qt::Key_Right) {
                selected_ = std::min(int(levels_.size()) - 1, selected_ + 1);
            }
        } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
            startLevel(selected_);
        } else if (key == Qt::Key_1) {
            startLevel(0);
        } else if (key == Qt::Key_2) {
            startLevel(1);
        } else if (key == Qt::Key_3) {
            startLevel(2);
        } else if (key == Qt::Key_E && isCustomLevel(selected_)) {
            openEditor();
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

    if (screen_ == Screen::Editor) {
        if (key == Qt::Key_Up || key == Qt::Key_W) {
            editorRow_ = std::max(0, editorRow_ - 1);
        } else if (key == Qt::Key_Down || key == Qt::Key_S) {
            editorRow_ = std::min(customWaveCount() - 1, editorRow_ + 1);
        } else if (key == Qt::Key_Left || key == Qt::Key_A) {
            editorColumn_ = std::max(0, editorColumn_ - 1);
        } else if (key == Qt::Key_Right || key == Qt::Key_D) {
            editorColumn_ = std::min(EditorColumns - 1, editorColumn_ + 1);
        } else if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
            adjustEditorCell(1);
        } else if (key == Qt::Key_Minus) {
            adjustEditorCell(-1);
        } else if (key == Qt::Key_N) {
            addCustomWave();
        } else if (key == Qt::Key_X) {
            removeCustomWave();
        } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            startLevel(CustomLevelIndex);
        } else if (key == Qt::Key_Escape || key == Qt::Key_Q) {
            returnToMenu();
        }
        return;
    }

    if (!games_) {
        return;
    }
    games_->keyPress(key, autoRepeat);
    if (key == Qt::Key_Q && games_->state() == Games::State::Paused) {
        returnToMenu();
    }
}

void Menu::keyRelease(int key)
{
    if (screen_ == Screen::Match && games_) {
        games_->keyRelease(key);
    }
}

void Menu::mouseMove(const Math::Vec2 &position)
{
    mouse_ = position;
    mouseActive_ = true;
    if (screen_ == Screen::Match && games_) {
        games_->mouseMove(position);
    }
}

void Menu::mousePressLeft(const Math::Vec2 &position)
{
    mouse_ = position;
    mouseActive_ = true;
    if (screen_ == Screen::Editor) {
        const QPointF point(position.x, position.y);
        if (editorBackRect().contains(point)) {
            returnToMenu();
            return;
        }
        if (editorDeployRect().contains(point)) {
            startLevel(CustomLevelIndex);
            return;
        }
        if (editorAddWaveRect().contains(point)) {
            addCustomWave();
            return;
        }
        if (editorRemoveWaveRect().contains(point)) {
            removeCustomWave();
            return;
        }
        if (editorColumn_ < 4 && editorMinusRect().contains(point)) {
            adjustEditorCell(-1);
            return;
        }
        if (editorColumn_ < 4 && editorPlusRect().contains(point)) {
            adjustEditorCell(1);
            return;
        }
        for (int row = 0; row < customWaveCount(); ++row) {
            for (int column = 0; column < EditorColumns; ++column) {
                if (!editorCellRect(row, column).contains(point)) {
                    continue;
                }
                editorRow_ = row;
                editorColumn_ = column;
                if (column == 4) {
                    // BOSS 列整格就是开关。
                    adjustEditorCell(1);
                }
                return;
            }
        }
        return;
    }
    if (screen_ == Screen::Main) {
        for (int i = 0; i < int(levels_.size()); ++i) {
            if (isCustomLevel(i)) {
                const QPointF point(position.x, position.y);
                const bool onMinus = customMinusRect().contains(point);
                const bool onPlus = customPlusRect().contains(point);
                if (onMinus || onPlus) {
                    selected_ = i;
                    adjustCustomWave(onMinus ? -1 : 1);
                    return;
                }
            }
            if (isCustomLevel(i) && customEditRect().contains(
                    QPointF(position.x, position.y))) {
                selected_ = i;
                openEditor();
                return;
            }
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
    if (games_) {
        games_->mousePressLeft(position);
    }
}

void Menu::mouseReleaseLeft()
{
    if (screen_ == Screen::Match && games_) {
        games_->mouseReleaseLeft();
    }
}

void Menu::clearInput()
{
    base_.clear();
}

void Menu::toggleMute()
{
    if (!base_.toggleMute()) {
        base_.playSound(QStringLiteral("ui"));
    }
}

void Menu::openEditor()
{
    customWave_ = std::clamp(customWave_, 1, maxCustomWave());
    editorRow_ = std::clamp(editorRow_, 0, customWaveCount() - 1);
    editorColumn_ = std::clamp(editorColumn_, 0, EditorColumns - 1);
    screen_ = Screen::Editor;
    base_.playSound(QStringLiteral("ui"));
}

QString Menu::enemyKind(int column)
{
    static const std::array<QString, 4> kinds = {
        QStringLiteral("drone"), QStringLiteral("spitter"),
        QStringLiteral("tank"), QStringLiteral("splitter")};
    if (column < 0 || column >= int(kinds.size())) {
        return QString();
    }
    return kinds[size_t(column)];
}

int Menu::enemyCount(int row, int column) const
{
    if (row < 0 || row >= customWaveCount() || column < 0 || column >= 4) {
        return 0;
    }
    const QString kind = enemyKind(column);
    for (const games::EnemySpawn &spawn : customScript_[size_t(row)].enemies) {
        if (spawn.kind == kind) {
            return spawn.count;
        }
    }
    return 0;
}

void Menu::setEnemyCount(int row, int column, int count)
{
    if (row < 0 || row >= customWaveCount() || column < 0 || column >= 4) {
        return;
    }
    std::vector<games::EnemySpawn> &enemies = customScript_[size_t(row)].enemies;
    const QString kind = enemyKind(column);
    for (auto it = enemies.begin(); it != enemies.end(); ++it) {
        if (it->kind == kind) {
            if (count <= 0) {
                enemies.erase(it);
            } else {
                it->count = count;
            }
            return;
        }
    }
    if (count > 0) {
        enemies.push_back({kind, count});
    }
}

bool Menu::waveHasBoss(int row) const
{
    if (row < 0 || row >= customWaveCount()) {
        return false;
    }
    for (const games::EnemySpawn &spawn : customScript_[size_t(row)].enemies) {
        if (spawn.kind == QStringLiteral("boss")) {
            return true;
        }
    }
    return false;
}

void Menu::setWaveBoss(int row, bool on)
{
    // 一份台本只允许一个 Boss：先清掉所有波里的 Boss，再按需要在目标波放一个。
    for (games::Wave &wave : customScript_) {
        wave.enemies.erase(std::remove_if(wave.enemies.begin(), wave.enemies.end(),
                                          [](const games::EnemySpawn &spawn) {
                                              return spawn.kind == QStringLiteral("boss");
                                          }),
                           wave.enemies.end());
    }
    if (on && row >= 0 && row < customWaveCount()) {
        customScript_[size_t(row)].enemies.push_back({QStringLiteral("boss"), 1});
    }
}

void Menu::adjustEditorCell(int delta)
{
    if (delta == 0) {
        return;
    }
    if (editorColumn_ == 4) {
        setWaveBoss(editorRow_, !waveHasBoss(editorRow_));
        base_.playSound(QStringLiteral("ui"));
        return;
    }
    const int current = enemyCount(editorRow_, editorColumn_);
    const int next = std::clamp(current + delta, 0, MaxEnemyCount);
    if (next == current) {
        return;
    }
    setEnemyCount(editorRow_, editorColumn_, next);
    base_.playSound(QStringLiteral("ui"));
}

void Menu::addCustomWave()
{
    if (customWaveCount() >= MaxCustomWaves) {
        return;
    }
    games::Wave wave;
    if (!customScript_.empty()) {
        // 复制最后一波的组成；Boss 不复制，全台本只留一个。
        for (const games::EnemySpawn &spawn : customScript_.back().enemies) {
            if (spawn.kind != QStringLiteral("boss")) {
                wave.enemies.push_back(spawn);
            }
        }
    }
    customScript_.push_back(wave);
    editorRow_ = customWaveCount() - 1;
    base_.playSound(QStringLiteral("ui"));
}

void Menu::removeCustomWave()
{
    if (customWaveCount() <= 1) {
        return;
    }
    customScript_.pop_back();
    editorRow_ = std::min(editorRow_, customWaveCount() - 1);
    customWave_ = std::clamp(customWave_, 1, maxCustomWave());
    base_.playSound(QStringLiteral("ui"));
}

int Menu::totalCustomEnemies() const
{
    int total = 0;
    for (const games::Wave &wave : customScript_) {
        for (const games::EnemySpawn &spawn : wave.enemies) {
            total += spawn.count;
        }
    }
    return total;
}

QRectF Menu::editorRowRect(int row) const
{
    return QRectF(EditorLeft, EditorRowsTop + float(row) * EditorRowHeight,
                  EditorRight - EditorLeft, EditorRowHeight);
}

QRectF Menu::editorCellRect(int row, int column) const
{
    const float width = (EditorRight - EditorLeft - EditorWaveColumn)
                        / float(EditorColumns);
    return QRectF(EditorLeft + EditorWaveColumn + float(column) * width,
                  EditorRowsTop + float(row) * EditorRowHeight, width, EditorRowHeight);
}

QRectF Menu::editorMinusRect() const
{
    const QRectF cell = editorCellRect(editorRow_, editorColumn_);
    return QRectF(cell.left() + 6.0f, cell.top() + 5.0f, 26.0f, cell.height() - 10.0f);
}

QRectF Menu::editorPlusRect() const
{
    const QRectF cell = editorCellRect(editorRow_, editorColumn_);
    return QRectF(cell.right() - 32.0f, cell.top() + 5.0f, 26.0f, cell.height() - 10.0f);
}

QRectF Menu::editorAddWaveRect() const
{
    return QRectF(EditorLeft, 648.0f, 150.0f, 32.0f);
}

QRectF Menu::editorRemoveWaveRect() const
{
    return QRectF(EditorLeft + 162.0f, 648.0f, 150.0f, 32.0f);
}

QRectF Menu::editorDeployRect() const
{
    return QRectF(EditorRight - 300.0f, 648.0f, 140.0f, 32.0f);
}

QRectF Menu::editorBackRect() const
{
    return QRectF(EditorRight - 148.0f, 648.0f, 148.0f, 32.0f);
}

QRectF Menu::customEditRect() const
{
    const QRectF rect = levelRect(CustomLevelIndex);
    return QRectF(float(rect.left()) + 16.0f, float(rect.bottom()) - 36.0f,
                  132.0f, 26.0f);
}

void Menu::drawEditor()
{
    frame_.fill(QColor(2, 3, 9));
    QPainter painter(&frame_);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient background(0, 0, 0, GameConfig::ScreenHeight);
    background.setColorAt(0.0, QColor(5, 15, 28));
    background.setColorAt(0.6, QColor(3, 8, 18));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     background);

    painter.save();
    painter.setPen(QPen(QColor(23, 50, 70), 1.0f));
    for (int x = 0; x < GameConfig::ScreenWidth; x += 80) {
        painter.drawLine(x, 0, x, GameConfig::ScreenHeight);
    }
    for (int y = 0; y < GameConfig::ScreenHeight; y += 80) {
        painter.drawLine(0, y, GameConfig::ScreenWidth, y);
    }
    painter.restore();

    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("SCRIPT EDITOR  //  CUSTOM PROTOCOL"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 70}, QStringLiteral("center"), 190);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("自定义协议"),
                   Base::White, {GameConfig::ScreenWidth / 2.0f, 118},
                   QStringLiteral("center"));
    Base::text(painter, Base::smallFont(),
               QStringLiteral("逐波编排敌方信号，ENTER 部署"), Base::Text,
               {GameConfig::ScreenWidth / 2.0f, 154}, QStringLiteral("center"), 215);

    Base::text(painter, Base::techSmallFont(),
               QStringLiteral("%1 波 · %2 个目标 · 起始波次 %3")
                   .arg(customWaveCount(), 2, 10, QChar('0'))
                   .arg(totalCustomEnemies())
                   .arg(customWave_, 2, 10, QChar('0')),
               Base::Muted, {EditorLeft, 206}, QStringLiteral("midleft"), 210);

    Base::text(painter, Base::techTinyFont(), QStringLiteral("WAVE"), Base::Muted,
               {EditorLeft + 16.0f, EditorHeaderY}, QStringLiteral("midleft"), 200);
    const std::array<QString, 5> labels = {
        QStringLiteral("DRONE"), QStringLiteral("SPITTER"), QStringLiteral("TANK"),
        QStringLiteral("SPLITTER"), QStringLiteral("BOSS")};
    for (int column = 0; column < EditorColumns; ++column) {
        const QRectF cell = editorCellRect(0, column);
        Base::text(painter, Base::techTinyFont(), labels[size_t(column)], Base::Muted,
                   {float(cell.center().x()), EditorHeaderY + 6.0f},
                   QStringLiteral("center"), 200);
    }
    Base::strokeLine(painter, {EditorLeft, EditorHeaderY + 14.0f},
                     {EditorRight, EditorHeaderY + 14.0f}, QColor(44, 78, 96), 1.0f);

    for (int row = 0; row < customWaveCount(); ++row) {
        const QRectF rowRect = editorRowRect(row);
        const bool activeRow = row == editorRow_;
        painter.fillRect(rowRect.adjusted(0.0f, 1.0f, 0.0f, -1.0f),
                         activeRow ? QColor(8, 22, 36, 235) : QColor(6, 13, 24, 210));
        Base::text(painter, Base::techSmallFont(),
                   QStringLiteral("%1").arg(row + 1, 2, 10, QChar('0')),
                   activeRow ? Base::Cyan : Base::Muted,
                   {float(rowRect.left()) + 16.0f, float(rowRect.center().y())},
                   QStringLiteral("midleft"), 220);

        for (int column = 0; column < EditorColumns; ++column) {
            const QRectF cell = editorCellRect(row, column);
            const bool active = activeRow && column == editorColumn_;
            if (active) {
                Base::panel(painter, cell.adjusted(3.0f, 3.0f, -3.0f, -3.0f),
                            Base::Cyan, 120, 6, QColor(9, 26, 40));
            }
            if (column == 4) {
                const bool boss = waveHasBoss(row);
                Base::text(painter, Base::techSmallFont(),
                           boss ? QStringLiteral("BOSS") : QStringLiteral("—"),
                           boss ? Base::Magenta : Base::Dim,
                           {float(cell.center().x()), float(cell.center().y())},
                           QStringLiteral("center"), boss ? 235 : 190);
                continue;
            }
            const int count = enemyCount(row, column);
            const QColor numberColor = count > 0 ? (active ? Base::White : Base::Text)
                                                 : Base::Dim;
            if (active) {
                Base::text(painter, Base::techSmallFont(), QStringLiteral("-"),
                           Base::Cyan,
                           {float(editorMinusRect().center().x()),
                            float(cell.center().y())},
                           QStringLiteral("center"));
                Base::text(painter, Base::techSmallFont(), QStringLiteral("+"),
                           Base::Cyan,
                           {float(editorPlusRect().center().x()),
                            float(cell.center().y())},
                           QStringLiteral("center"));
            }
            Base::text(painter, Base::techSmallFont(), QString::number(count),
                       numberColor,
                       {float(cell.center().x()), float(cell.center().y())},
                       QStringLiteral("center"), active ? 240 : 210);
        }
    }

    Base::panel(painter, editorAddWaveRect(), QColor(65, 104, 124), 210, 8,
                QColor(7, 14, 25));
    Base::panel(painter, editorRemoveWaveRect(), QColor(65, 104, 124), 210, 8,
                QColor(7, 14, 25));
    Base::panel(painter, editorDeployRect(), Base::Cyan, 225, 8, QColor(7, 18, 31));
    Base::panel(painter, editorBackRect(), QColor(65, 104, 124), 210, 8,
                QColor(7, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("N  新增波次"), Base::Text,
               {float(editorAddWaveRect().center().x()),
                float(editorAddWaveRect().center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("X  删除末波"), Base::Text,
               {float(editorRemoveWaveRect().center().x()),
                float(editorRemoveWaveRect().center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ENTER  部署"), Base::Cyan,
               {float(editorDeployRect().center().x()),
                float(editorDeployRect().center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回"),
               QColor(190, 215, 228),
               {float(editorBackRect().center().x()),
                float(editorBackRect().center().y())},
               QStringLiteral("center"));

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("↑ ↓ 选择波次    ← → 选择列    + / - 调整数量    N 加波    X 删末波    ENTER 部署    ESC 返回"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 22.0f},
               QStringLiteral("center"), 150);
}
