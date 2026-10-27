#include "Menu/Editor/Editor.h"

#include "Arena/Combatants/Enemies/DroneEnemy.h"
#include "Arena/Combatants/Enemy.h"
#include "Base/Base.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>
#include <array>

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
    dragKind_ = -1;
}

void Editor::prepare()
{
    row_ = std::clamp(row_, 0, std::max(0, waveCount() - 1));
    clampColumn();
    hold_ = 0;
    holdTimer_ = 0.0f;
    dragKind_ = -1;
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
    const Config cfg = config();
    if (cfg.mode == Mode::Single) {
        return true;
    }
    if (cfg.mode == Mode::Join) {
        return !cfg.host.isEmpty() && cfg.port != 0;
    }
    return cfg.port != 0;
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
    return QRectF(cell.left() + 6.0f, cell.top() + 5.0f, 26.0f, cell.height() - 10.0f);
}

QRectF Editor::plusRect() const
{
    if (columnCount() == 0) {
        return QRectF();
    }
    const QRectF cell = cellRect(row_, column_);
    return QRectF(cell.right() - 32.0f, cell.top() + 5.0f, 26.0f, cell.height() - 10.0f);
}

QRectF Editor::addWaveRect() const
{
    return QRectF(Right - 634.0f, 160.0f, 150.0f, 34.0f);
}

QRectF Editor::removeWaveRect() const
{
    return QRectF(Right - 472.0f, 160.0f, 150.0f, 34.0f);
}

QRectF Editor::deployRect() const
{
    return QRectF(Right - 310.0f, 160.0f, 150.0f, 34.0f);
}

QRectF Editor::backRect() const
{
    return QRectF(Right - 148.0f, 160.0f, 148.0f, 34.0f);
}

QRectF Editor::tabRect(int index) const
{
    constexpr float width = 220.0f;
    constexpr float gap = 18.0f;
    const float total = width * 3.0f + gap * 2.0f;
    const float left = (GameConfig::ScreenWidth - total) / 2.0f;
    return QRectF(left + float(index) * (width + gap), 82.0f, width, 42.0f);
}

QRectF Editor::portRect() const
{
    if (mode_ == Mode::Join) {
        return QRectF(440.0f, 392.0f, 400.0f, 44.0f);
    }
    const float top = std::min(522.0f, RowsTop + float(waveCount()) * RowHeight + 12.0f);
    return QRectF(440.0f, top, 400.0f, 44.0f);
}

QRectF Editor::hostRect() const
{
    return QRectF(440.0f, 322.0f, 400.0f, 44.0f);
}

QRectF Editor::paletteRect() const
{
    return QRectF(Left, PaletteTop, Right - Left, 94.0f);
}

