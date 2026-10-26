#include "Menu/Editor/Editor.h"

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
    wave.enemies.push_back({kindKey(0), 6});
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
}

QString Editor::kindKey(int kind)
{
    return kind >= 0 && kind < Kinds ? kKeys[size_t(kind)] : QString();
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
    const QString key = kindKey(kind);
    for (const games::EnemySpawn &spawn : wave.enemies) {
        if (spawn.kind == key) {
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
    const QString key = kindKey(kind);
    for (const games::EnemySpawn &spawn : script_[size_t(row)].enemies) {
        if (spawn.kind == key) {
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
    const QString key = kindKey(kind);
    for (auto it = enemies.begin(); it != enemies.end(); ++it) {
        if (it->kind == key) {
            if (count <= 0) {
                enemies.erase(it);
            } else {
                it->count = count;
            }
            return;
        }
    }
    if (count > 0) {
        enemies.push_back({key, count});
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
                                              return spawn.kind == kindKey(4);
                                          }),
                           wave.enemies.end());
    }
    if (on && row >= 0 && row < waveCount()) {
        script_[size_t(row)].enemies.push_back({kindKey(4), 1});
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
            if (spawn.kind != kindKey(4)) {
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
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        return Action::Deploy;
    } else if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
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
        return Action::Deploy;
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
    return QRectF(Right - 634.0f, 190.0f, 150.0f, 34.0f);
}

QRectF Editor::removeWaveRect() const
{
    return QRectF(Right - 472.0f, 190.0f, 150.0f, 34.0f);
}

QRectF Editor::deployRect() const
{
    return QRectF(Right - 310.0f, 190.0f, 150.0f, 34.0f);
}

QRectF Editor::backRect() const
{
    return QRectF(Right - 148.0f, 190.0f, 148.0f, 34.0f);
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
               muted ? QStringLiteral("SCRIPT EDITOR  //  CUSTOM PROTOCOL  //  MUTED")
                     : QStringLiteral("SCRIPT EDITOR  //  CUSTOM PROTOCOL"),
               QColor(121, 176, 194),
               {GameConfig::ScreenWidth / 2.0f, 70}, QStringLiteral("center"), 190);
    Base::glowText(painter, Base::bigFont(), QStringLiteral("自定义协议"),
                   Base::White, {GameConfig::ScreenWidth / 2.0f, 118},
                   QStringLiteral("center"));
    Base::text(painter, Base::smallFont(),
               QStringLiteral("把底部敌人拖到波次上，或按 1–9、0 添加；ENTER 部署"),
               Base::Text,
               {GameConfig::ScreenWidth / 2.0f, 154}, QStringLiteral("center"), 215);

    Base::text(painter, Base::techSmallFont(),
               QStringLiteral("%1 波 · %2 个目标")
                   .arg(waveCount(), 2, 10, QChar('0'))
                   .arg(totalEnemies()),
               Base::Muted, {Left, 206}, QStringLiteral("midleft"), 210);

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
}

} // namespace menu
