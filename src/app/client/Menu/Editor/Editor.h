#pragma once

#include "Arena/Combatants/EnemyType.h"
#include "Games/Games.h"
#include "GameConfig.h"
#include "Math.h"

#include <QRectF>
#include <QString>

#include <vector>

class QPainter;

namespace menu {

// Editor 组件：自定义台本的表格编辑器。
// 表格列不再固定：只显示当前台本里用到的敌人种类；
// 底部“敌人库”提供全部种类，拖到某一波（或按 1–5）才会加入该列。
class Editor {
public:
    enum class Action {
        None,
        Deploy,
        Back,
    };

    enum class Mode {
        Single,
        Host,
        Join,
    };

    struct Config {
        Mode mode = Mode::Single;
        QString host = QStringLiteral("127.0.0.1");
        quint16 port = 7777;
    };

    Editor();

    void reset();
    void prepare();
    Action keyPress(int key, bool autoRepeat = false);
    void keyRelease(int key);
    void update(float dt);

    Action mousePress(const Math::Vec2 &position);
    void mouseMove(const Math::Vec2 &position);
    void mouseRelease();

    void render(QPainter &painter, bool muted) const;

    Mode mode() const { return mode_; }
    Config config() const;
    void setMode(Mode mode);
    void setPort(quint16 port);
    void setHost(const QString &host);
    QString portText() const { return portText_; }
    QString hostText() const { return hostText_; }

    const games::Script &script() const { return script_; }
    int waveCount() const { return int(script_.size()); }
    int totalEnemies() const;

    int row() const { return row_; }
    int column() const { return column_; }
    int columnCount() const;
    int enemyCount(int row, int column) const;
    bool waveHasBoss(int row) const;

    // 测试 / 预览用
    QRectF paletteChipRect(int kind) const;
    QRectF rowRect(int row) const;

private:
    friend class ::SelfTest;

    static constexpr int MaxWaves = 12;
    static constexpr int MaxEnemyCount = 20;
    static constexpr int Kinds = 11;  // 基础 5 种 + 6 种支援/特殊敌人

    static constexpr float Left = 90.0f;
    static constexpr float Right = GameConfig::ScreenWidth - 90.0f;
    static constexpr float WaveColumn = 88.0f;
    static constexpr float HeaderY = 200.0f;
    static constexpr float RowsTop = 218.0f;
    static constexpr float RowHeight = 24.0f;
    static constexpr float RepeatDelay = 0.24f;
    static constexpr float RepeatInterval = 0.05f;

    static constexpr float PaletteTop = 576.0f;
    static constexpr float PaletteChipWidth = 138.0f;
    static constexpr float PaletteChipHeight = 32.0f;
    static constexpr float PaletteGap = 10.0f;
    static constexpr float PaletteChipsLeft = Left + 220.0f;

    static arena::EnemyType kindType(int kind);
    static QString kindLabel(int kind);
    static QColor kindColor(int kind);
    static bool waveHasKind(const games::Wave &wave, int kind);

    std::vector<int> visibleKinds() const;
    int kindForColumn(int column) const;
    void clampColumn();
    int enemyCountKind(int row, int kind) const;
    void setEnemyCountKind(int row, int kind, int count);
    void setWaveBoss(int row, bool on);
    void addKindToRow(int row, int kind);
    void adjustCell(int delta, bool playSound = true);
    void addWave();
    void removeWave();

    QRectF cellRect(int row, int column) const;
    QRectF minusRect() const;
    QRectF plusRect() const;
    QRectF addWaveRect() const;
    QRectF removeWaveRect() const;
    QRectF deployRect() const;
    QRectF backRect() const;
    QRectF paletteRect() const;
    QRectF tabRect(int index) const;
    QRectF portRect() const;
    QRectF hostRect() const;
    void cycleMode(int direction);
    bool startValid() const;

    enum class Focus {
        Grid,
        Port,
        Host,
    };

    games::Script script_;
    Mode mode_ = Mode::Single;
    Focus focus_ = Focus::Grid;
    QString portText_ = QStringLiteral("7777");
    QString hostText_ = QStringLiteral("127.0.0.1");
    int row_ = 0;
    int column_ = 0;
    int hold_ = 0;          // 当前按住的方向（+1 / -1），0 表示没有按住
    float holdTimer_ = 0.0f;
    int dragKind_ = -1;     // 正在从敌人库拖拽的种类
    Math::Vec2 dragPos_;
};

} // namespace menu
