#include "Menu/Editor/Editor.h"

#include "Base/Base.h"
#include "GameConfig.h"
#include "Menu/Editor/Palette.h"
#include "Menu/Editor/Theme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRadialGradient>
#include <Qt>

#include <algorithm>
#include <cmath>

namespace menu {

Editor::Editor()
{
    reset();
}

void Editor::reset()
{
    games::Wave wave;
    wave.enemies.push_back({editor::Palette::kindType(0), 6});
    script_ = {wave};
    row_ = 0;
    column_ = 0;
    hold_ = 0;
    holdTimer_ = 0.0f;
    time_ = 0.0f;
    dragKind_ = -1;
    pointer_.active = false;
}

void Editor::prepare()
{
    row_ = std::clamp(row_, 0, std::max(0, waveCount() - 1));
    clampColumn();
    hold_ = 0;
    holdTimer_ = 0.0f;
    dragKind_ = -1;
    pointer_.active = false;
    mode_ = Mode::Single;
    focus_ = Focus::Grid;
}

Editor::Config Editor::config() const
{
    Config result;
    result.mode = mode_;
    result.host = hostText_.trimmed();
    const uint value = portText_.toUInt();
    result.port = value > 0 && value <= 65535 ? quint16(value) : 0;
    if (mode_ == Mode::Single) {
        result.host.clear();
        result.port = 0;
    } else if (mode_ == Mode::Host) {
        result.host.clear();
    }
    return result;
}

void Editor::setMode(Mode mode)
{
    mode_ = mode;
    focus_ = mode == Mode::Join ? Focus::Host : Focus::Grid;
}

void Editor::setPort(quint16 port)
{
    portText_ = QString::number(std::max<quint16>(1, port));
}

void Editor::setHost(const QString &host)
{
    hostText_ = host;
}

void Editor::cycleMode(int direction)
{
    const int next = (int(mode_) + (direction >= 0 ? 1 : 2)) % 3;
    setMode(Mode(next));
    BaseService.playSound(QStringLiteral("ui"));
}

bool Editor::startValid() const
{
    if (mode_ == Mode::Single) {
        return true;
    }
    if (mode_ == Mode::Join) {
        return hostValid() && portValid();
    }
    return portValid();
}

int Editor::columnCount() const
{
    return int(editor::Palette::visibleKinds(script_).size());
}

int Editor::kindForColumn(int column) const
{
    const std::vector<int> kinds = editor::Palette::visibleKinds(script_);
    if (column < 0 || column >= int(kinds.size())) {
        return -1;
    }
    return kinds[size_t(column)];
}

void Editor::clampColumn()
{
    const int count = columnCount();
    column_ = count <= 0 ? 0 : std::clamp(column_, 0, count - 1);
}

bool Editor::waveHasBoss(int row) const
{
    return editor::WaveTable::waveHasBoss(script_, row);
}

int Editor::enemyCount(int row, int column) const
{
    const int kind = kindForColumn(column);
    return kind < 0 ? 0 : editor::WaveTable::enemyCountKind(script_, row, kind);
}

void Editor::addKindToRow(int row, int kind)
{
    if (row < 0 || row >= waveCount() || kind < 0 || kind >= editor::kKindCount) {
        return;
    }
    row_ = row;
    if (kind == editor::Palette::BossKind) {
        editor::WaveTable::setWaveBoss(script_, row, !waveHasBoss(row));
    } else {
        editor::WaveTable::setEnemyCountKind(
            script_, row, kind,
            std::min(editor::kMaxEnemyCount,
                     editor::WaveTable::enemyCountKind(script_, row, kind) + 1));
    }
    const int column = editor::Palette::columnForKind(script_, kind);
    if (column >= 0) {
        column_ = column;
    }
    clampColumn();
    BaseService.playSound(QStringLiteral("ui"));
}

void Editor::adjustCell(int delta, bool playSound)
{
    if (delta == 0) {
        return;
    }
    const std::vector<int> kinds = editor::Palette::visibleKinds(script_);
    if (kinds.empty()) {
        return;
    }
    clampColumn();
    const int kind = kinds[size_t(column_)];
    if (kind == editor::Palette::BossKind) {
        editor::WaveTable::setWaveBoss(script_, row_, !waveHasBoss(row_));
        if (playSound) {
            BaseService.playSound(QStringLiteral("ui"));
        }
        return;
    }
    const int current = editor::WaveTable::enemyCountKind(script_, row_, kind);
    const int next = std::clamp(current + delta, 0, editor::kMaxEnemyCount);
    if (next == current) {
        return;
    }
    editor::WaveTable::setEnemyCountKind(script_, row_, kind, next);
    if (playSound) {
        BaseService.playSound(QStringLiteral("ui"));
    }
    clampColumn();
}

void Editor::addWave()
{
    if (!waveTable_.addWave(script_)) {
        return;
    }
    row_ = waveCount() - 1;
    clampColumn();
    BaseService.playSound(QStringLiteral("ui"));
}

void Editor::removeWave()
{
    if (!waveTable_.removeWave(script_)) {
        return;
    }
    row_ = std::min(row_, waveCount() - 1);
    clampColumn();
    BaseService.playSound(QStringLiteral("ui"));
}

int Editor::totalEnemies() const
{
    return editor::WaveTable::totalEnemies(script_);
}

bool Editor::portValid() const
{
    const uint value = portText_.toUInt();
    return value > 0 && value <= 65535;
}

bool Editor::hostValid() const
{
    return !hostText_.trimmed().isEmpty();
}

float Editor::pulse() const
{
    return 0.5f + 0.5f * std::sin(time_ * 3.2f);
}

Editor::Action Editor::keyPress(int key, bool autoRepeat)
{
    if (key == Qt::Key_F1) {
        setMode(Mode::Single);
        return Action::None;
    }
    if (key == Qt::Key_F2) {
        setMode(Mode::Host);
        return Action::None;
    }
    if (key == Qt::Key_F3) {
        setMode(Mode::Join);
        return Action::None;
    }
    if (key == Qt::Key_Tab) {
        cycleMode(1);
        return Action::None;
    }
    if (key == Qt::Key_Backtab) {
        cycleMode(-1);
        return Action::None;
    }
    if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
    }
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (startValid()) {
            return Action::Deploy;
        }
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    if (mode_ == Mode::Join || (mode_ == Mode::Host && focus_ == Focus::Port)) {
        if (key == Qt::Key_Backspace) {
            if (focus_ == Focus::Host && !hostText_.isEmpty()) {
                hostText_.chop(1);
            } else if (focus_ == Focus::Port && !portText_.isEmpty()) {
                portText_.chop(1);
            }
            return Action::None;
        }
        QChar character;
        if (key >= Qt::Key_0 && key <= Qt::Key_9) {
            character = QChar(QLatin1Char('0' + char(key - Qt::Key_0)));
        } else if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            character = QChar(QLatin1Char('a' + char(key - Qt::Key_A)));
        } else if (key == Qt::Key_Period) {
            character = QLatin1Char('.');
        } else if (key == Qt::Key_Minus) {
            character = QLatin1Char('-');
        }
        if (!character.isNull()) {
            if (focus_ == Focus::Port) {
                if (character.isDigit() && portText_.size() < 5) {
                    if (portText_ == QStringLiteral("7777")) {
                        portText_.clear();
                    }
                    portText_.append(character);
                }
            } else if (focus_ == Focus::Host
                       && (character.isLetterOrNumber()
                           || character == QLatin1Char('.')
                           || character == QLatin1Char('-')
                           || character == QLatin1Char(':'))
                       && hostText_.size() < 64) {
                hostText_.append(character);
            }
        }
        return Action::None;
    }

    const bool adjustKey = key == Qt::Key_Plus || key == Qt::Key_Equal
                           || key == Qt::Key_Minus;
    // 连发由 update() 自己控制节拍，系统自动重复不再单独处理。
    if (autoRepeat && adjustKey) {
        return Action::None;
    }
    if (!adjustKey) {
        hold_ = 0;
    }
    if (key >= Qt::Key_1 && key <= Qt::Key_9) {
        addKindToRow(row_, key - Qt::Key_1);
    } else if (key == Qt::Key_0) {
        addKindToRow(row_, 9);
    } else if (key == Qt::Key_Up || key == Qt::Key_W) {
        row_ = std::max(0, row_ - 1);
    } else if (key == Qt::Key_Down || key == Qt::Key_S) {
        row_ = std::min(waveCount() - 1, row_ + 1);
    } else if (key == Qt::Key_Left || key == Qt::Key_A) {
        if (columnCount() > 0) {
            column_ = std::max(0, column_ - 1);
        }
    } else if (key == Qt::Key_Right || key == Qt::Key_D) {
        if (columnCount() > 0) {
            column_ = std::min(columnCount() - 1, column_ + 1);
        }
    } else if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
        adjustCell(1);
        hold_ = 1;
        holdTimer_ = RepeatDelay;
    } else if (key == Qt::Key_Minus) {
        adjustCell(-1);
        hold_ = -1;
        holdTimer_ = RepeatDelay;
    } else if (key == Qt::Key_N) {
        addWave();
    } else if (key == Qt::Key_X) {
        removeWave();
    }
    return Action::None;
}

