#include "Menu/Editor/Editor.h"

#include "Base/Base.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>
#include <array>

namespace menu {

Editor::Editor()
{
    reset();
}

void Editor::reset()
{
    games::Wave wave;
    wave.enemies.push_back({QStringLiteral("drone"), 6});
    script_ = {wave};
    row_ = 0;
    column_ = 0;
}

void Editor::prepare()
{
    row_ = std::clamp(row_, 0, std::max(0, waveCount() - 1));
    column_ = std::clamp(column_, 0, Columns - 1);
}

QString Editor::enemyKind(int column)
{
    static const std::array<QString, 4> kinds = {
        QStringLiteral("drone"), QStringLiteral("spitter"),
        QStringLiteral("tank"), QStringLiteral("splitter")};
    if (column < 0 || column >= int(kinds.size())) {
        return QString();
    }
    return kinds[size_t(column)];
}

int Editor::enemyCount(int row, int column) const
{
    if (row < 0 || row >= waveCount() || column < 0 || column >= 4) {
        return 0;
    }
    const QString kind = enemyKind(column);
    for (const games::EnemySpawn &spawn : script_[size_t(row)].enemies) {
        if (spawn.kind == kind) {
            return spawn.count;
        }
    }
    return 0;
}

void Editor::setEnemyCount(int row, int column, int count)
{
    if (row < 0 || row >= waveCount() || column < 0 || column >= 4) {
        return;
    }
    std::vector<games::EnemySpawn> &enemies = script_[size_t(row)].enemies;
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

bool Editor::waveHasBoss(int row) const
{
    if (row < 0 || row >= waveCount()) {
        return false;
    }
    for (const games::EnemySpawn &spawn : script_[size_t(row)].enemies) {
        if (spawn.kind == QStringLiteral("boss")) {
            return true;
        }
    }
    return false;
}

void Editor::setWaveBoss(int row, bool on)
{
    // 一份台本只允许一个 Boss：先清掉所有波里的 Boss，再按需要在目标波放一个。
    for (games::Wave &wave : script_) {
        wave.enemies.erase(std::remove_if(wave.enemies.begin(), wave.enemies.end(),
                                          [](const games::EnemySpawn &spawn) {
                                              return spawn.kind == QStringLiteral("boss");
                                          }),
                           wave.enemies.end());
    }
    if (on && row >= 0 && row < waveCount()) {
        script_[size_t(row)].enemies.push_back({QStringLiteral("boss"), 1});
    }
}

void Editor::adjustCell(int delta, Base &base)
{
    if (delta == 0) {
        return;
    }
    if (column_ == 4) {
        setWaveBoss(row_, !waveHasBoss(row_));
        base.playSound(QStringLiteral("ui"));
        return;
    }
    const int current = enemyCount(row_, column_);
    const int next = std::clamp(current + delta, 0, MaxEnemyCount);
    if (next == current) {
        return;
    }
    setEnemyCount(row_, column_, next);
    base.playSound(QStringLiteral("ui"));
}

void Editor::addWave(Base &base)
{
    if (waveCount() >= MaxWaves) {
        return;
    }
    games::Wave wave;
    if (!script_.empty()) {
        // 复制最后一波的组成；Boss 不复制，全台本只留一个。
        for (const games::EnemySpawn &spawn : script_.back().enemies) {
            if (spawn.kind != QStringLiteral("boss")) {
                wave.enemies.push_back(spawn);
            }
        }
    }
    script_.push_back(wave);
    row_ = waveCount() - 1;
    base.playSound(QStringLiteral("ui"));
}

void Editor::removeWave(Base &base)
{
    if (waveCount() <= 1) {
        return;
    }
    script_.pop_back();
    row_ = std::min(row_, waveCount() - 1);
    base.playSound(QStringLiteral("ui"));
}

int Editor::totalEnemies() const
{
    int total = 0;
    for (const games::Wave &wave : script_) {
        for (const games::EnemySpawn &spawn : wave.enemies) {
            total += spawn.count;
        }
    }
    return total;
}

Editor::Action Editor::keyPress(int key, Base &base)
{
    if (key == Qt::Key_Up || key == Qt::Key_W) {
        row_ = std::max(0, row_ - 1);
    } else if (key == Qt::Key_Down || key == Qt::Key_S) {
        row_ = std::min(waveCount() - 1, row_ + 1);
    } else if (key == Qt::Key_Left || key == Qt::Key_A) {
        column_ = std::max(0, column_ - 1);
    } else if (key == Qt::Key_Right || key == Qt::Key_D) {
        column_ = std::min(Columns - 1, column_ + 1);
    } else if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
        adjustCell(1, base);
    } else if (key == Qt::Key_Minus) {
        adjustCell(-1, base);
    } else if (key == Qt::Key_N) {
        addWave(base);
    } else if (key == Qt::Key_X) {
        removeWave(base);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        return Action::Deploy;
    } else if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
    }
    return Action::None;
}