QRectF Editor::paletteChipRect(int kind) const
{
    const int row = kind / 6;
    const int column = kind % 6;
    return QRectF(PaletteChipsLeft + float(column) * (PaletteChipWidth + PaletteGap),
                  PaletteTop + 10.0f
                      + float(row) * (PaletteChipHeight + 6.0f),
                  PaletteChipWidth, PaletteChipHeight);
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
               muted ? QStringLiteral("CUSTOM PROTOCOL  //  SCRIPT + SERVER  //  MUTED")
                     : QStringLiteral("CUSTOM PROTOCOL  //  SCRIPT + SERVER"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 42}, QStringLiteral("center"), 190);
    for (int tab = 0; tab < 3; ++tab) {
        const QRectF rect = tabRect(tab);
        const bool selected = int(mode_) == tab;
        const QColor accent = tab == 0 ? Base::Cyan
                                      : (tab == 1 ? Base::Green : Base::Amber);
        Base::panel(painter, rect, selected ? accent : QColor(48, 91, 112),
                    selected ? 235 : 180, 9,
                    selected ? QColor(9, 24, 38) : QColor(6, 14, 25));
        Base::text(painter, Base::smallFont(),
                   tab == 0 ? QStringLiteral("单机")
                            : (tab == 1 ? QStringLiteral("局域网主机")
                                        : QStringLiteral("加入主机")),
                   selected ? Base::White : Base::Text,
                   {float(rect.center().x()), float(rect.center().y())},
                   QStringLiteral("center"), 220);
    }
    Base::text(painter, Base::smallFont(),
               mode_ == Mode::Single
                   ? QStringLiteral("单机：编辑台本，不开放端口")
                   : (mode_ == Mode::Host
                          ? QStringLiteral("局域网主机：编辑台本，端口追加在波次下方")
                          : QStringLiteral("加入主机：填写主机地址和端口")),
               Base::Muted, {GameConfig::ScreenWidth / 2.0f, 140},
               QStringLiteral("center"), 220);

    Base::text(painter, Base::techSmallFont(),
               mode_ == Mode::Join
                   ? QStringLiteral("JOIN")
                   : QStringLiteral("%1 波 · %2 个目标")
                         .arg(waveCount(), 2, 10, QChar('0'))
                         .arg(totalEnemies()),
               Base::Muted, {Left, 174}, QStringLiteral("midleft"), 210);

    if (mode_ == Mode::Join) {
        Base::panel(painter, QRectF(240, 220, 800, 270), QColor(43, 83, 103),
                    180, 12, QColor(5, 13, 23));
        Base::text(painter, Base::mediumFont(), QStringLiteral("加入已有主机"),
                   Base::White, {262, 252}, QStringLiteral("midleft"), 230);
        Base::text(painter, Base::tinyFont(),
                   QStringLiteral("连接后由主机推进台本；本机只发送输入和渲染快照"),
                   Base::Muted, {262, 278}, QStringLiteral("midleft"), 200);

        Base::text(painter, Base::techTinyFont(), QStringLiteral("HOST ADDRESS"),
                   Base::Muted, {440, 306}, QStringLiteral("midleft"), 170);
        Base::panel(painter, hostRect(),
                    focus_ == Focus::Host ? Base::Amber : QColor(60, 104, 126),
                    focus_ == Focus::Host ? 240 : 190, 8, QColor(3, 10, 18));
        Base::text(painter, Base::mediumFont(),
                   hostText_.isEmpty() ? QStringLiteral("输入主机地址") : hostText_,
                   hostText_.isEmpty() ? Base::Muted : Base::White,
                   {456, 344}, QStringLiteral("midleft"), 220);

        Base::text(painter, Base::techTinyFont(), QStringLiteral("PORT"),
                   Base::Muted, {440, 378}, QStringLiteral("midleft"), 170);
        Base::panel(painter, portRect(),
                    focus_ == Focus::Port ? Base::Cyan : QColor(60, 104, 126),
                    focus_ == Focus::Port ? 240 : 190, 8, QColor(3, 10, 18));
        Base::text(painter, Base::mediumFont(), portText_,
                   portText_.isEmpty() ? Base::Muted : Base::White,
                   {456, 414}, QStringLiteral("midleft"), 220);
        Base::text(painter, Base::tinyFont(),
                   startValid() ? QStringLiteral("ENTER 连接主机")
                                : QStringLiteral("地址或端口无效"),
                   startValid() ? Base::Green : Base::Muted,
                   {262, 462}, QStringLiteral("midleft"), 200);

        Base::panel(painter, deployRect(), startValid() ? Base::Green
                                                        : QColor(60, 82, 94),
                    210, 8, QColor(6, 16, 26));
        Base::panel(painter, backRect(), QColor(65, 104, 124), 210, 8,
                    QColor(7, 14, 25));
        Base::text(painter, Base::techSmallFont(), QStringLiteral("ENTER  连接"),
                   startValid() ? Base::Text : Base::Muted,
                   {float(deployRect().center().x()), float(deployRect().center().y())},
                   QStringLiteral("center"));
        Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回"), Base::Text,
                   {float(backRect().center().x()), float(backRect().center().y())},
                   QStringLiteral("center"));
        Base::text(painter, Base::tinyFont(),
                   QStringLiteral("F1 单机  F2 局域网主机  F3 加入主机  点击字段编辑"),
                   QColor(105, 138, 158),
                   {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 26.0f},
                   QStringLiteral("center"), 170);
        return;
    }

    const std::vector<int> kinds = visibleKinds();

    Base::text(painter, Base::techTinyFont(), QStringLiteral("WAVE"), Base::Muted,
               {Left + 16.0f, HeaderY}, QStringLiteral("midleft"), 200);
    for (int column = 0; column < int(kinds.size()); ++column) {
        const int kind = kinds[size_t(column)];
        const QRectF cell = cellRect(0, column);
        Base::text(painter, Base::techTinyFont(), kindLabel(kind), kindColor(kind),
                   {float(cell.center().x()), HeaderY + 6.0f},
                   QStringLiteral("center"), 200);
    }
    Base::strokeLine(painter, {Left, HeaderY + 14.0f},
                     {Right, HeaderY + 14.0f}, QColor(44, 78, 96), 1.0f);

    if (kinds.empty()) {
        Base::text(painter, Base::bodyFont(),
                   QStringLiteral("还没有敌人：从下方敌人库拖到波次上，或按 1–5 添加"),
                   Base::Muted,
                   {float(Left + WaveColumn + 20.0f), RowsTop + 34.0f},
                   QStringLiteral("midleft"), 210);
    }

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

        for (int column = 0; column < int(kinds.size()); ++column) {
            const int kind = kinds[size_t(column)];
            const QRectF cell = cellRect(row, column);
            const bool active = activeRow && column == column_;
            if (active) {
                Base::panel(painter, cell.adjusted(3.0f, 3.0f, -3.0f, -3.0f),
                            kindColor(kind), 120, 6, QColor(9, 26, 40));
            }
            if (kind == 4) {
                const bool boss = waveHasBoss(row);
                Base::text(painter, Base::techSmallFont(),
                           boss ? QStringLiteral("BOSS") : QStringLiteral("—"),
                           boss ? Base::Magenta : Base::Dim,
                           {float(cell.center().x()), float(cell.center().y())},
                           QStringLiteral("center"), boss ? 235 : 190);
                continue;
            }
            const int count = enemyCountKind(row, kind);
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

    if (mode_ == Mode::Host) {
        Base::panel(painter, QRectF(Left, portRect().top() - 6.0f, Right - Left, 40.0f),
                    QColor(43, 83, 103), 180, 8, QColor(5, 13, 23));
        Base::text(painter, Base::techTinyFont(), QStringLiteral("服务器端口"),
                   Base::Muted, {Left + 18.0f, float(portRect().center().y()) + 2.0f},
                   QStringLiteral("midleft"), 200);
        Base::panel(painter, portRect(),
                    focus_ == Focus::Port ? Base::Cyan : QColor(60, 104, 126),
                    focus_ == Focus::Port ? 240 : 190, 8, QColor(3, 10, 18));
        Base::text(painter, Base::mediumFont(), portText_,
                   portText_.isEmpty() ? Base::Muted : Base::White,
                   {456, float(portRect().center().y()) + 2.0f},
                   QStringLiteral("midleft"), 220);
        Base::text(painter, Base::tinyFont(),
                   QStringLiteral("自机房：同一局域网客户端填本机地址 + 此端口"),
                   Base::Muted, {float(portRect().right()) + 18.0f,
                                 float(portRect().center().y()) + 2.0f},
                   QStringLiteral("midleft"), 200);
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

    // 底部敌人库
    Base::panel(painter, paletteRect(), QColor(48, 91, 112), 200, 10, QColor(6, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("敌人库"), Base::Cyan,
               {Left + 18.0f, PaletteTop + 24.0f}, QStringLiteral("midleft"), 230);
    Base::text(painter, Base::tinyFont(),
               QStringLiteral("拖到波次上 / 按 1–9、0"), Base::Muted,
               {Left + 18.0f, PaletteTop + 48.0f}, QStringLiteral("midleft"), 190);
    for (int kind = 0; kind < Kinds; ++kind) {
        const QRectF chip = paletteChipRect(kind);
        const QColor color = kindColor(kind);
        const bool dragging = dragKind_ == kind;
        Base::panel(painter, chip, dragging ? Base::Cyan : color,
                    dragging ? 240 : 200, 8,
                    dragging ? QColor(10, 26, 38) : QColor(8, 20, 32));
        const QString badge = kind < 9 ? QString::number(kind + 1)
                                       : (kind == 9 ? QStringLiteral("0")
                                                    : QStringLiteral("·"));
        Base::text(painter, Base::techTinyFont(), badge, Base::Muted,
                   {float(chip.left()) + 14.0f, float(chip.center().y())},
                   QStringLiteral("midleft"), 200);
        Base::text(painter, Base::techTinyFont(), kindLabel(kind), Base::White,
                   {float(chip.left()) + 30.0f, float(chip.center().y())},
                   QStringLiteral("midleft"), 225);
    }

    if (dragKind_ >= 0) {
        for (int row = 0; row < waveCount(); ++row) {
            if (!rowRect(row).contains(QPointF(dragPos_.x, dragPos_.y))) {
                continue;
            }
            painter.save();
            painter.setBrush(Qt::NoBrush);
            QPen pen(Base::Cyan);
            pen.setWidthF(2.0f);
            painter.setPen(pen);
            painter.drawRoundedRect(rowRect(row).adjusted(1.0f, 1.0f, -1.0f, -1.0f),
                                    4.0f, 4.0f);
            painter.restore();
            break;
        }
        const QRectF ghost(dragPos_.x - PaletteChipWidth * 0.5f,
                           dragPos_.y - PaletteChipHeight * 0.5f, PaletteChipWidth,
                           PaletteChipHeight);
        const QColor color = kindColor(dragKind_);
        Base::panel(painter, ghost, color, 200, 8, Base::withAlpha(color, 60));
        Base::text(painter, Base::techSmallFont(), kindLabel(dragKind_), Base::White,
                   {float(ghost.center().x()), float(ghost.center().y())},
                   QStringLiteral("center"), 240);
    }

    Base::text(painter, Base::tinyFont(),
               QStringLiteral("F1 单机  F2 局域网主机  F3 加入主机  ENTER 按当前选项卡开始"),
               QColor(105, 138, 158),
               {GameConfig::ScreenWidth / 2.0f, GameConfig::ScreenHeight - 26.0f},
               QStringLiteral("center"), 170);
}

} // namespace menu
