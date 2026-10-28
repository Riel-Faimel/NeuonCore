#include "Menu/Editor/WaveTable.h"

#include "Base/Base.h"
#include "Menu/Editor/Palette.h"
#include "Menu/Editor/Theme.h"

#include <QPainter>
#include <QPen>
#include <QPointF>
#include <Qt>

#include <algorithm>
#include <cstddef>

namespace menu::editor {

QRectF WaveTable::rowRect(int row) const
{
    return QRectF(Theme::Left, Theme::RowsTop + float(row) * Theme::RowHeight,
                  Theme::Right - Theme::Left, Theme::RowHeight);
}

QRectF WaveTable::cellRect(int row, int column, int columnCount) const
{
    // 列宽按当前列数分配，至少留 5 个槽位，避免只放 1 列时拉得过宽。
    const int slotCount = std::max(5, columnCount);
    const float width = (Theme::Right - Theme::Left - Theme::WaveColumn)
                        / float(slotCount);
    return QRectF(Theme::Left + Theme::WaveColumn + float(column) * width,
                  Theme::RowsTop + float(row) * Theme::RowHeight, width,
                  Theme::RowHeight);
}

QRectF WaveTable::minusRect(const QRectF &cell) const
{
    if (cell.isEmpty()) {
        return QRectF();
    }
    return QRectF(cell.left() + 7.0f, cell.top() + 3.0f, 24.0f,
                  cell.height() - 6.0f);
}

QRectF WaveTable::plusRect(const QRectF &cell) const
{
    if (cell.isEmpty()) {
        return QRectF();
    }
    return QRectF(cell.right() - 31.0f, cell.top() + 3.0f, 24.0f,
                  cell.height() - 6.0f);
}

bool WaveTable::waveHasBoss(const games::Script &script, int row)
{
    if (row < 0 || row >= int(script.size())) {
        return false;
    }
    return Palette::waveHasKind(script[size_t(row)], Palette::BossKind);
}

void WaveTable::setWaveBoss(games::Script &script, int row, bool on)
{
    // 一份台本只允许一个 Boss：先清掉所有波里的 Boss，再按需要在目标波放一个。
    for (games::Wave &wave : script) {
        wave.enemies.erase(std::remove_if(wave.enemies.begin(), wave.enemies.end(),
                                          [](const games::EnemySpawn &spawn) {
                                              return spawn.type
                                                     == Palette::kindType(
                                                         Palette::BossKind);
                                          }),
                           wave.enemies.end());
    }
    if (on && row >= 0 && row < int(script.size())) {
        script[size_t(row)].enemies.push_back(
            {Palette::kindType(Palette::BossKind), 1});
    }
}

int WaveTable::enemyCountKind(const games::Script &script, int row, int kind)
{
    if (row < 0 || row >= int(script.size()) || kind < 0 || kind >= kKindCount) {
        return 0;
    }
    const arena::EnemyType type = Palette::kindType(kind);
    for (const games::EnemySpawn &spawn : script[size_t(row)].enemies) {
        if (spawn.type == type) {
            return spawn.count;
        }
    }
    return 0;
}

void WaveTable::setEnemyCountKind(games::Script &script, int row, int kind,
                                  int count)
{
    if (row < 0 || row >= int(script.size()) || kind < 0 || kind >= kKindCount
        || kind == Palette::BossKind) {
        return;
    }
    std::vector<games::EnemySpawn> &enemies = script[size_t(row)].enemies;
    const arena::EnemyType type = Palette::kindType(kind);
    for (auto it = enemies.begin(); it != enemies.end(); ++it) {
        if (it->type == type) {
            if (count <= 0) {
                enemies.erase(it);
            } else {
                it->count = count;
            }
            return;
        }
    }
    if (count > 0) {
        enemies.push_back({type, count});
    }
}

int WaveTable::waveTotal(const games::Script &script, int row)
{
    if (row < 0 || row >= int(script.size())) {
        return 0;
    }
    int total = 0;
    for (const games::EnemySpawn &spawn : script[size_t(row)].enemies) {
        total += spawn.count;
    }
    return total;
}

int WaveTable::totalEnemies(const games::Script &script)
{
    int total = 0;
    for (const games::Wave &wave : script) {
        for (const games::EnemySpawn &spawn : wave.enemies) {
            total += spawn.count;
        }
    }
    return total;
}

bool WaveTable::addWave(games::Script &script)
{
    if (int(script.size()) >= kMaxWaves) {
        return false;
    }
    games::Wave wave;
    if (!script.empty()) {
        // 复制最后一波的组成；Boss 不复制，全台本只留一个。
        for (const games::EnemySpawn &spawn : script.back().enemies) {
            if (spawn.type != Palette::kindType(Palette::BossKind)) {
                wave.enemies.push_back(spawn);
            }
        }
    }
    script.push_back(wave);
    return true;
}

bool WaveTable::removeWave(games::Script &script)
{
    if (int(script.size()) <= 1) {
        return false;
    }
    script.pop_back();
    return true;
}

void WaveTable::draw(QPainter &painter, const games::Script &script,
                     const std::vector<int> &kinds, int row, int column,
                     const Pointer &pointer, int dragKind,
                     const Math::Vec2 &dragPos, const QColor &tint) const
{
    const int waveCount = int(script.size());

    painter.fillRect(QRectF(Theme::Left, 220.0f, Theme::Right - Theme::Left, 30.0f),
                     QColor(7, 17, 29, 180));
    Base::strokeLine(painter, {Theme::Left, 220.0f}, {Theme::Right, 220.0f},
                     Base::withAlpha(tint, 95), 1.0f);
    Base::strokeLine(painter, {Theme::Left, 250.0f}, {Theme::Right, 250.0f},
                     QColor(40, 76, 96), 1.0f);
    Base::text(painter, Theme::techTinyFont(), QStringLiteral("WAVE"), Base::Muted,
               {Theme::Left + 16.0f, Theme::HeaderY}, QStringLiteral("midleft"), 210);
    Base::text(painter, Theme::techTinyFont(), QStringLiteral("波次"), Base::Dim,
               {Theme::Left + 58.0f, Theme::HeaderY}, QStringLiteral("midleft"), 170);

    for (int col = 0; col < int(kinds.size()); ++col) {
        const int kind = kinds[size_t(col)];
        const QRectF cell = cellRect(0, col, int(kinds.size()));
        const QColor color = Palette::kindColor(kind);
        Base::text(painter, Theme::techTinyFont(), Palette::kindLabel(kind), color,
                   {float(cell.center().x()), Theme::HeaderY - 2.0f},
                   QStringLiteral("center"), 220);
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(Base::withAlpha(color, 170));
        painter.drawRoundedRect(QRectF(float(cell.center().x()) - 12.0f,
                                       Theme::HeaderY + 9.0f, 24.0f, 1.5f),
                                0.8f, 0.8f);
        painter.restore();
    }

    if (!kinds.empty()) {
        const QRectF lastCell = cellRect(0, int(kinds.size()) - 1,
                                         int(kinds.size()));
        if (Theme::Right - float(lastCell.right()) > 320.0f) {
            Base::text(painter, Theme::techTinyFont(),
                       QStringLiteral("单击单元格选中 · 从敌人库拖到波次加入"),
                       QColor(105, 138, 158), {Theme::Right - 16.0f, Theme::HeaderY},
                       QStringLiteral("midright"), 160);
        }
    }

    if (kinds.empty()) {
        // 没有任何敌人列时也照样画出每一波（下面继续走行循环）：
        // 空波次必须是可见、可选中、可拖入的，整张表不能消失。
        Base::strokeLine(painter,
                         {Theme::Left + Theme::WaveColumn, Theme::RowsTop},
                         {Theme::Left + Theme::WaveColumn,
                          Theme::RowsTop + float(waveCount) * Theme::RowHeight},
                         Base::withAlpha(tint, 80), 1.0f);
    }

    for (int waveRow = 0; waveRow < waveCount; ++waveRow) {
        const QRectF rect = rowRect(waveRow);
        const bool activeRow = waveRow == row;
        const bool hoveredRow = pointer.hovering(rect) && dragKind < 0;
        const int total = waveTotal(script, waveRow);
        QColor fill = activeRow ? QColor(9, 24, 38, 235)
                                : (waveRow % 2 == 0 ? QColor(5, 13, 23, 205)
                                                    : QColor(7, 16, 28, 205));
        if (hoveredRow && !activeRow) {
            fill = QColor(11, 24, 38, 228);
        }
        painter.fillRect(rect, fill);
        if (activeRow) {
            painter.fillRect(QRectF(Theme::Left, float(rect.top()) + 1.0f, 3.0f,
                                    float(rect.height()) - 2.0f),
                             tint);
        }
        Base::strokeLine(painter, {Theme::Left + 10.0f, float(rect.bottom())},
                         {Theme::Right - 10.0f, float(rect.bottom())},
                         QColor(30, 55, 74, 110), 1.0f);

        Base::text(painter, Theme::techSmallFont(),
                   QStringLiteral("%1").arg(waveRow + 1, 2, 10, QChar('0')),
                   activeRow ? tint : Base::Muted,
                   {Theme::Left + 18.0f, float(rect.center().y())},
                   QStringLiteral("midleft"), activeRow ? 240 : 210);
        // 空波次在波次号和总量之间打一个“空”标记，避免 0 被误读成“还没填”。
        if (total == 0 && !kinds.empty()) {
            Base::text(painter, Theme::techTinyFont(), QStringLiteral("空"),
                       activeRow ? Base::withAlpha(tint, 220)
                                 : Base::withAlpha(Base::Amber, 150),
                       {Theme::Left + 42.0f, float(rect.center().y())},
                       QStringLiteral("midleft"), 200);
        }
        Base::text(painter, Theme::techTinyFont(),
                   QStringLiteral("×%1").arg(total),
                   activeRow ? tint : Base::Dim,
                   {Theme::Left + Theme::WaveColumn - 12.0f,
                    float(rect.center().y())},
                   QStringLiteral("midright"), activeRow ? 215 : 165);

        if (kinds.empty()) {
            // 没有列时，每一行自己说明状态：空波次仍然是一行可选中的波次。
            Base::text(painter, Theme::bodyFont(),
                       QStringLiteral("空波次 · 拖动敌人到这一波"),
                       activeRow ? tint : Base::Muted,
                       {Theme::Left + Theme::WaveColumn + 12.0f,
                        float(rect.center().y())},
                       QStringLiteral("midleft"), activeRow ? 235 : 185);
            continue;
        }

        for (int col = 0; col < int(kinds.size()); ++col) {
            const int kind = kinds[size_t(col)];
            const QRectF cell = cellRect(waveRow, col, int(kinds.size()));
            const bool activeCell = activeRow && col == column;
            const bool hoverCell = pointer.hovering(cell) && dragKind < 0;
            const QColor color = Palette::kindColor(kind);

            if (activeCell) {
                painter.save();
                painter.setBrush(Base::withAlpha(color, 28));
                painter.setPen(QPen(Base::withAlpha(color, 190), 1.2f));
                painter.drawRoundedRect(cell.adjusted(3.0f, 2.0f, -3.0f, -2.0f),
                                        5.0f, 5.0f);
                painter.restore();
            } else if (hoverCell) {
                painter.save();
                painter.setBrush(QColor(40, 80, 100, 28));
                painter.setPen(QPen(QColor(70, 120, 145, 85), 1.0f));
                painter.drawRoundedRect(cell.adjusted(4.0f, 2.0f, -4.0f, -2.0f),
                                        5.0f, 5.0f);
                painter.restore();
            }

            if (kind == Palette::BossKind) {
                const bool boss = waveHasBoss(script, waveRow);
                Base::text(painter, Theme::techSmallFont(),
                           boss ? QStringLiteral("BOSS") : QStringLiteral("—"),
                           boss ? Base::Magenta : Base::Dim,
                           {float(cell.center().x()), float(cell.center().y())},
                           QStringLiteral("center"), boss ? 240 : 180);
                continue;
            }

            const int count = enemyCountKind(script, waveRow, kind);
            if (activeCell) {
                const auto step = [&](const QRectF &button, const QString &glyph) {
                    const bool hot = pointer.hovering(button);
                    painter.save();
                    painter.setBrush(hot ? Base::withAlpha(color, 55)
                                         : QColor(3, 11, 19, 230));
                    painter.setPen(QPen(hot ? color : Base::withAlpha(color, 120),
                                        1.0f));
                    painter.drawRoundedRect(button, 4.0f, 4.0f);
                    painter.restore();
                    Base::text(painter, Theme::techSmallFont(), glyph,
                               hot ? Base::White : color,
                               {float(button.center().x()),
                                float(button.center().y())},
                               QStringLiteral("center"), hot ? 245 : 220);
                };
                step(minusRect(cell), QStringLiteral("-"));
                step(plusRect(cell), QStringLiteral("+"));
                Base::text(painter, Theme::techSmallFont(), QString::number(count),
                           Base::White,
                           {float(cell.center().x()), float(cell.center().y())},
                           QStringLiteral("center"), 245);
            } else {
                Base::text(painter, Theme::techSmallFont(), QString::number(count),
                           count > 0 ? Base::Text : Base::Dim,
                           {float(cell.center().x()), float(cell.center().y())},
                           QStringLiteral("center"), count > 0 ? 215 : 175);
            }
        }
    }

    if (kinds.empty()) {
        // 表格下方补一句操作提示：表格本身已经把每一波画出来了，这里只说明怎么加列。
        Base::text(painter, Theme::tinyFont(),
                   QStringLiteral("还没有敌人列：把下方敌人库拖到某一波，或按 1–9 / 0 添加"),
                   Base::Muted,
                   {Theme::Left + 18.0f,
                    Theme::RowsTop + float(waveCount) * Theme::RowHeight + 14.0f},
                   QStringLiteral("midleft"), 190);
    }

    if (dragKind >= 0) {
        for (int waveRow = 0; waveRow < waveCount; ++waveRow) {
            if (!rowRect(waveRow).contains(QPointF(dragPos.x, dragPos.y))) {
                continue;
            }
            const QColor color = Palette::kindColor(dragKind);
            const QRectF target = rowRect(waveRow);
            painter.save();
            painter.setBrush(Base::withAlpha(color, 30));
            painter.setPen(QPen(Base::withAlpha(color, 235), 2.0f));
            painter.drawRoundedRect(target.adjusted(1.0f, 1.0f, -1.0f, -1.0f),
                                    5.0f, 5.0f);
            painter.restore();
            Base::text(painter, Theme::techTinyFont(),
                       QStringLiteral("释放加入 第 %1 波").arg(waveRow + 1), color,
                       {float(target.right()) - 18.0f, float(target.center().y())},
                       QStringLiteral("midright"), 230);
            break;
        }
    }
}

} // namespace menu::editor
