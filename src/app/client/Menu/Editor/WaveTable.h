#pragma once

#include "Games/Games.h"
#include "Types.h"

#include <QColor>
#include <QRectF>

#include <vector>

class QPainter;

namespace menu::editor {

// WaveTable 组件：波次表格的行/列几何、单元格计数、Boss 开关与增删波次。
// 它只操作传入的台本数据，不持有选中行/列 —— 那属于 Editor 状态机。
class WaveTable {
public:
    QRectF rowRect(int row) const;
    QRectF cellRect(int row, int column, int columnCount) const;
    QRectF minusRect(const QRectF &cell) const;
    QRectF plusRect(const QRectF &cell) const;

    static bool waveHasBoss(const games::Script &script, int row);
    static void setWaveBoss(games::Script &script, int row, bool on);
    static int enemyCountKind(const games::Script &script, int row, int kind);
    static void setEnemyCountKind(games::Script &script, int row, int kind, int count);
    static int waveTotal(const games::Script &script, int row);
    static int totalEnemies(const games::Script &script);
    static bool addWave(games::Script &script);
    static bool removeWave(games::Script &script);

    void draw(QPainter &painter, const games::Script &script,
              const std::vector<int> &kinds, int row, int column,
              const Pointer &pointer, int dragKind, const Math::Vec2 &dragPos,
              const QColor &tint) const;
};

} // namespace menu::editor
