#pragma once

#include "Arena/Combatants/EnemyType.h"
#include "Games/Games.h"
#include "GameConfig.h"
#include "Math.h"

#include <QColor>
#include <QRectF>
#include <QString>

#include <vector>

class QPainter;

namespace menu {

// Editor 组件：自定义台本的表格编辑器。
// 表格列不再固定：只显示当前台本里用到的敌人种类；
// 底部“敌人库”提供全部种类，拖到某一波（或按 1–9、0）才会加入该列。
// 顶部 F1/F2/F3 选项卡决定单机 / 局域网主机（波次下方端口） / 加入主机。
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

    // ----- 版式（固定 1280x720 逻辑画布） -----
    static constexpr float Left = 90.0f;
    static constexpr float Right = GameConfig::ScreenWidth - 90.0f;
    static constexpr float WaveColumn = 92.0f;
    static constexpr float HeaderY = 236.0f;
    static constexpr float RowsTop = 252.0f;
    static constexpr float RowHeight = 21.0f;
    static constexpr float TabsTop = 94.0f;
    static constexpr float TabWidth = 216.0f;
    static constexpr float TabHeight = 44.0f;
    static constexpr float TabGap = 16.0f;
    static constexpr float ActionsTop = 178.0f;
    static constexpr float ActionsHeight = 36.0f;
    static constexpr float RepeatDelay = 0.24f;
    static constexpr float RepeatInterval = 0.05f;

    static constexpr float PaletteTop = 570.0f;
    static constexpr float PaletteHeight = 96.0f;
    static constexpr float PaletteChipWidth = 138.0f;
    static constexpr float PaletteChipHeight = 32.0f;
    static constexpr float PaletteGap = 9.0f;
    static constexpr float PaletteChipsLeft = Left + 218.0f;

    static arena::EnemyType kindType(int kind);
    static QString kindLabel(int kind);
    static QColor kindColor(int kind);
    static QColor tabAccent(int index);
    static bool waveHasKind(const games::Wave &wave, int kind);

    std::vector<int> visibleKinds() const;
    int kindForColumn(int column) const;
    void clampColumn();
    int enemyCountKind(int row, int kind) const;
    int waveTotal(int row) const;
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
    QRectF portRowRect() const;
    QRectF inspectorRect() const;
    void cycleMode(int direction);
    bool startValid() const;

    // ----- 绘制 -----
    QColor accent() const { return tabAccent(int(mode_)); }
    bool pointerHovering(const QRectF &rect) const;
    bool portValid() const;
    bool hostValid() const;
    float pulse() const;
    void drawBackdrop(QPainter &painter) const;
    void drawHeader(QPainter &painter, bool muted) const;
    void drawTabs(QPainter &painter) const;
    void drawModeHint(QPainter &painter) const;
    void drawActions(QPainter &painter) const;
    void drawButton(QPainter &painter, const QRectF &rect, const QString &key,
                    const QString &label, const QColor &tint, bool primary,
                    bool enabled) const;
    void drawWaveTable(QPainter &painter) const;
    void drawHostPortRow(QPainter &painter) const;
    void drawInspector(QPainter &painter) const;
    void drawPalette(QPainter &painter) const;
    void drawJoinCard(QPainter &painter) const;
    void drawTextField(QPainter &painter, const QRectF &rect, const QString &text,
                       const QString &placeholder, bool focused, bool valid,
                       const QColor &tint) const;

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
    float time_ = 0.0f;     // 只用于选中态的呼吸光，不参与任何逻辑
    int dragKind_ = -1;     // 正在从敌人库拖拽的种类
    Math::Vec2 dragPos_;
    Math::Vec2 pointer_;
    bool pointerActive_ = false;
};

} // namespace menu
