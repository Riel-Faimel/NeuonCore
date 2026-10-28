#include "Menu/Editor/Actions.h"

#include "Base/Base.h"
#include "Menu/Editor/Theme.h"

#include <QPainter>
#include <Qt>

namespace menu::editor {

QRectF Actions::addWaveRect() const
{
    return QRectF(Theme::Right - 648.0f, Theme::ActionsTop, 136.0f,
                  Theme::ActionsHeight);
}

QRectF Actions::removeWaveRect() const
{
    return QRectF(Theme::Right - 498.0f, Theme::ActionsTop, 136.0f,
                  Theme::ActionsHeight);
}

QRectF Actions::deployRect() const
{
    return QRectF(Theme::Right - 348.0f, Theme::ActionsTop, 176.0f,
                  Theme::ActionsHeight);
}

QRectF Actions::backRect() const
{
    return QRectF(Theme::Right - 158.0f, Theme::ActionsTop, 158.0f,
                  Theme::ActionsHeight);
}

void Actions::draw(QPainter &painter, Mode mode, int waveCount, int totalEnemies,
                   const QString &hostText, const QString &portText,
                   bool startValid, const Pointer &pointer) const
{
    const QColor tint = Theme::accent(mode);
    if (mode == Mode::Join) {
        Base::text(painter, Theme::techTinyFont(),
                   QStringLiteral("JOIN SESSION // 加入"), tint,
                   {Theme::Left, Theme::ActionsTop + 11.0f},
                   QStringLiteral("midleft"), 215);
        Base::text(painter, Theme::techSmallFont(),
                   startValid ? QStringLiteral("连接 %1:%2").arg(hostText).arg(portText)
                              : QStringLiteral("填写主机地址与端口后才能连接"),
                   startValid ? Base::Green : Base::Muted,
                   {Theme::Left, Theme::ActionsTop + 27.0f},
                   QStringLiteral("midleft"), 220);
    } else {
        const float baseY = Theme::ActionsTop + 29.0f;
        for (int i = 0; i < 4; ++i) {
            const float height = 6.0f + float(i) * 4.0f;
            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(Base::withAlpha(tint, 90 + i * 38));
            painter.drawRoundedRect(QRectF(Theme::Left + float(i) * 6.0f,
                                           baseY - height, 3.5f, height),
                                    1.5f, 1.5f);
            painter.restore();
        }
        Base::text(painter, Theme::techTinyFont(),
                   QStringLiteral("WAVE SCRIPT // 台本"), Base::Muted,
                   {Theme::Left + 32.0f, Theme::ActionsTop + 11.0f},
                   QStringLiteral("midleft"), 195);
        Base::text(painter, Theme::techSmallFont(),
                   QStringLiteral("%1 波  ·  %2 个目标  ·  上限 %3 波")
                       .arg(waveCount, 2, 10, QChar('0'))
                       .arg(totalEnemies)
                       .arg(kMaxWaves),
                   Base::Text, {Theme::Left + 32.0f, Theme::ActionsTop + 27.0f},
                   QStringLiteral("midleft"), 228);
    }

    if (mode != Mode::Join) {
        Theme::button(painter, addWaveRect(), QStringLiteral("N"),
                      QStringLiteral("新增波次"), tint, false,
                      waveCount < kMaxWaves, pointer);
        Theme::button(painter, removeWaveRect(), QStringLiteral("X"),
                      QStringLiteral("删除末波"), tint, false, waveCount > 1,
                      pointer);
    }
    Theme::button(painter, deployRect(), QStringLiteral("ENTER"),
                  mode == Mode::Join
                      ? QStringLiteral("连接")
                      : (mode == Mode::Host ? QStringLiteral("进入房间")
                                            : QStringLiteral("开始")),
                  tint, true, startValid, pointer);
    Theme::button(painter, backRect(), QStringLiteral("ESC"),
                  QStringLiteral("返回"), tint, false, true, pointer);
}

} // namespace menu::editor
