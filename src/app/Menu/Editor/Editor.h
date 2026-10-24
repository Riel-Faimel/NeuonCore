#pragma once

#include "Games/Games.h"
#include "GameConfig.h"
#include "Math.h"

#include <QRectF>
#include <QString>

class Base;
class QPainter;

namespace menu {

// Editor 组件：自定义台本的表格编辑器。
// 自己持有台本数据（初始 1 波），负责绘制、命中测试和编辑操作；
// 是否部署由 Menu 决定，Editor 只把意图作为 Action 交回去。
class Editor {
public:
    enum class Action {
        None,
        Deploy,
        Back,
    };

    Editor();

    void reset();
    void prepare();   // 进入编辑器前把选中行列夹到合法范围
    Action keyPress(int key, Base &base);
    Action mousePress(const Math::Vec2 &position, Base &base);

    void render(QPainter &painter, bool muted) const;

    const games::Script &script() const { return script_; }
    int waveCount() const { return int(script_.size()); }
    int totalEnemies() const;

    // 测试 / 预览用
    int row() const { return row_; }
    int column() const { return column_; }
    int enemyCount(int row, int column) const;
    bool waveHasBoss(int row) const;

private:
    friend class ::SelfTest;

    static constexpr int MaxWaves = 12;
    static constexpr int MaxEnemyCount = 20;
    static constexpr int Columns = 5;   // 四种敌人 + BOSS

    static constexpr float Left = 90.0f;
    static constexpr float Right = GameConfig::ScreenWidth - 90.0f;
    static constexpr float WaveColumn = 88.0f;
    static constexpr float HeaderY = 238.0f;
    static constexpr float RowsTop = 256.0f;
    static constexpr float RowHeight = 30.0f;

    static QString enemyKind(int column);
    void setEnemyCount(int row, int column, int count);
    void setWaveBoss(int row, bool on);
    void adjustCell(int delta, Base &base);
    void addWave(Base &base);
    void removeWave(Base &base);

    QRectF rowRect(int row) const;
    QRectF cellRect(int row, int column) const;
    QRectF minusRect() const;
    QRectF plusRect() const;
    QRectF addWaveRect() const;
    QRectF removeWaveRect() const;
    QRectF deployRect() const;
    QRectF backRect() const;

    games::Script script_;
    int row_ = 0;
    int column_ = 0;
};

} // namespace menu
