#include "Menu/Editor/Editor.h"

#include "Arena/Combatants/Enemies/DroneEnemy.h"
#include "Arena/Combatants/Enemy.h"
#include "Base/Base.h"

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <Qt>

#include <algorithm>
#include <array>
#include <cmath>

namespace menu {

namespace {
const QString kKeys[11] = {
    QStringLiteral("drone"),       QStringLiteral("spitter"),
    QStringLiteral("tank"),        QStringLiteral("splitter"),
    QStringLiteral("boss"),        QStringLiteral("amplifier"),
    QStringLiteral("linker"),      QStringLiteral("mender"),
    QStringLiteral("rusher"),      QStringLiteral("necromancer"),
    QStringLiteral("summoner")};
const QString kLabels[11] = {
    QStringLiteral("DRONE"),       QStringLiteral("SPITTER"),
    QStringLiteral("TANK"),        QStringLiteral("SPLITTER"),
    QStringLiteral("BOSS"),        QStringLiteral("AMPLIFIER"),
    QStringLiteral("LINKER"),      QStringLiteral("MENDER"),
    QStringLiteral("RUSHER"),      QStringLiteral("NECRO"),
    QStringLiteral("SUMMONER")};
const QString kTabLabels[3] = {
    QStringLiteral("单机"), QStringLiteral("局域网主机"),
    QStringLiteral("加入主机")};
const QString kTabSubtitles[3] = {
    QStringLiteral("LOCAL SCRIPT"), QStringLiteral("HOST SESSION"),
    QStringLiteral("JOIN SESSION")};
const QString kTabKeys[3] = {
    QStringLiteral("F1"), QStringLiteral("F2"), QStringLiteral("F3")};
} // namespace

Editor::Editor()
{
    reset();
}

void Editor::reset()
{
    games::Wave wave;
    wave.enemies.push_back({kindType(0), 6});
    script_ = {wave};
    row_ = 0;
    column_ = 0;
    hold_ = 0;
    holdTimer_ = 0.0f;
    time_ = 0.0f;
    dragKind_ = -1;
    pointerActive_ = false;
}

void Editor::prepare()
{
    row_ = std::clamp(row_, 0, std::max(0, waveCount() - 1));
    clampColumn();
    hold_ = 0;
    holdTimer_ = 0.0f;
    dragKind_ = -1;
    pointerActive_ = false;
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

arena::EnemyType Editor::kindType(int kind)
{
    const QString key = kind >= 0 && kind < Kinds ? kKeys[size_t(kind)] : kKeys[0];
    const auto type = arena::Enemy::typeForKey(key);
    return type ? *type : arena::EnemyType(typeid(arena::DroneEnemy));
}

QString Editor::kindLabel(int kind)
{
    return kind >= 0 && kind < Kinds ? kLabels[size_t(kind)] : QString();
}

QColor Editor::kindColor(int kind)
{
    switch (kind) {
    case 0:
        return Base::Coral;
    case 1:
        return Base::Magenta;
    case 2:
        return Base::Amber;
    case 3:
        return Base::Violet;
    case 4:
        return QColor(255, 74, 160);
    case 5:
        return QColor(120, 220, 255);
    case 6:
        return QColor(150, 160, 255);
    case 7:
        return QColor(120, 255, 178);
    case 8:
        return QColor(255, 140, 80);
    case 9:
        return QColor(190, 120, 255);
    default:
        return QColor(255, 200, 90);
    }
}

QColor Editor::tabAccent(int index)
{
    switch (index) {
    case 0:
        return Base::Cyan;
    case 1:
        return Base::Violet;
    default:
        return Base::Amber;
    }
}

bool Editor::waveHasKind(const games::Wave &wave, int kind)
{
    const arena::EnemyType type = kindType(kind);
    for (const games::EnemySpawn &spawn : wave.enemies) {
        if (spawn.type == type) {
            return true;
        }
    }
    return false;
}

std::vector<int> Editor::visibleKinds() const
{
    std::vector<int> kinds;
    for (int kind = 0; kind < Kinds; ++kind) {
        for (const games::Wave &wave : script_) {
            if (waveHasKind(wave, kind)) {
                kinds.push_back(kind);
                break;
            }
        }
    }
    return kinds;
}

int Editor::columnCount() const
{
    return int(visibleKinds().size());
}

int Editor::kindForColumn(int column) const
{
    const std::vector<int> kinds = visibleKinds();
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

int Editor::enemyCountKind(int row, int kind) const
{
    if (row < 0 || row >= waveCount() || kind < 0 || kind >= Kinds) {
        return 0;
    }
    const arena::EnemyType type = kindType(kind);
    for (const games::EnemySpawn &spawn : script_[size_t(row)].enemies) {
        if (spawn.type == type) {
            return spawn.count;
        }
    }
    return 0;
}

void Editor::setEnemyCountKind(int row, int kind, int count)
{
    if (row < 0 || row >= waveCount() || kind < 0 || kind >= Kinds || kind == 4) {
        return;
    }
    std::vector<games::EnemySpawn> &enemies = script_[size_t(row)].enemies;
    const arena::EnemyType type = kindType(kind);
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

bool Editor::waveHasBoss(int row) const
{
    if (row < 0 || row >= waveCount()) {
        return false;
    }
    return waveHasKind(script_[size_t(row)], 4);
}

void Editor::setWaveBoss(int row, bool on)
{
    // 一份台本只允许一个 Boss：先清掉所有波里的 Boss，再按需要在目标波放一个。
    for (games::Wave &wave : script_) {
        wave.enemies.erase(std::remove_if(wave.enemies.begin(), wave.enemies.end(),
                                          [](const games::EnemySpawn &spawn) {
                                              return spawn.type == kindType(4);
                                          }),
                           wave.enemies.end());
    }
    if (on && row >= 0 && row < waveCount()) {
        script_[size_t(row)].enemies.push_back({kindType(4), 1});
    }
    clampColumn();
}

int Editor::enemyCount(int row, int column) const
{
    const int kind = kindForColumn(column);
    return kind < 0 ? 0 : enemyCountKind(row, kind);
}

void Editor::addKindToRow(int row, int kind)
{
    if (row < 0 || row >= waveCount() || kind < 0 || kind >= Kinds) {
        return;
    }
    row_ = row;
    if (kind == 4) {
        setWaveBoss(row, !waveHasBoss(row));
    } else {
        setEnemyCountKind(row, kind,
                          std::min(MaxEnemyCount, enemyCountKind(row, kind) + 1));
    }
    const std::vector<int> kinds = visibleKinds();
    for (int i = 0; i < int(kinds.size()); ++i) {
        if (kinds[size_t(i)] == kind) {
            column_ = i;
            break;
        }
    }
    clampColumn();
    BaseService.playSound(QStringLiteral("ui"));
}

void Editor::adjustCell(int delta, bool playSound)
{
    if (delta == 0) {
        return;
    }
    const std::vector<int> kinds = visibleKinds();
    if (kinds.empty()) {
        return;
    }
    clampColumn();
    const int kind = kinds[size_t(column_)];
    if (kind == 4) {
        setWaveBoss(row_, !waveHasBoss(row_));
        if (playSound) {
            BaseService.playSound(QStringLiteral("ui"));
        }
        return;
    }
    const int current = enemyCountKind(row_, kind);
    const int next = std::clamp(current + delta, 0, MaxEnemyCount);
    if (next == current) {
        return;
    }
    setEnemyCountKind(row_, kind, next);
    if (playSound) {
        BaseService.playSound(QStringLiteral("ui"));
    }
    clampColumn();
}

void Editor::addWave()
{
    if (waveCount() >= MaxWaves) {
        return;
    }
    games::Wave wave;
    if (!script_.empty()) {
        // 复制最后一波的组成；Boss 不复制，全台本只留一个。
        for (const games::EnemySpawn &spawn : script_.back().enemies) {
            if (spawn.type != kindType(4)) {
                wave.enemies.push_back(spawn);
            }
        }
    }
    script_.push_back(wave);
    row_ = waveCount() - 1;
    clampColumn();
    BaseService.playSound(QStringLiteral("ui"));
}

void Editor::removeWave()
{
    if (waveCount() <= 1) {
        return;
    }
    script_.pop_back();
    row_ = std::min(row_, waveCount() - 1);
    clampColumn();
    BaseService.playSound(QStringLiteral("ui"));
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

int Editor::waveTotal(int row) const
{
    if (row < 0 || row >= waveCount()) {
        return 0;
    }
    int total = 0;
    for (const games::EnemySpawn &spawn : script_[size_t(row)].enemies) {
        total += spawn.count;
    }
    return total;
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
    pointer_ = position;
    pointerActive_ = true;
    const QPointF point(position.x, position.y);
    if (backRect().contains(point)) {
        return Action::Back;
    }
    if (deployRect().contains(point)) {
        if (startValid()) {
            return Action::Deploy;
        }
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    for (int mode = 0; mode < 3; ++mode) {
        if (tabRect(mode).contains(point)) {
            setMode(Mode(mode));
            BaseService.playSound(QStringLiteral("ui"));
            return Action::None;
        }
    }
    if (mode_ == Mode::Join && hostRect().contains(point)) {
        focus_ = Focus::Host;
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    if ((mode_ == Mode::Join || mode_ == Mode::Host)
        && portRect().contains(point)) {
        focus_ = Focus::Port;
        BaseService.playSound(QStringLiteral("ui"));
        return Action::None;
    }
    focus_ = Focus::Grid;
    if (mode_ == Mode::Join) {
        return Action::None;
    }
    if (addWaveRect().contains(point)) {
        addWave();
        return Action::None;
    }
    if (removeWaveRect().contains(point)) {
        removeWave();
        return Action::None;
    }
    for (int kind = 0; kind < Kinds; ++kind) {
        if (paletteChipRect(kind).contains(point)) {
            dragKind_ = kind;
            dragPos_ = position;
            hold_ = 0;
            return Action::None;
        }
    }
    hold_ = 0;
    const std::vector<int> kinds = visibleKinds();
    if (!kinds.empty()) {
        clampColumn();
        const int kind = kinds[size_t(column_)];
        if (kind != 4) {
            if (minusRect().contains(point)) {
                adjustCell(-1);
                hold_ = -1;
                holdTimer_ = RepeatDelay;
                return Action::None;
            }
            if (plusRect().contains(point)) {
                adjustCell(1);
                hold_ = 1;
                holdTimer_ = RepeatDelay;
                return Action::None;
            }
        }
    }
    for (int row = 0; row < waveCount(); ++row) {
        for (int column = 0; column < int(kinds.size()); ++column) {
            if (!cellRect(row, column).contains(point)) {
                continue;
            }
            row_ = row;
            column_ = column;
            if (kinds[size_t(column)] == 4) {
                adjustCell(1);
            }
            return Action::None;
        }
    }
    return Action::None;
}

void Editor::mouseMove(const Math::Vec2 &position)
{
    pointer_ = position;
    pointerActive_ = true;
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
        if (!rowRect(row).contains(QPointF(dragPos_.x, dragPos_.y))) {
            continue;
        }
        addKindToRow(row, dragKind_);
        break;
    }
    dragKind_ = -1;
}

QRectF Editor::rowRect(int row) const
{
    return QRectF(Left, RowsTop + float(row) * RowHeight, Right - Left, RowHeight);
}

QRectF Editor::cellRect(int row, int column) const
{
    // 列宽按当前列数分配，至少留 5 个槽位，避免只放 1 列时拉得过宽。
    const int slotCount = std::max(5, columnCount());
    const float width = (Right - Left - WaveColumn) / float(slotCount);
    return QRectF(Left + WaveColumn + float(column) * width,
                  RowsTop + float(row) * RowHeight, width, RowHeight);
}

QRectF Editor::minusRect() const
{
    if (columnCount() == 0) {
        return QRectF();
    }
    const QRectF cell = cellRect(row_, column_);
    return QRectF(cell.left() + 7.0f, cell.top() + 3.0f, 24.0f, cell.height() - 6.0f);
}

QRectF Editor::plusRect() const
{
    if (columnCount() == 0) {
        return QRectF();
    }
    const QRectF cell = cellRect(row_, column_);
    return QRectF(cell.right() - 31.0f, cell.top() + 3.0f, 24.0f,
                  cell.height() - 6.0f);
}

QRectF Editor::addWaveRect() const
{
    return QRectF(Right - 648.0f, ActionsTop, 136.0f, ActionsHeight);
}

QRectF Editor::removeWaveRect() const
{
    return QRectF(Right - 498.0f, ActionsTop, 136.0f, ActionsHeight);
}

QRectF Editor::deployRect() const
{
    return QRectF(Right - 348.0f, ActionsTop, 176.0f, ActionsHeight);
}

QRectF Editor::backRect() const
{
    return QRectF(Right - 158.0f, ActionsTop, 158.0f, ActionsHeight);
}

QRectF Editor::tabRect(int index) const
{
    const float total = TabWidth * 3.0f + TabGap * 2.0f;
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + float(index) * (TabWidth + TabGap), TabsTop,
                  TabWidth, TabHeight);
}

QRectF Editor::portRowRect() const
{
    const float top = std::min(508.0f,
                               RowsTop + float(waveCount()) * RowHeight + 4.0f);
    return QRectF(Left, top, Right - Left, 48.0f);
}

QRectF Editor::portRect() const
{
    if (mode_ == Mode::Join) {
        return QRectF(774.0f, 336.0f, 190.0f, 44.0f);
    }
    const QRectF row = portRowRect();
    return QRectF(Left + 360.0f, float(row.top()) + 4.0f, 250.0f, 40.0f);
}

QRectF Editor::hostRect() const
{
    return QRectF(314.0f, 336.0f, 440.0f, 44.0f);
}

QRectF Editor::inspectorRect() const
{
    const float tableBottom = RowsTop + float(waveCount()) * RowHeight;
    float top = std::max(388.0f, tableBottom + 18.0f);
    if (mode_ == Mode::Host) {
        top = std::max(top, float(portRowRect().bottom()) + 18.0f);
    }
    const float height = std::min(150.0f, PaletteTop - 14.0f - top);
    if (height < 116.0f) {
        return QRectF();
    }
    return QRectF(Left, top, Right - Left, height);
}

QRectF Editor::paletteRect() const
{
    return QRectF(Left, PaletteTop, Right - Left, PaletteHeight);
}

QRectF Editor::paletteChipRect(int kind) const
{
    const int row = kind / 6;
    const int column = kind % 6;
    return QRectF(PaletteChipsLeft + float(column) * (PaletteChipWidth + PaletteGap),
                  PaletteTop + 12.0f
                      + float(row) * (PaletteChipHeight + 7.0f),
                  PaletteChipWidth, PaletteChipHeight);
}

bool Editor::pointerHovering(const QRectF &rect) const
{
    return pointerActive_ && !rect.isEmpty()
           && rect.contains(QPointF(pointer_.x, pointer_.y));
}

void Editor::render(QPainter &painter, bool muted) const
{
    drawBackdrop(painter);
    drawHeader(painter, muted);
    drawTabs(painter);
    drawModeHint(painter);
    drawActions(painter);

    if (mode_ == Mode::Join) {
        drawJoinCard(painter);
    } else {
        drawWaveTable(painter);
        if (mode_ == Mode::Host) {
            drawHostPortRow(painter);
        }
        drawInspector(painter);
    }

    drawPalette(painter);

    const QString footer =
        mode_ == Mode::Join
            ? QStringLiteral("点击字段输入  ·  BACKSPACE 删除  ·  ENTER 连接  ·  ESC 返回")
            : QStringLiteral("F1 / F2 / F3 切换选项卡  ·  TAB 循环  ·  1–9 / 0 添加敌人  ·  ENTER 按当前选项卡开始  ·  ESC 返回");
    Base::text(painter, Base::tinyFont(), footer, QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 24.0f},
               QStringLiteral("center"), 170);
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
    const QColor tint = accent();
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
    Base::text(painter, Base::techTinyFont(),
               muted ? QStringLiteral("CUSTOM PROTOCOL  //  SCRIPT + SERVER  //  MUTED")
                     : QStringLiteral("CUSTOM PROTOCOL  //  SCRIPT + SERVER"),
               muted ? Base::Amber : QColor(121, 176, 194),
               {centerX, 20.0f}, QStringLiteral("center"), muted ? 215 : 190);

    Base::glowText(painter, Base::bigFont(), QStringLiteral("自定义协议"),
                   Base::White, {centerX, 56.0f}, QStringLiteral("center"), 250);

    const QColor tint = accent();
    Base::strokeLine(painter, {centerX - 170.0f, 86.0f}, {centerX - 34.0f, 86.0f},
                     Base::withAlpha(tint, 130), 1.0f);
    Base::strokeLine(painter, {centerX + 34.0f, 86.0f}, {centerX + 170.0f, 86.0f},
                     Base::withAlpha(tint, 130), 1.0f);
    Base::fillCircle(painter, {centerX - 176.0f, 86.0f}, 2.0f, tint);
    Base::fillCircle(painter, {centerX + 176.0f, 86.0f}, 2.0f, tint);
}

void Editor::drawTabs(QPainter &painter) const
{
    for (int tab = 0; tab < 3; ++tab) {
        const QRectF rect = tabRect(tab);
        const QColor tint = tabAccent(tab);
        const bool selected = int(mode_) == tab;
        const bool hovered = pointerHovering(rect);

        if (selected || hovered) {
            painter.save();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Base::withAlpha(tint, selected ? 55 : 30),
                                selected ? 3.0f : 2.0f));
            painter.drawRoundedRect(rect.adjusted(-1.0f, -1.0f, 1.0f, 1.0f), 11.0f, 11.0f);
            painter.restore();
        }

        QLinearGradient fill(0.0f, float(rect.top()), 0.0f, float(rect.bottom()));
        if (selected) {
            fill.setColorAt(0.0, QColor(12, 30, 46, 250));
            fill.setColorAt(1.0, QColor(6, 16, 28, 250));
        } else if (hovered) {
            fill.setColorAt(0.0, QColor(10, 23, 37, 242));
            fill.setColorAt(1.0, QColor(5, 13, 24, 242));
        } else {
            fill.setColorAt(0.0, QColor(7, 16, 28, 230));
            fill.setColorAt(1.0, QColor(4, 11, 21, 230));
        }
        painter.save();
        painter.setBrush(fill);
        painter.setPen(QPen(selected ? tint
                                    : (hovered ? Base::withAlpha(tint, 150)
                                               : QColor(42, 76, 96)),
                            selected ? 1.6f : 1.0f));
        painter.drawRoundedRect(rect, 10.0f, 10.0f);
        painter.restore();

        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(Base::withAlpha(tint, selected ? 210 : (hovered ? 95 : 38)));
        painter.drawRoundedRect(QRectF(rect.left() + 12.0f, rect.top() + 2.0f,
                                       rect.width() - 24.0f, 2.0f),
                                1.0f, 1.0f);
        painter.restore();

        const QRectF key(rect.left() + 14.0f, float(rect.center().y()) - 11.0f,
                         30.0f, 22.0f);
        painter.save();
        painter.setBrush(QColor(3, 10, 18, 235));
        painter.setPen(QPen(selected ? Base::withAlpha(tint, 220)
                                     : (hovered ? Base::withAlpha(tint, 120)
                                                : QColor(44, 78, 98)),
                            1.0f));
        painter.drawRoundedRect(key, 5.0f, 5.0f);
        painter.restore();
        Base::text(painter, Base::techTinyFont(), kTabKeys[tab],
                   selected ? Base::White : (hovered ? Base::Text : Base::Muted),
                   {float(key.center().x()), float(key.center().y())},
                   QStringLiteral("center"), 230);

        Base::text(painter, Base::smallFont(), kTabLabels[tab],
                   selected ? Base::White : (hovered ? Base::Ink : Base::Text),
                   {float(rect.left()) + 56.0f,
                    float(rect.center().y()) - 6.0f},
                   QStringLiteral("midleft"), 238);
        Base::text(painter, Base::techTinyFont(), kTabSubtitles[tab],
                   selected ? tint : (hovered ? Base::withAlpha(tint, 200) : Base::Muted),
                   {float(rect.left()) + 56.0f,
                    float(rect.center().y()) + 11.0f},
                   QStringLiteral("midleft"), selected ? 220 : 180);

        if (selected) {
            Base::fillCircle(painter,
                             {float(rect.right()) - 16.0f, float(rect.center().y())},
                             2.6f,
                             Base::withAlpha(tint, int(120.0f + 120.0f * pulse())));
        }
    }
}

void Editor::drawModeHint(QPainter &painter) const
{
    QString hint;
    switch (mode_) {
    case Mode::Single:
        hint = QStringLiteral("单机：编辑台本并直接开始，不开放端口");
        break;
    case Mode::Host:
        hint = QStringLiteral("局域网主机：编辑台本，服务器端口在波次下方设置");
        break;
    case Mode::Join:
        hint = QStringLiteral("加入主机：填写主机地址与端口，连接后由主机推进台本");
        break;
    }

    const QColor tint = accent();
    const QRectF pill(GameConfig::ScreenWidth / 2.0f - 380.0f, 144.0f, 760.0f, 26.0f);
    Base::panel(painter, pill, Base::shade(tint, 0.7f), 170, 13, QColor(5, 13, 24));
    Base::fillCircle(painter, {float(pill.left()) + 18.0f, float(pill.center().y())},
                     3.4f, Base::withAlpha(tint, int(120.0f + 100.0f * pulse())));
    Base::text(painter, Base::tinyFont(), hint, QColor(190, 211, 227),
               {float(pill.left()) + 30.0f, float(pill.center().y())},
               QStringLiteral("midleft"), 225);
    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("MODE %1 / 03").arg(int(mode_) + 1), tint,
               {float(pill.right()) - 16.0f, float(pill.center().y())},
               QStringLiteral("midright"), 200);
}

void Editor::drawActions(QPainter &painter) const
{
    const QColor tint = accent();
    if (mode_ == Mode::Join) {
        Base::text(painter, Base::techTinyFont(), QStringLiteral("JOIN SESSION // 加入"),
                   tint, {Left, ActionsTop + 11.0f}, QStringLiteral("midleft"), 215);
        Base::text(painter, Base::techSmallFont(),
                   startValid() ? QStringLiteral("连接 %1:%2").arg(hostText_).arg(portText_)
                                : QStringLiteral("填写主机地址与端口后才能连接"),
                   startValid() ? Base::Green : Base::Muted,
                   {Left, ActionsTop + 27.0f}, QStringLiteral("midleft"), 220);
    } else {
        const float baseY = ActionsTop + 29.0f;
        for (int i = 0; i < 4; ++i) {
            const float height = 6.0f + float(i) * 4.0f;
            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(Base::withAlpha(tint, 90 + i * 38));
            painter.drawRoundedRect(QRectF(Left + float(i) * 6.0f, baseY - height,
                                           3.5f, height),
                                    1.5f, 1.5f);
            painter.restore();
        }
        Base::text(painter, Base::techTinyFont(), QStringLiteral("WAVE SCRIPT // 台本"),
                   Base::Muted, {Left + 32.0f, ActionsTop + 11.0f},
                   QStringLiteral("midleft"), 195);
        Base::text(painter, Base::techSmallFont(),
                   QStringLiteral("%1 波  ·  %2 个目标  ·  上限 %3 波")
                       .arg(waveCount(), 2, 10, QChar('0'))
                       .arg(totalEnemies())
                       .arg(MaxWaves),
                   Base::Text, {Left + 32.0f, ActionsTop + 27.0f},
                   QStringLiteral("midleft"), 228);
    }

    if (mode_ != Mode::Join) {
        drawButton(painter, addWaveRect(), QStringLiteral("N"),
                   QStringLiteral("新增波次"), tint, false, waveCount() < MaxWaves);
        drawButton(painter, removeWaveRect(), QStringLiteral("X"),
                   QStringLiteral("删除末波"), tint, false, waveCount() > 1);
    }
    drawButton(painter, deployRect(), QStringLiteral("ENTER"),
               mode_ == Mode::Join ? QStringLiteral("连接") : QStringLiteral("部署"),
               tint, true, startValid());
    drawButton(painter, backRect(), QStringLiteral("ESC"), QStringLiteral("返回"),
               tint, false, true);
}

void Editor::drawButton(QPainter &painter, const QRectF &rect, const QString &key,
                        const QString &label, const QColor &tint, bool primary,
                        bool enabled) const
{
    const bool hovered = enabled && pointerHovering(rect);
    QColor border = primary ? tint : QColor(58, 96, 118);
    if (!enabled) {
        border = QColor(48, 62, 76);
    } else if (hovered && !primary) {
        border = Base::withAlpha(tint, 185);
    }

    painter.save();
    QLinearGradient fill(0.0f, float(rect.top()), 0.0f, float(rect.bottom()));
    if (!enabled) {
        fill.setColorAt(0.0, QColor(8, 14, 22, 220));
        fill.setColorAt(1.0, QColor(5, 10, 18, 220));
    } else if (primary) {
        fill.setColorAt(0.0, Base::withAlpha(tint, hovered ? 82 : 54));
        fill.setColorAt(1.0, QColor(6, 16, 28, 238));
    } else {
        fill.setColorAt(0.0, hovered ? QColor(12, 28, 42, 242)
                                     : QColor(8, 19, 31, 232));
        fill.setColorAt(1.0, QColor(5, 12, 22, 232));
    }
    painter.setBrush(fill);
    painter.setPen(QPen(border, primary && enabled ? 1.5f : 1.0f));
    painter.drawRoundedRect(rect, 8.0f, 8.0f);
    if (enabled && hovered) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Base::withAlpha(tint, 170));
        painter.drawRoundedRect(QRectF(rect.left() + 12.0f, rect.bottom() - 2.5f,
                                       rect.width() - 24.0f, 1.5f),
                                0.8f, 0.8f);
    }
    painter.restore();

    const QFontMetricsF metrics(Base::techTinyFont());
    const float keyWidth = std::max(22.0f, float(metrics.horizontalAdvance(key)) + 12.0f);
    const QRectF badge(rect.left() + 12.0f, float(rect.center().y()) - 10.0f,
                       keyWidth, 20.0f);
    painter.save();
    painter.setBrush(QColor(3, 10, 18, 235));
    painter.setPen(QPen(enabled ? Base::withAlpha(primary ? tint : QColor(96, 134, 154), 190)
                                : QColor(48, 62, 76),
                        1.0f));
    painter.drawRoundedRect(badge, 4.0f, 4.0f);
    painter.restore();
    Base::text(painter, Base::techTinyFont(), key,
               enabled ? (primary ? tint : Base::Text) : Base::Dim,
               {float(badge.center().x()), float(badge.center().y())},
               QStringLiteral("center"), 235);
    Base::text(painter, Base::techSmallFont(), label,
               enabled ? (primary ? Base::White : Base::Text) : Base::Dim,
               {float(badge.right()) + 9.0f, float(rect.center().y())},
               QStringLiteral("midleft"), 232);
}