Editor::Action Editor::mousePress(const Math::Vec2 &position, Base &base)
{
    const QPointF point(position.x, position.y);
    if (backRect().contains(point)) {
        return Action::Back;
    }
    if (deployRect().contains(point)) {
        return Action::Deploy;
    }
    if (addWaveRect().contains(point)) {
        addWave(base);
        return Action::None;
    }
    if (removeWaveRect().contains(point)) {
        removeWave(base);
        return Action::None;
    }
    if (column_ < 4 && minusRect().contains(point)) {
        adjustCell(-1, base);
        return Action::None;
    }
    if (column_ < 4 && plusRect().contains(point)) {
        adjustCell(1, base);
        return Action::None;
    }
    for (int row = 0; row < waveCount(); ++row) {
        for (int column = 0; column < Columns; ++column) {
            if (!cellRect(row, column).contains(point)) {
                continue;
            }
            row_ = row;
            column_ = column;
            if (column == 4) {
                // BOSS 列整格就是开关。
                adjustCell(1, base);
            }
            return Action::None;
        }
    }
    return Action::None;
}

QRectF Editor::rowRect(int row) const
{
    return QRectF(Left, RowsTop + float(row) * RowHeight, Right - Left, RowHeight);
}

QRectF Editor::cellRect(int row, int column) const
{
    const float width = (Right - Left - WaveColumn) / float(Columns);
    return QRectF(Left + WaveColumn + float(column) * width,
                  RowsTop + float(row) * RowHeight, width, RowHeight);
}

QRectF Editor::minusRect() const
{
    const QRectF cell = cellRect(row_, column_);
    return QRectF(cell.left() + 6.0f, cell.top() + 5.0f, 26.0f, cell.height() - 10.0f);
}

QRectF Editor::plusRect() const
{
    const QRectF cell = cellRect(row_, column_);
    return QRectF(cell.right() - 32.0f, cell.top() + 5.0f, 26.0f, cell.height() - 10.0f);
}

QRectF Editor::addWaveRect() const
{
    return QRectF(Left, 648.0f, 150.0f, 32.0f);
}

QRectF Editor::removeWaveRect() const
{
    return QRectF(Left + 162.0f, 648.0f, 150.0f, 32.0f);
}

QRectF Editor::deployRect() const
{
    return QRectF(Right - 300.0f, 648.0f, 140.0f, 32.0f);
}

QRectF Editor::backRect() const
{
    return QRectF(Right - 148.0f, 648.0f, 148.0f, 32.0f);
}

