#pragma once

#include "Arena/Combatants/EnemyType.h"
#include "Games/Games.h"
#include "Types.h"

#include <QColor>
#include <QRectF>
#include <QString>

#include <vector>

class QPainter;

namespace menu::editor {

// Palette 组件：底部“敌人库”。它拥有全部敌人种类的键位/标签/颜色/类型映射，
// 负责芯片格与拖拽幽灵的绘制；拖拽状态本身由 Editor 状态机持有。
class Palette {
public:
    static constexpr int BossKind = 4;

    static int kindCount() { return kKindCount; }
    static arena::EnemyType kindType(int kind);
    static QString kindLabel(int kind);
    static QColor kindColor(int kind);
    static bool waveHasKind(const games::Wave &wave, int kind);
    static std::vector<int> visibleKinds(const games::Script &script);
    static int columnForKind(const games::Script &script, int kind);

    QRectF rect() const;
    QRectF chipRect(int kind) const;

    void draw(QPainter &painter, const Pointer &pointer, int dragKind,
              const QColor &tint) const;
    void drawGhost(QPainter &painter, int kind, const Math::Vec2 &position) const;
};

} // namespace menu::editor
