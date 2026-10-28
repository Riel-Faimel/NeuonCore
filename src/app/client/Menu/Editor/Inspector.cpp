#include "Menu/Editor/Inspector.h"

#include "Base/Base.h"
#include "Menu/Editor/Palette.h"
#include "Menu/Editor/ServerRow.h"
#include "Menu/Editor/Theme.h"
#include "Menu/Editor/WaveTable.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPen>
#include <Qt>

#include <algorithm>
#include <vector>

namespace menu::editor {

QRectF Inspector::rect(int waveCount, Mode mode) const
{
    const float tableBottom = Theme::RowsTop + float(waveCount) * Theme::RowHeight;
    float top = std::max(388.0f, tableBottom + 18.0f);
    if (mode == Mode::Host) {
        top = std::max(top, float(ServerRow::rowRect(waveCount).bottom()) + 18.0f);
    }
    const float height = std::min(150.0f, Theme::PaletteTop - 14.0f - top);
    if (height < 116.0f) {
        return QRectF();
    }
    return QRectF(Theme::Left, top, Theme::Right - Theme::Left, height);
}

void Inspector::draw(QPainter &painter, const games::Script &script, int row,
                     Mode mode, const QColor &tint) const
{
    const int waveCount = int(script.size());
    const QRectF rect = this->rect(waveCount, mode);
    if (rect.isEmpty()) {
        return;
    }
    const float top = float(rect.top());
    const float x1 = float(rect.left()) + float(rect.width()) * 0.355f;
    const float x2 = float(rect.left()) + float(rect.width()) * 0.69f;

    Theme::panel(painter, rect, Base::shade(tint, 0.55f), 160, 10,
                 QColor(5, 13, 23));
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(Base::withAlpha(tint, 120));
    painter.drawRoundedRect(QRectF(float(rect.left()) + 18.0f,
                                   float(rect.top()) + 1.0f,
                                   float(rect.width()) - 36.0f, 2.0f),
                            1.0f, 1.0f);
    painter.restore();
    Base::strokeLine(painter, {x1, top + 16.0f}, {x1, float(rect.bottom()) - 16.0f},
                     Base::withAlpha(tint, 55), 1.0f);
    Base::strokeLine(painter, {x2, top + 16.0f}, {x2, float(rect.bottom()) - 16.0f},
                     Base::withAlpha(tint, 55), 1.0f);

    // 第一列：当前选中波次
    Base::text(painter, Theme::techTinyFont(),
               QStringLiteral("SELECTED // 当前波次"), tint,
               {Theme::Left + 24.0f, top + 18.0f}, QStringLiteral("midleft"), 220);
    Base::text(painter, Theme::largeFont(),
               QStringLiteral("%1").arg(row + 1, 2, 10, QChar('0')), Base::White,
               {Theme::Left + 24.0f, top + 47.0f}, QStringLiteral("midleft"), 245);
    Base::text(painter, Theme::techTinyFont(),
               QStringLiteral("WAVE %1 / %2").arg(row + 1).arg(waveCount),
               Base::Muted, {Theme::Left + 92.0f, top + 41.0f},
               QStringLiteral("midleft"), 195);
    Base::text(painter, Theme::techSmallFont(),
               QStringLiteral("%1 个目标").arg(WaveTable::waveTotal(script, row)),
               Base::Text, {Theme::Left + 92.0f, top + 61.0f},
               QStringLiteral("midleft"), 225);

    if (rect.height() >= 132.0f) {
        const QFontMetricsF metrics(Theme::techTinyFont());
        float x = Theme::Left + 24.0f;
        const float y = top + 91.0f;
        bool any = false;
        const std::vector<int> kinds = Palette::visibleKinds(script);
        for (int kind : kinds) {
            const int count = WaveTable::enemyCountKind(script, row, kind);
            if (count <= 0) {
                continue;
            }
            const QString part = QStringLiteral("%1 ×%2")
                                     .arg(Palette::kindLabel(kind))
                                     .arg(count);
            const float width = float(metrics.horizontalAdvance(part));
            if (x + width > x1 - 28.0f) {
                Base::text(painter, Theme::techTinyFont(), QStringLiteral("…"),
                           Base::Dim, {x, y}, QStringLiteral("midleft"), 190);
                break;
            }
            Base::text(painter, Theme::techTinyFont(), part,
                       Palette::kindColor(kind), {x, y}, QStringLiteral("midleft"),
                       225);
            x += width + 22.0f;
            any = true;
        }
        if (WaveTable::waveHasBoss(script, row)) {
            Base::text(painter, Theme::techTinyFont(), QStringLiteral("BOSS ×1"),
                       Base::Magenta, {x, y}, QStringLiteral("midleft"), 230);
            any = true;
        }
        if (!any) {
            Base::text(painter, Theme::tinyFont(),
                       QStringLiteral("空波次 · 拖动敌人到这一波"), Base::Muted,
                       {Theme::Left + 24.0f, y}, QStringLiteral("midleft"), 190);
        }
    }

    Base::text(painter, Theme::tinyFont(),
               QStringLiteral("↑↓ 选择波次  ·  ←→ 选择列  ·  ENTER 开始 / 进入房间"),
               Base::Muted, {Theme::Left + 24.0f, float(rect.bottom()) - 17.0f},
               QStringLiteral("midleft"), 180);

    // 第二列：台本概览
    const float cx = x1 + 22.0f;
    Base::text(painter, Theme::techTinyFont(),
               QStringLiteral("SCRIPT // 台本概览"), tint, {cx, top + 18.0f},
               QStringLiteral("midleft"), 220);

    int bossRow = -1;
    for (int wave = 0; wave < waveCount; ++wave) {
        if (WaveTable::waveHasBoss(script, wave)) {
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
                                      .arg(waveCount, 2, 10, QChar('0'))
                                      .arg(kMaxWaves),
         Base::Text},
        {QStringLiteral("目标总数"), QString::number(WaveTable::totalEnemies(script)),
         Base::Text},
        {QStringLiteral("最终 BOSS"),
         bossRow >= 0 ? QStringLiteral("第 %1 波").arg(bossRow + 1)
                      : QStringLiteral("未配置"),
         bossRow >= 0 ? Base::Magenta : Base::Muted},
    };
    for (int i = 0; i < 3; ++i) {
        const float y = top + 45.0f + float(i) * 23.0f;
        Base::text(painter, Theme::techTinyFont(), rows[i].label, Base::Muted,
                   {cx, y}, QStringLiteral("midleft"), 195);
        Base::text(painter, Theme::techSmallFont(), rows[i].value, rows[i].color,
                   {cx + 82.0f, y}, QStringLiteral("midleft"), 225);
    }

    // 第三列：键鼠提示
    const float rx = x2 + 22.0f;
    Base::text(painter, Theme::techTinyFont(), QStringLiteral("CONTROLS // 操作"),
               tint, {rx, top + 18.0f}, QStringLiteral("midleft"), 220);
    const QString lines[] = {
        QStringLiteral("↑↓ / W S 选择波次    ←→ / A D 选择列"),
        QStringLiteral("按住 + / − 连续调整  ·  N / X 增删波次"),
        QStringLiteral("拖入敌人库或按 1–9 / 0 直接加入"),
    };
    for (int i = 0; i < 3; ++i) {
        Base::text(painter, Theme::tinyFont(), lines[i], QColor(158, 184, 202),
                   {rx, top + 45.0f + float(i) * 23.0f},
                   QStringLiteral("midleft"), 190);
    }
}

} // namespace menu::editor