void Editor::drawWaveTable(QPainter &painter) const
{
    const std::vector<int> kinds = visibleKinds();
    const QColor tint = accent();

    painter.fillRect(QRectF(Left, 220.0f, Right - Left, 30.0f),
                     QColor(7, 17, 29, 180));
    Base::strokeLine(painter, {Left, 220.0f}, {Right, 220.0f},
                     Base::withAlpha(tint, 95), 1.0f);
    Base::strokeLine(painter, {Left, 250.0f}, {Right, 250.0f},
                     QColor(40, 76, 96), 1.0f);
    Base::text(painter, Base::techTinyFont(), QStringLiteral("WAVE"), Base::Muted,
               {Left + 16.0f, HeaderY}, QStringLiteral("midleft"), 210);
    Base::text(painter, Base::techTinyFont(), QStringLiteral("波次"), Base::Dim,
               {Left + 58.0f, HeaderY}, QStringLiteral("midleft"), 170);

    for (int column = 0; column < int(kinds.size()); ++column) {
        const int kind = kinds[size_t(column)];
        const QRectF cell = cellRect(0, column);
        const QColor color = kindColor(kind);
        Base::text(painter, Base::techTinyFont(), kindLabel(kind), color,
                   {float(cell.center().x()), HeaderY - 2.0f},
                   QStringLiteral("center"), 220);
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(Base::withAlpha(color, 170));
        painter.drawRoundedRect(QRectF(float(cell.center().x()) - 12.0f, HeaderY + 9.0f,
                                       24.0f, 1.5f),
                                0.8f, 0.8f);
        painter.restore();
    }

    if (!kinds.empty()) {
        const QRectF lastCell = cellRect(0, int(kinds.size()) - 1);
        if (Right - float(lastCell.right()) > 320.0f) {
            Base::text(painter, Base::techTinyFont(),
                       QStringLiteral("单击单元格选中 · 从敌人库拖到波次加入"),
                       QColor(105, 138, 158), {Right - 16.0f, HeaderY},
                       QStringLiteral("midright"), 160);
        }
    }

    if (kinds.empty()) {
        painter.fillRect(QRectF(Left, RowsTop, Right - Left, 66.0f),
                         QColor(6, 14, 25, 190));
        Base::text(painter, Base::bodyFont(),
                   QStringLiteral("还没有敌人列：把下方敌人库拖到某一波，或按 1–9 / 0 添加"),
                   Base::Muted, {Left + WaveColumn + 12.0f, RowsTop + 34.0f},
                   QStringLiteral("midleft"), 215);
        Base::strokeLine(painter, {Left + WaveColumn, RowsTop},
                         {Left + WaveColumn, RowsTop + 66.0f},
                         Base::withAlpha(tint, 80), 1.0f);
        return;
    }

    for (int row = 0; row < waveCount(); ++row) {
        const QRectF rect = rowRect(row);
        const bool activeRow = row == row_;
        const bool hoveredRow = pointerHovering(rect) && dragKind_ < 0;
        QColor fill = activeRow ? QColor(9, 24, 38, 235)
                                : (row % 2 == 0 ? QColor(5, 13, 23, 205)
                                                : QColor(7, 16, 28, 205));
        if (hoveredRow && !activeRow) {
            fill = QColor(11, 24, 38, 228);
        }
        painter.fillRect(rect, fill);
        if (activeRow) {
            painter.fillRect(QRectF(Left, float(rect.top()) + 1.0f, 3.0f,
                                    float(rect.height()) - 2.0f),
                             tint);
        }
        Base::strokeLine(painter, {Left + 10.0f, float(rect.bottom())},
                         {Right - 10.0f, float(rect.bottom())},
                         QColor(30, 55, 74, 110), 1.0f);

        Base::text(painter, Base::techSmallFont(),
                   QStringLiteral("%1").arg(row + 1, 2, 10, QChar('0')),
                   activeRow ? tint : Base::Muted,
                   {Left + 18.0f, float(rect.center().y())},
                   QStringLiteral("midleft"), activeRow ? 240 : 210);
        Base::text(painter, Base::techTinyFont(),
                   QStringLiteral("×%1").arg(waveTotal(row)),
                   activeRow ? tint : Base::Dim,
                   {Left + WaveColumn - 12.0f, float(rect.center().y())},
                   QStringLiteral("midright"), activeRow ? 215 : 165);

        for (int column = 0; column < int(kinds.size()); ++column) {
            const int kind = kinds[size_t(column)];
            const QRectF cell = cellRect(row, column);
            const bool activeCell = activeRow && column == column_;
            const bool hoverCell = pointerHovering(cell) && dragKind_ < 0;
            const QColor color = kindColor(kind);

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

            if (kind == 4) {
                const bool boss = waveHasBoss(row);
                Base::text(painter, Base::techSmallFont(),
                           boss ? QStringLiteral("BOSS") : QStringLiteral("—"),
                           boss ? Base::Magenta : Base::Dim,
                           {float(cell.center().x()), float(cell.center().y())},
                           QStringLiteral("center"), boss ? 240 : 180);
                continue;
            }

            const int count = enemyCountKind(row, kind);
            if (activeCell) {
                const auto step = [&](const QRectF &button, const QString &glyph) {
                    const bool hot = pointerHovering(button);
                    painter.save();
                    painter.setBrush(hot ? Base::withAlpha(color, 55)
                                         : QColor(3, 11, 19, 230));
                    painter.setPen(QPen(hot ? color : Base::withAlpha(color, 120),
                                        1.0f));
                    painter.drawRoundedRect(button, 4.0f, 4.0f);
                    painter.restore();
                    Base::text(painter, Base::techSmallFont(), glyph,
                               hot ? Base::White : color,
                               {float(button.center().x()), float(button.center().y())},
                               QStringLiteral("center"), hot ? 245 : 220);
                };
                step(minusRect(), QStringLiteral("-"));
                step(plusRect(), QStringLiteral("+"));
                Base::text(painter, Base::techSmallFont(), QString::number(count),
                           Base::White,
                           {float(cell.center().x()), float(cell.center().y())},
                           QStringLiteral("center"), 245);
            } else {
                Base::text(painter, Base::techSmallFont(), QString::number(count),
                           count > 0 ? Base::Text : Base::Dim,
                           {float(cell.center().x()), float(cell.center().y())},
                           QStringLiteral("center"), count > 0 ? 215 : 175);
            }
        }
    }

    if (dragKind_ >= 0) {
        for (int row = 0; row < waveCount(); ++row) {
            if (!rowRect(row).contains(QPointF(dragPos_.x, dragPos_.y))) {
                continue;
            }
            const QColor color = kindColor(dragKind_);
            const QRectF target = rowRect(row);
            painter.save();
            painter.setBrush(Base::withAlpha(color, 30));
            painter.setPen(QPen(Base::withAlpha(color, 235), 2.0f));
            painter.drawRoundedRect(target.adjusted(1.0f, 1.0f, -1.0f, -1.0f),
                                    5.0f, 5.0f);
            painter.restore();
            Base::text(painter, Base::techTinyFont(),
                       QStringLiteral("释放加入 第 %1 波").arg(row + 1), color,
                       {float(target.right()) - 18.0f, float(target.center().y())},
                       QStringLiteral("midright"), 230);
            break;
        }

        const QRectF ghost(dragPos_.x - PaletteChipWidth * 0.5f,
                           dragPos_.y - PaletteChipHeight * 0.5f,
                           PaletteChipWidth, PaletteChipHeight);
        const QColor color = kindColor(dragKind_);
        Base::panel(painter, ghost, color, 210, 8, Base::withAlpha(color, 70));
        Base::text(painter, Base::techSmallFont(), kindLabel(dragKind_), Base::White,
                   {float(ghost.center().x()), float(ghost.center().y())},
                   QStringLiteral("center"), 245);
    }
}

