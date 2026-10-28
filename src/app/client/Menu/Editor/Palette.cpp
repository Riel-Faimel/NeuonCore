#include "Menu/Editor/Palette.h"

#include "Arena/Combatants/Enemies/DroneEnemy.h"
#include "Arena/Combatants/Enemy.h"
#include "Base/Base.h"
#include "Menu/Editor/Theme.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPen>
#include <Qt>

namespace menu::editor {

namespace {
const QString kKeys[kKindCount] = {
    QStringLiteral("drone"),       QStringLiteral("spitter"),
    QStringLiteral("tank"),        QStringLiteral("splitter"),
    QStringLiteral("boss"),        QStringLiteral("amplifier"),
    QStringLiteral("linker"),      QStringLiteral("mender"),
    QStringLiteral("rusher"),      QStringLiteral("necromancer"),
    QStringLiteral("summoner")};
const QString kLabels[kKindCount] = {
    QStringLiteral("DRONE"),       QStringLiteral("SPITTER"),
    QStringLiteral("TANK"),        QStringLiteral("SPLITTER"),
    QStringLiteral("BOSS"),        QStringLiteral("AMPLIFIER"),
    QStringLiteral("LINKER"),      QStringLiteral("MENDER"),
    QStringLiteral("RUSHER"),      QStringLiteral("NECRO"),
    QStringLiteral("SUMMONER")};
} // namespace

arena::EnemyType Palette::kindType(int kind)
{
    const QString key = kind >= 0 && kind < kKindCount ? kKeys[size_t(kind)]
                                                       : kKeys[0];
    const auto type = arena::Enemy::typeForKey(key);
    return type ? *type : arena::EnemyType(typeid(arena::DroneEnemy));
}

QString Palette::kindLabel(int kind)
{
    return kind >= 0 && kind < kKindCount ? kLabels[size_t(kind)] : QString();
}

QColor Palette::kindColor(int kind)
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

bool Palette::waveHasKind(const games::Wave &wave, int kind)
{
    const arena::EnemyType type = kindType(kind);
    for (const games::EnemySpawn &spawn : wave.enemies) {
        if (spawn.type == type) {
            return true;
        }
    }
    return false;
}

std::vector<int> Palette::visibleKinds(const games::Script &script)
{
    std::vector<int> kinds;
    for (int kind = 0; kind < kKindCount; ++kind) {
        for (const games::Wave &wave : script) {
            if (waveHasKind(wave, kind)) {
                kinds.push_back(kind);
                break;
            }
        }
    }
    return kinds;
}

int Palette::columnForKind(const games::Script &script, int kind)
{
    const std::vector<int> kinds = visibleKinds(script);
    for (int i = 0; i < int(kinds.size()); ++i) {
        if (kinds[size_t(i)] == kind) {
            return i;
        }
    }
    return -1;
}

QRectF Palette::rect() const
{
    return QRectF(Theme::Left, Theme::PaletteTop, Theme::Right - Theme::Left,
                  Theme::PaletteHeight);
}

QRectF Palette::chipRect(int kind) const
{
    const int row = kind / 6;
    const int column = kind % 6;
    return QRectF(Theme::PaletteChipsLeft
                      + float(column) * (Theme::PaletteChipWidth + Theme::PaletteGap),
                  Theme::PaletteTop + 12.0f
                      + float(row) * (Theme::PaletteChipHeight + 7.0f),
                  Theme::PaletteChipWidth, Theme::PaletteChipHeight);
}

void Palette::draw(QPainter &painter, const Pointer &pointer, int dragKind,
                   const QColor &tint) const
{
    const QRectF panel = rect();
    Theme::panel(painter, panel, QColor(48, 91, 112), 190, 10, QColor(6, 14, 25));
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(Base::withAlpha(tint, 100));
    painter.drawRoundedRect(QRectF(float(panel.left()) + 16.0f,
                                   float(panel.top()) + 1.0f,
                                   float(panel.width()) - 32.0f, 2.0f),
                            1.0f, 1.0f);
    painter.restore();

    Base::text(painter, Theme::smallFont(), QStringLiteral("敌人库"), Base::White,
               {Theme::Left + 20.0f, Theme::PaletteTop + 28.0f},
               QStringLiteral("midleft"), 235);
    Base::text(painter, Theme::techTinyFont(), QStringLiteral("ENEMY PALETTE"), tint,
               {Theme::Left + 20.0f, Theme::PaletteTop + 50.0f},
               QStringLiteral("midleft"), 215);
    Base::text(painter, Theme::tinyFont(),
               QStringLiteral("拖到波次 / 按 1–9、0"), Base::Muted,
               {Theme::Left + 20.0f, Theme::PaletteTop + 72.0f},
               QStringLiteral("midleft"), 190);
    Base::strokeLine(painter, {Theme::Left + 206.0f, Theme::PaletteTop + 16.0f},
                     {Theme::Left + 206.0f,
                      Theme::PaletteTop + Theme::PaletteHeight - 16.0f},
                     QColor(40, 72, 92), 1.0f);

    for (int kind = 0; kind < kKindCount; ++kind) {
        const QRectF chip = chipRect(kind);
        const QColor color = kindColor(kind);
        const bool dragging = dragKind == kind;
        const bool hovered = pointer.hovering(chip);

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
        Theme::keycap(painter, key, badge,
                      dragging || hovered ? Base::withAlpha(color, 180)
                                          : QColor(46, 78, 96),
                      dragging || hovered ? Base::White : Base::Muted, 220, 4.0f);

        Base::fillCircle(painter, {float(chip.left()) + 37.0f,
                                   float(chip.center().y())},
                         3.2f, color);
        if (kind == BossKind) {
            Base::strokeCircle(painter, {float(chip.left()) + 37.0f,
                                         float(chip.center().y())},
                               5.6f, Base::withAlpha(color, 120), 1.0f);
        }
        Base::text(painter, Theme::techTinyFont(), kindLabel(kind),
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

void Palette::drawGhost(QPainter &painter, int kind, const Math::Vec2 &position) const
{
    if (kind < 0) {
        return;
    }
    const QRectF ghost(position.x - Theme::PaletteChipWidth * 0.5f,
                       position.y - Theme::PaletteChipHeight * 0.5f,
                       Theme::PaletteChipWidth, Theme::PaletteChipHeight);
    const QColor color = kindColor(kind);
    Theme::panel(painter, ghost, color, 210, 8, Base::withAlpha(color, 70));
    Base::text(painter, Theme::techSmallFont(), kindLabel(kind), Base::White,
               {float(ghost.center().x()), float(ghost.center().y())},
               QStringLiteral("center"), 245);
}

} // namespace menu::editor
