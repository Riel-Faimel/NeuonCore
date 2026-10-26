#include "Menu/Result/Result.h"

#include "Base/Base.h"
#include "GameConfig.h"

#include <QLocale>
#include <QPainter>
#include <Qt>

#include <array>
#include <utility>

namespace menu {

QRectF Result::retryRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f - 270.0f, 560.0f, 250, 48);
}

QRectF Result::menuRect() const
{
    return QRectF(GameConfig::ScreenWidth / 2.0f + 20.0f, 560.0f, 250, 48);
}

void Result::render(QPainter &painter, const Summary &summary) const
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRectF(0, 0, GameConfig::ScreenWidth, GameConfig::ScreenHeight),
                     QColor(2, 3, 9, 210));
    const QColor accent = summary.victory ? Base::Cyan : Base::Red;
    Base::strokeLine(painter, {GameConfig::ScreenWidth / 2.0f - 250, 118},
                     {GameConfig::ScreenWidth / 2.0f + 250, 118}, accent, 1.0f);
    Base::text(painter, Base::techSmallFont(),
               summary.victory ? QStringLiteral("MISSION COMPLETE  //  CORE STABILIZED")
                               : QStringLiteral("MISSION FAILED  //  CORE SIGNAL LOST"),
               accent, {GameConfig::ScreenWidth / 2.0f, 152},
               QStringLiteral("center"), 230);
    Base::glowText(painter, Base::bigFont(),
                   summary.victory ? QStringLiteral("星核已稳定")
                                   : QStringLiteral("防线失守"),
                   summary.victory ? QColor(151, 255, 220) : QColor(255, 119, 133),
                   {GameConfig::ScreenWidth / 2.0f, 222}, QStringLiteral("center"));
    Base::text(painter, Base::bodyFont(),
               summary.victory ? QStringLiteral("你守住了最后一道光。")
                               : QStringLiteral("星核在黑暗中熄灭。"),
               QColor(194, 224, 232),
               {GameConfig::ScreenWidth / 2.0f, 278}, QStringLiteral("center"), 225);
    Base::text(painter, Base::smallFont(), summary.levelName,
               Base::Muted, {GameConfig::ScreenWidth / 2.0f, 310},
               QStringLiteral("center"), 200);

    const int minutes = int(summary.runTime) / 60;
    const int seconds = int(summary.runTime) % 60;
    const QString timeText = QStringLiteral("%1:%2")
                                 .arg(minutes, 2, 10, QChar('0'))
                                 .arg(seconds, 2, 10, QChar('0'));
    static const QLocale english(QLocale::English);
    const std::array<std::pair<QString, QString>, 4> stats = {{
        {QStringLiteral("本局获得"), english.toString(summary.score)},
        {QStringLiteral("到达波次"),
         QStringLiteral("%1 / %2").arg(summary.wave).arg(summary.totalWaves)},
        {QStringLiteral("击破目标"), QString::number(summary.kills)},
        {QStringLiteral("存活时间"), timeText},
    }};
    constexpr float totalWidth = 760.0f;
    constexpr float cell = totalWidth / 4.0f;
    const float start = GameConfig::ScreenWidth / 2.0f - totalWidth / 2.0f;
    for (int i = 0; i < 4; ++i) {
        const float cx = start + cell * i + cell / 2.0f;
        if (i > 0) {
            Base::strokeLine(painter, {start + cell * i, 390},
                             {start + cell * i, 482}, QColor(44, 67, 83), 1.0f);
        }
        Base::text(painter, Base::tinyFont(), stats[size_t(i)].first, Base::Muted,
                   {cx, 390}, QStringLiteral("center"), 190);
        Base::text(painter, Base::largeFont(), stats[size_t(i)].second, Base::White,
                   {cx, 417}, QStringLiteral("center"));
    }

    Base::panel(painter, retryRect(), Base::Cyan, 230, 9, QColor(7, 18, 31));
    Base::panel(painter, menuRect(), QColor(65, 104, 124), 210, 9, QColor(7, 14, 25));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("R  重新部署"),
               Base::Cyan,
               {float(retryRect().center().x()), float(retryRect().center().y())},
               QStringLiteral("center"));
    Base::text(painter, Base::techSmallFont(), QStringLiteral("ESC  返回主菜单"),
               QColor(190, 215, 228),
               {float(menuRect().center().x()), float(menuRect().center().y())},
               QStringLiteral("center"));
}

Result::Action Result::keyPress(int key) const
{
    if (key == Qt::Key_R || key == Qt::Key_Return || key == Qt::Key_Enter) {
        return Action::Retry;
    }
    if (key == Qt::Key_Escape || key == Qt::Key_Q) {
        return Action::Back;
    }
    return Action::None;
}

Result::Action Result::mousePress(const Math::Vec2 &position) const
{
    const QPointF point(position.x, position.y);
    if (retryRect().contains(point)) {
        return Action::Retry;
    }
    if (menuRect().contains(point)) {
        return Action::Back;
    }
    return Action::None;
}

} // namespace menu