void Editor::drawHostPortRow(QPainter &painter) const
{
    const QRectF row = portRowRect();
    const QColor tint = Base::Violet;
    const bool hoveredRow = pointerHovering(row);
    const bool valid = portValid();

    QLinearGradient fill(0.0f, float(row.top()), 0.0f, float(row.bottom()));
    fill.setColorAt(0.0, hoveredRow ? QColor(14, 20, 40, 245)
                                    : QColor(9, 16, 32, 240));
    fill.setColorAt(1.0, QColor(5, 11, 22, 240));
    painter.save();
    painter.setBrush(fill);
    painter.setPen(QPen(Base::withAlpha(tint, hoveredRow ? 165 : 110), 1.2f));
    painter.drawRoundedRect(row, 9.0f, 9.0f);
    painter.setPen(Qt::NoPen);
    painter.setBrush(tint);
    painter.drawRoundedRect(QRectF(float(row.left()) + 1.0f, float(row.top()) + 10.0f,
                                   2.5f, float(row.height()) - 20.0f),
                            1.2f, 1.2f);
    painter.restore();

    Base::text(painter, Base::techSmallFont(), QStringLiteral("服务器端口"),
               Base::White,
               {float(row.left()) + 20.0f, float(row.center().y()) - 8.0f},
               QStringLiteral("midleft"), 235);
    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("SERVER PORT // TCP · LISTEN 0.0.0.0"), Base::Muted,
               {float(row.left()) + 20.0f, float(row.center().y()) + 11.0f},
               QStringLiteral("midleft"), 190);
    Base::strokeLine(painter, {float(row.left()) + 340.0f, float(row.top()) + 14.0f},
                     {float(row.left()) + 340.0f, float(row.bottom()) - 14.0f},
                     Base::withAlpha(tint, 70), 1.0f);

    drawTextField(painter, portRect(), portText_, QStringLiteral("端口"),
                  focus_ == Focus::Port, valid, tint);

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("同一局域网客户端填本机地址 + 此端口"), Base::Muted,
               {float(portRect().right()) + 24.0f, float(row.center().y())},
               QStringLiteral("midleft"), 200);

    const QRectF pill(Right - 166.0f, float(row.center().y()) - 13.0f, 142.0f, 26.0f);
    painter.save();
    painter.setBrush(QColor(3, 10, 18, 230));
    painter.setPen(QPen(Base::withAlpha(valid ? tint : Base::Red, 150), 1.0f));
    painter.drawRoundedRect(pill, 13.0f, 13.0f);
    painter.restore();
    Base::fillCircle(painter, {float(pill.left()) + 14.0f, float(pill.center().y())},
                     2.6f, valid ? Base::Green : Base::Red);
    Base::text(painter, Base::techTinyFont(),
               valid ? QStringLiteral("TCP 0.0.0.0:%1").arg(portText_)
                     : QStringLiteral("端口 1–65535"),
               valid ? Base::Text : Base::Red,
               {float(pill.left()) + 24.0f, float(pill.center().y())},
               QStringLiteral("midleft"), 220);
}