void Editor::render(QPainter &painter, bool muted) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 3, 9));

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
               muted ? QStringLiteral("SCRIPT EDITOR  //  CUSTOM PROTOCOL  //  MUTED")
                     : QStringLiteral("SCRIPT EDITOR  //  CUSTOM PROTOCOL"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 70}, QStringLiteral("center"), 190);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("自定义协议"),
                   Base::White, {GameConfig::ScreenWidth / 2.0f, 118},
                   QStringLiteral("center"));
    Base::text(painter, Base::smallFont(),
               QStringLiteral("逐波编排敌方信号，ENTER 部署"), Base::Text,
               {GameConfig::ScreenWidth / 2.0f, 154}, QStringLiteral("center"), 215);

    Base::text(painter, Base::techSmallFont(),
               QStringLiteral("%1 波 · %2 个目标")
                   .arg(waveCount(), 2, 10, QChar('0'))
                   .arg(totalEnemies()),
               Base::Muted, {Left, 206}, QStringLiteral("midleft"), 210);

    Base::text(painter, Base::techTinyFont(), QStringLiteral("WAVE"), Base::Muted,
               {Left + 16.0f, HeaderY}, QStringLiteral("midleft"), 200);
    const std::array<QString, Columns> labels = {
        QStringLiteral("DRONE"), QStringLiteral("SPITTER"), QStringLiteral("TANK"),
        QStringLiteral("SPLITTER"), QStringLiteral("BOSS")};
    for (int column = 0; column < Columns; ++column) {
        const QRectF cell = cellRect(0, column);
        Base::text(painter, Base::techTinyFont(), labels[size_t(column)], Base::Muted,
                   {float(cell.center().x()), HeaderY + 6.0f},
                   QStringLiteral("center"), 200);
    }
    Base::strokeLine(painter, {Left, HeaderY + 14.0f},
                     {Right, HeaderY + 14.0f}, QColor(44, 78, 96), 1.0f);

    for (int row = 0; row < waveCount(); ++row) {
        const QRectF rect = rowRect(row);
        const bool activeRow = row == row_;
        painter.fillRect(rect.adjusted(0.0f, 1.0f, 0.0f, -1.0f),
                         activeRow ? QColor(8, 22, 36, 235) : QColor(6, 13, 24, 210));
        Base::text(painter, Base::techSmallFont(),
                   QStringLiteral("%1").arg(row + 1, 2, 10, QChar('0')),
                   activeRow ? Base::Cyan : Base::Muted,
                   {float(rect.left()) + 16.0f, float(rect.center().y())},
                   QStringLiteral("midleft"), 220);

        for (int column = 0; column < Columns; ++column) {
            const QRectF cell = cellRect(row, column);
            const bool active = activeRow && column == column_;
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
                           {float(minusRect().center().x()), float(cell.center().y())},
                           QStringLiteral("center"));
                Base::text(painter, Base::techSmallFont(), QStringLiteral("+"),
                           Base::Cyan,
                           {float(plusRect().center().x()), float(cell.center().y())},
                           QStringLiteral("center"));
            }
            Base::text(painter, Base::techSmallFont(), QString::number(count),
                       numberColor,
                       {float(cell.center().x()), float(cell.center().y())},
                       QStringLiteral("center"), active ? 240 : 210);
        }
    }

    Base::panel(painter, addWaveRect(), QColor(65, 104, 124), 210, 8,
                QColor(7, 14, 25));
    Base::panel(painter, removeWaveRect(), QColor(65, 104, 124), 210, 8,
                QColor(7, 14, 25));
    Base::panel(painter, deployRect(), Base::Cyan, 225, 8, QColor(7, 18, 31));
    Base::panel(painter, backRect(), QColor(65, 104, 124), 210, 8,
                QColor(7, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("N  新增波次"), Base::Text,
               {float(addWaveRect().center().x()), float(addWaveRect().center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("X  删除末波"), Base::Text,
               {float(removeWaveRect().center().x()),
                float(removeWaveRect().center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ENTER  部署"), Base::Cyan,
               {float(deployRect().center().x()), float(deployRect().center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回"),
               QColor(190, 215, 228),
               {float(backRect().center().x()), float(backRect().center().y())},
               QStringLiteral("center"));

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("↑ ↓ 选择波次    ← → 选择列    + / - 调整数量    N 加波    X 删末波    ENTER 部署    ESC 返回"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 22.0f},
               QStringLiteral("center"), 150);
}

} // namespace menu