void Editor::keyRelease(int key)
{
    if (key == Qt::Key_Plus || key == Qt::Key_Equal || key == Qt::Key_Minus) {
        hold_ = 0;
    }
}

// 按住 + / - 不放时：先等 RepeatDelay，再按 RepeatInterval 连续调整。
void Editor::update(float dt)
{
    time_ += dt;
    if (hold_ == 0) {
        return;
    }
    holdTimer_ -= dt;
    if (holdTimer_ > 0.0f) {
        return;
    }
    holdTimer_ = RepeatInterval;
    adjustCell(hold_, false);
}

Editor::Action Editor::mousePress(const Math::Vec2 &position)
{
    pointer_.position = position;
    pointer_.active = true;
    const QPointF point(position.x, position.y);
    if (actions_.backRect().contains(point)) {
        return Action::Back;
    }
    if (actions_.deployRect().contains(point)) {
        if (startValid()) {
            return Action::Deploy;
        }
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    for (int mode = 0; mode < editor::Tabs::Count; ++mode) {
        if (tabs_.rect(mode).contains(point)) {
            setMode(Mode(mode));
            BaseService.playSound(QStringLiteral("ui"));
            return Action::None;
        }
    }
    if (mode_ == Mode::Join && editor::JoinPanel::hostRect().contains(point)) {
        focus_ = Focus::Host;
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    if ((mode_ == Mode::Join || mode_ == Mode::Host)
        && (mode_ == Mode::Join ? editor::JoinPanel::portRect()
                                : editor::ServerRow::portRect(waveCount()))
               .contains(point)) {
        focus_ = Focus::Port;
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    focus_ = Focus::Grid;
    if (mode_ == Mode::Join) {
        return Action::None;
    }
    if (actions_.addWaveRect().contains(point)) {
        addWave();
        return Action::None;
    }
    if (actions_.removeWaveRect().contains(point)) {
        removeWave();
        return Action::None;
    }
    for (int kind = 0; kind < editor::kKindCount; ++kind) {
        if (palette_.chipRect(kind).contains(point)) {
            dragKind_ = kind;
            dragPos_ = position;
            hold_ = 0;
            return Action::None;
        }
    }
    hold_ = 0;
    const std::vector<int> kinds = editor::Palette::visibleKinds(script_);
    if (!kinds.empty()) {
        clampColumn();
        const int kind = kinds[size_t(column_)];
        if (kind != editor::Palette::BossKind) {
            const QRectF cell = waveTable_.cellRect(row_, column_, int(kinds.size()));
            if (waveTable_.minusRect(cell).contains(point)) {
                adjustCell(-1);
                hold_ = -1;
                holdTimer_ = RepeatDelay;
                return Action::None;
            }
            if (waveTable_.plusRect(cell).contains(point)) {
                adjustCell(1);
                hold_ = 1;
                holdTimer_ = RepeatDelay;
                return Action::None;
            }
        }
    }
    for (int row = 0; row < waveCount(); ++row) {
        for (int column = 0; column < int(kinds.size()); ++column) {
            if (!waveTable_.cellRect(row, column, int(kinds.size())).contains(point)) {
                continue;
            }
            row_ = row;
            column_ = column;
            if (kinds[size_t(column)] == editor::Palette::BossKind) {
                adjustCell(1);
            }
            return Action::None;
        }
    }
    return Action::None;
}

void Editor::mouseMove(const Math::Vec2 &position)
{
    pointer_.position = position;
    pointer_.active = true;
    if (dragKind_ >= 0) {
        dragPos_ = position;
    }
}

void Editor::mouseRelease()
{
    hold_ = 0;
    if (dragKind_ < 0) {
        return;
    }
    for (int row = 0; row < waveCount(); ++row) {
        if (!waveTable_.rowRect(row).contains(QPointF(dragPos_.x, dragPos_.y))) {
            continue;
        }
        addKindToRow(row, dragKind_);
        break;
    }
    dragKind_ = -1;
}

QRectF Editor::rowRect(int row) const
{
    return waveTable_.rowRect(row);
}

QRectF Editor::paletteChipRect(int kind) const
{
    return palette_.chipRect(kind);
}

void Editor::render(QPainter &painter, bool muted) const
{
    drawBackdrop(painter);
    drawHeader(painter, muted);
    tabs_.draw(painter, mode_, pointer_, pulse());
    drawModeHint(painter);
    actions_.draw(painter, mode_, waveCount(), totalEnemies(), hostText_, portText_,
                  startValid(), pointer_);

    if (mode_ == Mode::Join) {
        joinPanel_.draw(painter, hostText_, portText_, focus_ == Focus::Host,
                        focus_ == Focus::Port, hostValid(), portValid(), pointer_,
                        pulse());
    } else {
        const QColor tint = editor::Theme::accent(mode_);
        const std::vector<int> kinds = editor::Palette::visibleKinds(script_);
        waveTable_.draw(painter, script_, kinds, row_, column_, pointer_, dragKind_,
                        dragPos_, tint);
        // 幽灵在表格之后、端口行/检查器之前绘制，保持原有叠放顺序；
        // 没有任何敌人列时表格提前返回，原实现也不会画幽灵。
        if (dragKind_ >= 0 && !kinds.empty()) {
            palette_.drawGhost(painter, dragKind_, dragPos_);
        }
        if (mode_ == Mode::Host) {
            serverRow_.draw(painter, waveCount(), portText_,
                            focus_ == Focus::Port, portValid(), pointer_, pulse());
        }
        inspector_.draw(painter, script_, row_, mode_, tint);
    }

    palette_.draw(painter, pointer_, dragKind_, editor::Theme::accent(mode_));
    drawFooter(painter);
}

void Editor::drawBackdrop(QPainter &painter) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF screen(0.0f, 0.0f, float(GameConfig::ScreenWidth),
                        float(GameConfig::ScreenHeight));
    painter.fillRect(screen, QColor(2, 3, 9));

    QLinearGradient background(0.0f, 0.0f, 0.0f, float(GameConfig::ScreenHeight));
    background.setColorAt(0.0, QColor(5, 15, 28));
    background.setColorAt(0.6, QColor(3, 8, 18));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.fillRect(screen, background);

    painter.save();
    painter.setPen(QPen(QColor(23, 50, 70), 1.0f));
    for (int x = 0; x < GameConfig::ScreenWidth; x += 80) {
        painter.drawLine(x, 0, x, GameConfig::ScreenHeight);
    }
    for (int y = 0; y < GameConfig::ScreenHeight; y += 80) {
        painter.drawLine(0, y, GameConfig::ScreenWidth, y);
    }
    painter.restore();

    // 选项卡主色的顶部光晕：让每种模式有一块专属的“灯区”。
    const QColor tint = editor::Theme::accent(mode_);
    QRadialGradient bloom(QPointF(GameConfig::ScreenWidth / 2.0f, 54.0f), 520.0f);
    bloom.setColorAt(0.0, Base::withAlpha(tint, 40));
    bloom.setColorAt(0.45, Base::withAlpha(tint, 12));
    bloom.setColorAt(1.0, QColor(tint.red(), tint.green(), tint.blue(), 0));
    painter.fillRect(QRectF(0.0f, 0.0f, float(GameConfig::ScreenWidth), 340.0f), bloom);

    QLinearGradient fade(0.0f, float(GameConfig::ScreenHeight) - 180.0f, 0.0f,
                         float(GameConfig::ScreenHeight));
    fade.setColorAt(0.0, QColor(0, 0, 0, 0));
    fade.setColorAt(1.0, QColor(0, 0, 0, 120));
    painter.fillRect(QRectF(0.0f, float(GameConfig::ScreenHeight) - 180.0f,
                            float(GameConfig::ScreenWidth), 180.0f),
                     fade);
}

void Editor::drawHeader(QPainter &painter, bool muted) const
{
    const float centerX = GameConfig::ScreenWidth / 2.0f;
    Base::text(painter, editor::Theme::techTinyFont(),
               muted ? QStringLiteral("CUSTOM PROTOCOL  //  SCRIPT + SERVER  //  MUTED")
                     : QStringLiteral("CUSTOM PROTOCOL  //  SCRIPT + SERVER"),
               muted ? Base::Amber : QColor(121, 176, 194),
               {centerX, 20.0f}, QStringLiteral("center"), muted ? 215 : 190);

    Base::glowText(painter, editor::Theme::bigFont(), QStringLiteral("自定义协议"),
                   Base::White, {centerX, 56.0f}, QStringLiteral("center"), 250);

    const QColor tint = editor::Theme::accent(mode_);
    Base::strokeLine(painter, {centerX - 170.0f, 86.0f}, {centerX - 34.0f, 86.0f},
                     Base::withAlpha(tint, 130), 1.0f);
    Base::strokeLine(painter, {centerX + 34.0f, 86.0f}, {centerX + 170.0f, 86.0f},
                     Base::withAlpha(tint, 130), 1.0f);
    Base::fillCircle(painter, {centerX - 176.0f, 86.0f}, 2.0f, tint);
    Base::fillCircle(painter, {centerX + 176.0f, 86.0f}, 2.0f, tint);
}

void Editor::drawModeHint(QPainter &painter) const
{
    QString hint;
    switch (mode_) {
    case Mode::Single:
        hint = QStringLiteral("单机：编辑台本并直接开始，不开放端口");
        break;
    case Mode::Host:
        hint = QStringLiteral("局域网主机：编辑台本、设置端口，ENTER 创建房间");
        break;
    case Mode::Join:
        hint = QStringLiteral("加入主机：填写主机地址与端口，连接后由主机推进台本");
        break;
    }

    const QColor tint = editor::Theme::accent(mode_);
    const QRectF pill(GameConfig::ScreenWidth / 2.0f - 380.0f, 144.0f, 760.0f, 26.0f);
    editor::Theme::panel(painter, pill, Base::shade(tint, 0.7f), 170, 13,
                         QColor(5, 13, 24));
    Base::fillCircle(painter, {float(pill.left()) + 18.0f, float(pill.center().y())},
                     3.4f, Base::withAlpha(tint, int(120.0f + 100.0f * pulse())));
    Base::text(painter, editor::Theme::tinyFont(), hint, QColor(190, 211, 227),
               {float(pill.left()) + 30.0f, float(pill.center().y())},
               QStringLiteral("midleft"), 225);
    Base::text(painter, editor::Theme::techTinyFont(),
               QStringLiteral("MODE %1 / 03").arg(int(mode_) + 1), tint,
               {float(pill.right()) - 16.0f, float(pill.center().y())},
               QStringLiteral("midright"), 200);
}

void Editor::drawFooter(QPainter &painter) const
{
    const QString footer =
        mode_ == Mode::Join
            ? QStringLiteral("点击字段输入  ·  BACKSPACE 删除  ·  ENTER 连接  ·  ESC 返回")
            : (mode_ == Mode::Host
                   ? QStringLiteral("F1 / F2 / F3 切换选项卡  ·  TAB 循环  ·  1–9 / 0 添加敌人  ·  ENTER 进入房间  ·  ESC 返回")
                   : QStringLiteral("F1 / F2 / F3 切换选项卡  ·  TAB 循环  ·  1–9 / 0 添加敌人  ·  ENTER 开始  ·  ESC 返回"));
    Base::text(painter, editor::Theme::tinyFont(), footer, QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 24.0f},
               QStringLiteral("center"), 170);
}

} // namespace menu