void Editor::drawInspector(QPainter &painter) const
{
    const QRectF rect = inspectorRect();
    if (rect.isEmpty()) {
        return;
    }
    const QColor tint = accent();
    const float top = float(rect.top());
    const float x1 = float(rect.left()) + float(rect.width()) * 0.355f;
    const float x2 = float(rect.left()) + float(rect.width()) * 0.69f;

    Base::panel(painter, rect, Base::shade(tint, 0.55f), 160, 10, QColor(5, 13, 23));
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(Base::withAlpha(tint, 120));
    painter.drawRoundedRect(QRectF(float(rect.left()) + 18.0f, float(rect.top()) + 1.0f,
                                   float(rect.width()) - 36.0f, 2.0f),
                            1.0f, 1.0f);
    painter.restore();
    Base::strokeLine(painter, {x1, top + 16.0f}, {x1, float(rect.bottom()) - 16.0f},
                     Base::withAlpha(tint, 55), 1.0f);
    Base::strokeLine(painter, {x2, top + 16.0f}, {x2, float(rect.bottom()) - 16.0f},
                     Base::withAlpha(tint, 55), 1.0f);

    // 第一列：当前选中波次
    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("SELECTED // 当前波次"), tint,
               {Left + 24.0f, top + 18.0f}, QStringLiteral("midleft"), 220);
    Base::text(painter, Base::largeFont(),
               QStringLiteral("%1").arg(row_ + 1, 2, 10, QChar('0')), Base::White,
               {Left + 24.0f, top + 47.0f}, QStringLiteral("midleft"), 245);
    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("WAVE %1 / %2").arg(row_ + 1).arg(waveCount()),
               Base::Muted, {Left + 92.0f, top + 41.0f}, QStringLiteral("midleft"), 195);
    Base::text(painter, Base::techSmallFont(),
               QStringLiteral("%1 个目标").arg(waveTotal(row_)), Base::Text,
               {Left + 92.0f, top + 61.0f}, QStringLiteral("midleft"), 225);

    if (rect.height() >= 132.0f) {
        const QFontMetricsF metrics(Base::techTinyFont());
        float x = Left + 24.0f;
        const float y = top + 91.0f;
        bool any = false;
        const std::vector<int> kinds = visibleKinds();
        for (int kind : kinds) {
            const int count = enemyCountKind(row_, kind);
            if (count <= 0) {
                continue;
            }
            const QString part = QStringLiteral("%1 ×%2").arg(kindLabel(kind)).arg(count);
            const float width = float(metrics.horizontalAdvance(part));
            if (x + width > x1 - 28.0f) {
                Base::text(painter, Base::techTinyFont(), QStringLiteral("…"),
                           Base::Dim, {x, y}, QStringLiteral("midleft"), 190);
                break;
            }
            Base::text(painter, Base::techTinyFont(), part, kindColor(kind),
                       {x, y}, QStringLiteral("midleft"), 225);
            x += width + 22.0f;
            any = true;
        }
        if (waveHasBoss(row_)) {
            Base::text(painter, Base::techTinyFont(), QStringLiteral("BOSS ×1"),
                       Base::Magenta, {x, y}, QStringLiteral("midleft"), 230);
            any = true;
        }
        if (!any) {
            Base::text(painter, Base::tinyFont(),
                       QStringLiteral("空波次 · 拖动敌人到这一波"), Base::Muted,
                       {Left + 24.0f, y}, QStringLiteral("midleft"), 190);
        }
    }

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("↑↓ 选择波次  ·  ←→ 选择列  ·  ENTER 部署"),
               Base::Muted, {Left + 24.0f, float(rect.bottom()) - 17.0f},
               QStringLiteral("midleft"), 180);

    // 第二列：台本概览
    const float cx = x1 + 22.0f;
    Base::text(painter, Base::techTinyFont(), QStringLiteral("SCRIPT // 台本概览"),
               tint, {cx, top + 18.0f}, QStringLiteral("midleft"), 220);

    int bossRow = -1;
    for (int wave = 0; wave < waveCount(); ++wave) {
        if (waveHasBoss(wave)) {
            bossRow = wave;
            break;
        }
    }
    const struct {
        QString label;
        QString value;
        QColor color;
    } rows[] = {
        {QStringLiteral("波次"), QStringLiteral("%1 / %2")
                                      .arg(waveCount(), 2, 10, QChar('0'))
                                      .arg(MaxWaves),
         Base::Text},
        {QStringLiteral("目标总数"), QString::number(totalEnemies()), Base::Text},
        {QStringLiteral("最终 BOSS"),
         bossRow >= 0 ? QStringLiteral("第 %1 波").arg(bossRow + 1)
                      : QStringLiteral("未配置"),
         bossRow >= 0 ? Base::Magenta : Base::Muted},
    };
    for (int i = 0; i < 3; ++i) {
        const float y = top + 45.0f + float(i) * 23.0f;
        Base::text(painter, Base::techTinyFont(), rows[i].label, Base::Muted,
                   {cx, y}, QStringLiteral("midleft"), 195);
        Base::text(painter, Base::techSmallFont(), rows[i].value, rows[i].color,
                   {cx + 82.0f, y}, QStringLiteral("midleft"), 225);
    }

    // 第三列：键鼠提示
    const float rx = x2 + 22.0f;
    Base::text(painter, Base::techTinyFont(), QStringLiteral("CONTROLS // 操作"),
               tint, {rx, top + 18.0f}, QStringLiteral("midleft"), 220);
    const QString lines[] = {
        QStringLiteral("↑↓ / W S 选择波次    ←→ / A D 选择列"),
        QStringLiteral("按住 + / − 连续调整  ·  N / X 增删波次"),
        QStringLiteral("拖入敌人库或按 1–9 / 0 直接加入"),
    };
    for (int i = 0; i < 3; ++i) {
        Base::text(painter, Base::tinyFont(), lines[i], QColor(158, 184, 202),
                   {rx, top + 45.0f + float(i) * 23.0f},
                   QStringLiteral("midleft"), 190);
    }
}

