#pragma once

#include "Debug.h"

#include "Games/Games.h"
#include "Math.h"
#include "Menu/Editor/Actions.h"
#include "Menu/Editor/Inspector.h"
#include "Menu/Editor/JoinPanel.h"
#include "Menu/Editor/Palette.h"
#include "Menu/Editor/ServerRow.h"
#include "Menu/Editor/Tabs.h"
#include "Menu/Editor/Theme.h"
#include "Menu/Editor/Types.h"
#include "Menu/Editor/WaveTable.h"

#include <QColor>
#include <QRectF>
#include <QString>

#include <vector>

class QPainter;

namespace menu {

// Editor 组件：自定义台本的表格编辑器（公开门面 + 状态机）。
// 表格列不再固定：只显示当前台本里用到的敌人种类；
// 底部“敌人库”提供全部种类，拖到某一波（或按 1–9、0）才会加入该列。
// 顶部 F1/F2/F3 选项卡决定单机 / 局域网主机（波次下方端口） / 加入主机。
// 具体版式和绘制拆到 menu::editor 的同目录组件里，Editor 只组合它们并维护状态。
class Editor {
public:
    using Mode = editor::Mode;

    enum class Action {
        None,
        Deploy,
        Back,
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
    DebugInfo(friend class ::SelfTest;)

    static constexpr float RepeatDelay = 0.24f;
    static constexpr float RepeatInterval = 0.05f;

    void clampColumn();
    int kindForColumn(int column) const;
    void adjustCell(int delta, bool playSound = true);
    void addKindToRow(int row, int kind);
    void addWave();
    void removeWave();
    void cycleMode(int direction);
    bool startValid() const;
    bool portValid() const;
    bool hostValid() const;
    float pulse() const;

    // ----- 编辑器外壳（背景、标题、模式提示、底栏） -----
    void drawBackdrop(QPainter &painter) const;
    void drawHeader(QPainter &painter, bool muted) const;
    void drawModeHint(QPainter &painter) const;
    void drawFooter(QPainter &painter) const;

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
    editor::Pointer pointer_;

    editor::Tabs tabs_;
    editor::WaveTable waveTable_;
    editor::Palette palette_;
    editor::ServerRow serverRow_;
    editor::JoinPanel joinPanel_;
    editor::Inspector inspector_;
    editor::Actions actions_;
};

} // namespace menu