void Editor::drawPalette(QPainter &painter) const
{
    const QRectF panel = paletteRect();
    const QColor tint = accent();
    Base::panel(painter, panel, QColor(48, 91, 112), 190, 10, QColor(6, 14, 25));
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(Base::withAlpha(tint, 100));
    painter.drawRoundedRect(QRectF(float(panel.left()) + 16.0f,
                                   float(panel.top()) + 1.0f,
                                   float(panel.width()) - 32.0f, 2.0f),
                            1.0f, 1.0f);
    painter.restore();

    Base::text(painter, Base::smallFont(), QStringLiteral("敌人库"), Base::White,
               {Left + 20.0f, PaletteTop + 28.0f}, QStringLiteral("midleft"), 235);
    Base::text(painter, Base::techTinyFont(), QStringLiteral("ENEMY PALETTE"), tint,
               {Left + 20.0f, PaletteTop + 50.0f}, QStringLiteral("midleft"), 215);
    Base::text(painter, Base::tinyFont(),
               QStringLiteral("拖到波次 / 按 1–9、0"), Base::Muted,
               {Left + 20.0f, PaletteTop + 72.0f}, QStringLiteral("midleft"), 190);
    Base::strokeLine(painter, {Left + 206.0f, PaletteTop + 16.0f},
                     {Left + 206.0f, PaletteTop + PaletteHeight - 16.0f},
                     QColor(40, 72, 92), 1.0f);

    for (int kind = 0; kind < Kinds; ++kind) {
        const QRectF chip = paletteChipRect(kind);
        const QColor color = kindColor(kind);
        const bool dragging = dragKind_ == kind;
        const bool hovered = pointerHovering(chip);

        if (dragging || hovered) {
            painter.save();
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Base::withAlpha(dragging ? Base::Cyan : color,
                                               dragging ? 90 : 55),
                                dragging ? 3.0f : 2.0f));
            painter.drawRoundedRect(chip.adjusted(-1.0f, -1.0f, 1.0f, 1.0f),
                                    8.0f, 8.0f);
            painter.restore();
        }

        QLinearGradient fill(0.0f, float(chip.top()), 0.0f, float(chip.bottom()));
        fill.setColorAt(0.0, dragging ? QColor(14, 32, 46, 245)
                                      : (hovered ? QColor(12, 28, 42, 242)
                                                 : QColor(8, 20, 32, 236)));
        fill.setColorAt(1.0, dragging ? QColor(8, 20, 32, 245)
                                      : QColor(5, 13, 23, 236));
        painter.save();
        painter.setBrush(fill);
        painter.setPen(QPen(dragging ? Base::Cyan
                                    : (hovered ? color : Base::withAlpha(color, 150)),
                            dragging ? 1.6f : 1.0f));
        painter.drawRoundedRect(chip, 7.0f, 7.0f);
        painter.restore();

        const QRectF key(chip.left() + 7.0f, float(chip.center().y()) - 9.0f,
                         18.0f, 18.0f);
        const QString badge = kind < 9 ? QString::number(kind + 1)
                                       : (kind == 9 ? QStringLiteral("0")
                                                    : QStringLiteral("—"));
        painter.save();
        painter.setBrush(QColor(3, 10, 18, 235));
        painter.setPen(QPen(dragging || hovered ? Base::withAlpha(color, 180)
                                                : QColor(46, 78, 96),
                            1.0f));
        painter.drawRoundedRect(key, 4.0f, 4.0f);
        painter.restore();
        Base::text(painter, Base::techTinyFont(), badge,
                   dragging || hovered ? Base::White : Base::Muted,
                   {float(key.center().x()), float(key.center().y())},
                   QStringLiteral("center"), 220);

        Base::fillCircle(painter, {float(chip.left()) + 37.0f,
                                   float(chip.center().y())},
                         3.2f, color);
        if (kind == 4) {
            Base::strokeCircle(painter, {float(chip.left()) + 37.0f,
                                         float(chip.center().y())},
                               5.6f, Base::withAlpha(color, 120), 1.0f);
        }
        Base::text(painter, Base::techTinyFont(), kindLabel(kind),
                   dragging || hovered ? Base::White : Base::Text,
                   {float(chip.left()) + 46.0f, float(chip.center().y())},
                   QStringLiteral("midleft"), 230);
        for (int dot = 0; dot < 3; ++dot) {
            Base::fillCircle(painter,
                             {float(chip.right()) - 11.0f,
                              float(chip.center().y()) - 4.0f + float(dot) * 4.0f},
                             1.0f,
                             Base::withAlpha(color, dragging || hovered ? 190 : 90));
        }
    }
}

void Editor::drawJoinCard(QPainter &painter) const
{
    const QColor tint = Base::Amber;
    const QRectF card(280.0f, 236.0f, 720.0f, 300.0f);

    QLinearGradient fill(0.0f, float(card.top()), 0.0f, float(card.bottom()));
    fill.setColorAt(0.0, QColor(12, 26, 40, 245));
    fill.setColorAt(1.0, QColor(5, 12, 22, 245));
    painter.save();
    painter.setBrush(fill);
    painter.setPen(QPen(Base::withAlpha(tint, 200), 1.4f));
    painter.drawRoundedRect(card, 12.0f, 12.0f);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Base::withAlpha(tint, 190));
    painter.drawRoundedRect(QRectF(float(card.left()) + 18.0f, float(card.top()) + 1.0f,
                                   float(card.width()) - 36.0f, 2.0f),
                            1.0f, 1.0f);
    painter.restore();

    // 右上角的“主机 ← 客户端”示意
    const Math::Vec2 server(936.0f, 284.0f);
    const Math::Vec2 client(972.0f, 309.0f);
    Base::strokeLine(painter, server, client, Base::withAlpha(tint, 125), 1.2f);
    Base::strokeCircle(painter, server, 15.0f, Base::withAlpha(tint, 115), 1.2f);
    Base::strokeCircle(painter, client, 10.0f, Base::withAlpha(tint, 115), 1.2f);
    Base::strokeCircle(painter, client, 13.0f + 4.0f * pulse(),
                       Base::withAlpha(tint, int(70.0f * (1.0f - pulse()))), 1.0f);
    Base::fillCircle(painter, server, 3.0f, tint);
    Base::fillCircle(painter, client, 2.4f, Base::Cyan);

    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("JOIN HOST // 连接远端"), tint,
               {314.0f, 260.0f}, QStringLiteral("midleft"), 220);
    Base::text(painter, Base::mediumFont(), QStringLiteral("加入已有主机"), Base::White,
               {314.0f, 286.0f}, QStringLiteral("midleft"), 240);
    Base::text(painter, Base::tinyFont(),
               QStringLiteral("连接后由主机推进台本；本机只发送输入并渲染快照"),
               Base::Muted, {314.0f, 310.0f}, QStringLiteral("midleft"), 210);

    Base::text(painter, Base::techTinyFont(),
               QStringLiteral("HOST ADDRESS  主机地址"), Base::Muted,
               {314.0f, 323.0f}, QStringLiteral("midleft"), 190);
    Base::text(painter, Base::techTinyFont(), QStringLiteral("PORT  端口"),
               Base::Muted, {774.0f, 323.0f}, QStringLiteral("midleft"), 190);

    drawTextField(painter, hostRect(), hostText_, QStringLiteral("输入主机地址"),
                  focus_ == Focus::Host, hostValid(), tint);
    drawTextField(painter, portRect(), portText_, QStringLiteral("端口"),
                  focus_ == Focus::Port, portValid(), tint);

    const bool valid = startValid();
    const float statusY = 401.0f;
    Base::fillCircle(painter, {320.0f, statusY}, 3.0f,
                     valid ? Base::Green : Base::Amber);
    Base::text(painter, Base::techSmallFont(),
               valid ? QStringLiteral("准备连接  %1 : %2").arg(hostText_).arg(portText_)
                     : QStringLiteral("地址或端口无效：请填写主机地址与 1–65535 端口"),
               valid ? Base::Green : Base::Amber,
               {330.0f, statusY}, QStringLiteral("midleft"), 225);
    Base::text(painter, Base::techTinyFont(),
               valid ? QStringLiteral("ENTER 连接") : QStringLiteral("等待输入"),
               Base::Muted, {966.0f, statusY}, QStringLiteral("midright"), 190);

    Base::strokeLine(painter, {314.0f, 430.0f}, {966.0f, 430.0f},
                     Base::withAlpha(tint, 55), 1.0f);
    Base::text(painter, Base::tinyFont(),
               QStringLiteral("同一局域网内填写主机 IP；端口范围 1–65535"),
               Base::Muted, {314.0f, 452.0f}, QStringLiteral("midleft"), 200);
    Base::text(painter, Base::tinyFont(),
               QStringLiteral("BACKSPACE 删除字符  ·  点击字段切换输入  ·  TAB / F1–F3 切换选项卡"),
               QColor(105, 138, 158), {314.0f, 476.0f},
               QStringLiteral("midleft"), 180);
}

void Editor::drawTextField(QPainter &painter, const QRectF &rect,
                           const QString &text, const QString &placeholder,
                           bool focused, bool valid, const QColor &tint) const
{
    const bool hovered = pointerHovering(rect);
    QColor border = QColor(52, 88, 110);
    if (!valid) {
        border = Base::Red;
    } else if (focused) {
        border = tint;
    } else if (hovered) {
        border = Base::withAlpha(tint, 150);
    }

    painter.save();
    painter.setBrush(QColor(3, 10, 18, 245));
    painter.setPen(QPen(border, focused ? 1.6f : 1.1f));
    painter.drawRoundedRect(rect, 8.0f, 8.0f);
    if (focused) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Base::withAlpha(tint, 220));
        painter.drawRoundedRect(QRectF(float(rect.left()) + 1.0f,
                                       float(rect.top()) + 9.0f, 2.2f,
                                       float(rect.height()) - 18.0f),
                                1.1f, 1.1f);
    }
    painter.restore();

    const bool empty = text.isEmpty();
    painter.save();
    painter.setClipRect(rect.adjusted(10.0f, 0.0f, -12.0f, 0.0f));
    if (empty) {
        if (!focused) {
            Base::text(painter, Base::smallFont(), placeholder, Base::Dim,
                       {float(rect.left()) + 14.0f, float(rect.center().y())},
                       QStringLiteral("midleft"), 200);
        }
    } else {
        Base::text(painter, Base::techFont(), text, Base::White,
                   {float(rect.left()) + 14.0f, float(rect.center().y())},
                   QStringLiteral("midleft"), 240);
    }
    painter.restore();

    if (focused) {
        const float advance = empty
                                  ? 0.0f
                                  : float(QFontMetricsF(Base::techFont())
                                              .horizontalAdvance(text));
        const float caretX = float(rect.left()) + 14.0f + advance + 2.0f;
        Base::strokeLine(painter, {caretX, float(rect.top()) + 12.0f},
                         {caretX, float(rect.bottom()) - 12.0f},
                         Base::withAlpha(tint, int(150.0f + 90.0f * pulse())), 1.6f);
    }
}

} // namespace menu
