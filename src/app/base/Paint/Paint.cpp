#include "Paint.h"

#include <QFontDatabase>
#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>

#include <algorithm>
#include <array>
#include <cmath>

namespace base {
bool Paint::sFontsReady = false;
QFont Paint::sTiny;
QFont Paint::sSmall;
QFont Paint::sBody;
QFont Paint::sMedium;
QFont Paint::sLarge;
QFont Paint::sBig;
QFont Paint::sTitle;
QFont Paint::sHuge;
QFont Paint::sTechTiny;
QFont Paint::sTechSmall;
QFont Paint::sTech;
QFont Paint::sTechLarge;

const QColor Paint::Ink{233, 246, 255};
const QColor Paint::Text{207, 226, 242};
const QColor Paint::Muted{119, 146, 171};
const QColor Paint::Dim{72, 96, 120};
const QColor Paint::Cyan{83, 232, 255};
const QColor Paint::CyanSoft{39, 152, 181};
const QColor Paint::Magenta{255, 91, 173};
const QColor Paint::Coral{255, 96, 112};
const QColor Paint::Amber{255, 171, 78};
const QColor Paint::Violet{181, 128, 255};
const QColor Paint::Green{105, 255, 184};
const QColor Paint::White{244, 251, 255};
const QColor Paint::Red{255, 72, 88};
const QColor Paint::Panel{7, 14, 27};
const QColor Paint::Black{1, 3, 8};
void Paint::initialize()
{
    if (sFontsReady) {
        return;
    }
    sFontsReady = true;

    QString cjkFamily;
    int cjkId = QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/msyhbd.ttc"));
    if (cjkId < 0) {
        cjkId = QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/simhei.ttf"));
    }
    if (cjkId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(cjkId);
        if (!families.isEmpty()) {
            cjkFamily = families.first();
        }
    }
    if (cjkFamily.isEmpty()) {
        cjkFamily = QStringLiteral("Microsoft YaHei");
    }

    QString techFamily;
    const int techId = QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/bahnschrift.ttf"));
    if (techId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(techId);
        if (!families.isEmpty()) {
            techFamily = families.first();
        }
    }
    if (techFamily.isEmpty()) {
        techFamily = QStringLiteral("Consolas");
    }

    const auto make = [&](int pixelSize, bool bold, bool tech) {
        QFont font(tech ? techFamily : cjkFamily);
        font.setPixelSize(pixelSize);
        font.setWeight(bold ? QFont::Bold : QFont::Normal);
        font.setHintingPreference(QFont::PreferNoHinting);
        font.setStyleStrategy(QFont::PreferAntialias);
        return font;
    };

    sTiny = make(13, false, false);
    sSmall = make(16, false, false);
    sBody = make(19, false, false);
    sMedium = make(23, false, false);
    sLarge = make(32, true, false);
    sBig = make(44, true, false);
    sTitle = make(92, true, false);
    sHuge = make(108, true, false);
    sTechTiny = make(12, false, true);
    sTechSmall = make(14, false, true);
    sTech = make(18, false, true);
    sTechLarge = make(28, true, true);
}

const QFont &Paint::tinyFont() { return sTiny; }
const QFont &Paint::smallFont() { return sSmall; }
const QFont &Paint::bodyFont() { return sBody; }
const QFont &Paint::mediumFont() { return sMedium; }
const QFont &Paint::largeFont() { return sLarge; }
const QFont &Paint::bigFont() { return sBig; }
const QFont &Paint::titleFont() { return sTitle; }
const QFont &Paint::hugeFont() { return sHuge; }
const QFont &Paint::techTinyFont() { return sTechTiny; }
const QFont &Paint::techSmallFont() { return sTechSmall; }
const QFont &Paint::techFont() { return sTech; }
const QFont &Paint::techLargeFont() { return sTechLarge; }

QColor Paint::shade(const QColor &color, float factor)
{
    return QColor(int(Math::clamp(color.red() * factor, 0.0f, 255.0f)),
                  int(Math::clamp(color.green() * factor, 0.0f, 255.0f)),
                  int(Math::clamp(color.blue() * factor, 0.0f, 255.0f)),
                  color.alpha());
}

QColor Paint::withAlpha(const QColor &color, int alpha)
{
    return QColor(color.red(), color.green(), color.blue(), std::clamp(alpha, 0, 255));
}

void Paint::fillCircle(QPainter &painter, const Math::Vec2 &center,
                      float radius, const QColor &color)
{
    if (radius <= 0.0f || color.alpha() <= 0) {
        return;
    }
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPointF(center.x, center.y), radius, radius);
    painter.restore();
}

void Paint::strokeCircle(QPainter &painter, const Math::Vec2 &center,
                        float radius, const QColor &color, float width)
{
    if (radius <= 0.0f || color.alpha() <= 0) {
        return;
    }
    painter.save();
    painter.setBrush(Qt::NoBrush);
    QPen pen(color);
    pen.setWidthF(width);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawEllipse(QPointF(center.x, center.y), radius, radius);
    painter.restore();
}

void Paint::strokeLine(QPainter &painter, const Math::Vec2 &start,
                      const Math::Vec2 &end, const QColor &color, float width)
{
    painter.save();
    QPen pen(color);
    pen.setWidthF(width);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(QPointF(start.x, start.y), QPointF(end.x, end.y));
    painter.restore();
}

void Paint::arcRadians(QPainter &painter, const Math::Vec2 &center, float radius,
                      float startRadians, float spanRadians,
                      const QColor &color, float width)
{
    if (radius <= 0.0f || color.alpha() <= 0) {
        return;
    }
    painter.save();
    painter.setBrush(Qt::NoBrush);
    QPen pen(color);
    pen.setWidthF(width);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    const QRectF rect(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);
    const int startAngle = int(std::lround(-startRadians * 180.0f / Math::Pi * 16.0f));
    const int spanAngle = int(std::lround(-spanRadians * 180.0f / Math::Pi * 16.0f));
    painter.drawArc(rect, startAngle, spanAngle);
    painter.restore();
}

void Paint::reticle(QPainter &painter, const Math::Vec2 &center, float time)
{
    fillCircle(painter, center, 2.0f, Cyan);
    for (int i = 0; i < 4; ++i) {
        const float angle = Math::Pi * 0.5f * float(i);
        const Math::Vec2 direction(std::cos(angle), std::sin(angle));
        strokeLine(painter,
                   {center.x + direction.x * 8.0f, center.y + direction.y * 8.0f},
                   {center.x + direction.x * 15.0f, center.y + direction.y * 15.0f},
                   Cyan, 1.0f);
    }
    arcRadians(painter, center, 22.0f, time * 0.8f, 1.2f,
               QColor(81, 169, 190), 1.0f);
}

void Paint::gear(QPainter &painter, const Math::Vec2 &center, float radius,
                 const QColor &color)
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    const float toothWidth = radius * 0.42f;
    const float toothLength = radius * 0.36f;
    for (int i = 0; i < 8; ++i) {
        painter.save();
        painter.translate(center.x, center.y);
        painter.rotate(22.5f + float(i) * 45.0f);
        painter.drawRoundedRect(
            QRectF(-toothWidth * 0.5f, -radius - toothLength * 0.2f,
                   toothWidth, toothLength),
            toothWidth * 0.22f, toothWidth * 0.22f);
        painter.restore();
    }
    QPainterPath ring;
    ring.addEllipse(QPointF(center.x, center.y), radius * 0.78f, radius * 0.78f);
    ring.addEllipse(QPointF(center.x, center.y), radius * 0.34f, radius * 0.34f);
    ring.setFillRule(Qt::OddEvenFill);
    painter.fillPath(ring, color);
    painter.restore();
}

void Paint::crate(QPainter &painter, const Math::Vec2 &center, float radius,
                  const QColor &color)
{
    painter.save();
    QPen pen(color);
    pen.setWidthF(1.6f);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    const QRectF box(center.x - radius, center.y - radius * 0.8f, radius * 2.0f,
                     radius * 1.6f);
    painter.drawRoundedRect(box, 2.0f, 2.0f);
    painter.drawLine(box.topLeft(), box.bottomRight());
    painter.drawLine(box.topRight(), box.bottomLeft());
    painter.drawLine(QPointF(box.left(), box.center().y()),
                     QPointF(box.right(), box.center().y()));
    painter.restore();
}

QPixmap Paint::appIcon(int size)
{
    constexpr int canvas = 256;
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    if (size <= 0) {
        return pixmap;
    }
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.scale(float(size) / float(canvas), float(size) / float(canvas));

    QLinearGradient background(0, 0, 0, canvas);
    background.setColorAt(0.0, QColor(10, 26, 48));
    background.setColorAt(0.55, QColor(5, 12, 26));
    background.setColorAt(1.0, QColor(1, 3, 8));
    painter.setBrush(background);
    painter.setPen(QPen(withAlpha(Cyan, 160), 6.0f));
    painter.drawRoundedRect(QRectF(7, 7, canvas - 14, canvas - 14), 52, 52);

    const Math::Vec2 center(canvas * 0.5f, canvas * 0.5f);

    // 准星语言：左右两段外弧
    arcRadians(painter, center, 96.0f, -0.72f, 1.22f, withAlpha(Cyan, 190), 4.0f);
    arcRadians(painter, center, 96.0f, Math::Pi - 0.72f, 1.22f,
               withAlpha(Magenta, 180), 4.0f);

    // 六边形“星核”
    const auto hexagon = [&](float radius, QPolygonF &points) {
        points.clear();
        for (int i = 0; i < 6; ++i) {
            const float angle = -Math::Pi * 0.5f + Math::Pi / 3.0f * float(i);
            points.append(QPointF(center.x + std::cos(angle) * radius,
                                  center.y + std::sin(angle) * radius));
        }
    };
    QPolygonF outer;
    hexagon(78.0f, outer);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(withAlpha(Cyan, 55), 14.0f));
    painter.drawPolygon(outer);
    painter.setPen(QPen(withAlpha(Cyan, 235), 5.0f));
    painter.drawPolygon(outer);

    QPolygonF inner;
    hexagon(52.0f, inner);
    painter.setBrush(withAlpha(Cyan, 36));
    painter.setPen(QPen(withAlpha(Cyan, 120), 2.0f));
    painter.drawPolygon(inner);

    // 核心光点
    painter.setPen(Qt::NoPen);
    painter.setBrush(withAlpha(Cyan, 45));
    painter.drawEllipse(QPointF(center.x, center.y), 34.0, 34.0);
    painter.setBrush(Cyan);
    painter.drawEllipse(QPointF(center.x, center.y), 17.0, 17.0);
    painter.setBrush(White);
    painter.drawEllipse(QPointF(center.x, center.y), 8.0, 8.0);

    painter.end();
    return pixmap;
}

void Paint::polygon(QPainter &painter, const QPolygonF &points,
                   const QColor &fill, const QColor &outline, float width)
{
    if (points.size() < 3) {
        return;
    }
    painter.save();
    painter.setBrush(fill.alpha() > 0 ? QBrush(fill) : QBrush(Qt::NoBrush));
    if (outline.alpha() > 0 && width > 0.0f) {
        QPen pen(outline);
        pen.setWidthF(width);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
    } else {
        painter.setPen(Qt::NoPen);
    }
    painter.drawPolygon(points);
    painter.restore();
}

void Paint::glow(QPainter &glowPainter, const Math::Vec2 &pos, float radius,
                const QColor &color, float intensity)
{
    const std::array<std::pair<float, float>, 3> layers = {
        std::pair{1.75f, 0.055f},
        std::pair{1.22f, 0.10f},
        std::pair{0.78f, 0.18f},
    };
    for (const auto &[scale, strength] : layers) {
        const QColor layerColor(
            int(Math::clamp(color.red() * strength * intensity, 0.0f, 255.0f)),
            int(Math::clamp(color.green() * strength * intensity, 0.0f, 255.0f)),
            int(Math::clamp(color.blue() * strength * intensity, 0.0f, 255.0f)),
            255);
        fillCircle(glowPainter, pos, std::max(1.0f, radius * scale), layerColor);
    }
}

QRectF Paint::textBounds(const QFont &font, const QString &value,
                        const Math::Vec2 &pos, const QString &anchor)
{
    const QFontMetricsF metrics(font);
    const float width = float(metrics.horizontalAdvance(value));
    const float height = float(metrics.height());
    float x = pos.x;
    float y = pos.y;

    if (anchor == QStringLiteral("center")) {
        x -= width * 0.5f;
        y -= height * 0.5f;
    } else if (anchor == QStringLiteral("midleft")) {
        y -= height * 0.5f;
    } else if (anchor == QStringLiteral("midright")) {
        x -= width;
        y -= height * 0.5f;
    } else if (anchor == QStringLiteral("topright")) {
        x -= width;
    } else if (anchor == QStringLiteral("midtop")) {
        x -= width * 0.5f;
    } else if (anchor == QStringLiteral("bottomleft")) {
        y -= height;
    } else if (anchor == QStringLiteral("bottomcenter")) {
        x -= width * 0.5f;
        y -= height;
    }
    return QRectF(x, y, width, height);
}

void Paint::text(QPainter &painter, const QFont &font, const QString &value,
                const QColor &color, const Math::Vec2 &pos,
                const QString &anchor, int alpha)
{
    if (alpha <= 2 || value.isEmpty()) {
        return;
    }
    const QRectF rect = textBounds(font, value, pos, anchor);
    const QFontMetricsF metrics(font);
    painter.save();
    painter.setOpacity(Math::clamp(alpha / 255.0f, 0.0f, 1.0f));
    painter.setFont(font);
    painter.setPen(QColor(color.red(), color.green(), color.blue()));
    painter.drawText(QPointF(rect.left(), rect.top() + metrics.ascent()), value);
    painter.restore();
}

void Paint::shadowText(QPainter &painter, const QFont &font, const QString &value,
                      const QColor &color, const Math::Vec2 &pos,
                      const QString &anchor, int alpha, const QColor &shadow)
{
    text(painter, font, value, shadow, {pos.x + 2.0f, pos.y + 3.0f},
         anchor, int(alpha * 0.75f));
    text(painter, font, value, color, pos, anchor, alpha);
}

void Paint::glowText(QPainter &painter, const QFont &font, const QString &value,
                    const QColor &color, const Math::Vec2 &pos,
                    const QString &anchor, int alpha)
{
    const QColor glowColor = shade(color, 0.45f);
    const std::array<Math::Vec2, 6> offsets = {
        Math::Vec2{-3, 0}, Math::Vec2{3, 0}, Math::Vec2{0, -3},
        Math::Vec2{0, 3}, Math::Vec2{-2, -2}, Math::Vec2{2, 2},
    };
    for (const Math::Vec2 &offset : offsets) {
        text(painter, font, value, glowColor, pos + offset, anchor, int(alpha * 0.42f));
    }
    text(painter, font, value, color, pos, anchor, alpha);
}

QStringList Paint::wrapText(const QString &value, const QFont &font, float maxWidth)
{
    QStringList lines;
    QString current;
    const QFontMetricsF metrics(font);
    for (QChar character : value) {
        if (character == QLatin1Char('\n')) {
            lines.append(current);
            current.clear();
            continue;
        }
        const QString trial = current + character;
        if (metrics.horizontalAdvance(trial) > maxWidth && !current.isEmpty()) {
            lines.append(current);
            current = character;
        } else {
            current = trial;
        }
    }
    if (!current.isEmpty()) {
        lines.append(current);
    }
    return lines;
}

void Paint::bar(QPainter &painter, const QRectF &rect, float ratio,
               const QColor &color, float ghost, const QColor &background)
{
    ratio = Math::clamp(ratio, 0.0f, 1.0f);
    const float radius = std::max(2.0f, float(rect.height()) / 2.0f);
    painter.save();
    QPainterPath clip;
    clip.addRoundedRect(rect, radius, radius);
    painter.setClipPath(clip);
    painter.fillRect(rect, background);

    if (ghost > ratio) {
        QRectF ghostRect = rect;
        ghostRect.setWidth(std::max(1.0f, float(rect.width())
                                             * Math::clamp(ghost, 0.0f, 1.0f)));
        painter.fillRect(ghostRect, QColor(118, 35, 48));
    }
    if (ratio > 0.0f) {
        QRectF fill = rect;
        fill.setWidth(std::max(1.0f, float(rect.width()) * ratio));
        painter.fillRect(fill, color);
        QRectF highlight = fill;
        highlight.setHeight(std::max(2.0f, float(fill.height()) / 3.0f));
        painter.fillRect(highlight, shade(color, 1.22f));
    }
    painter.restore();

    painter.save();
    painter.setBrush(Qt::NoBrush);
    QPen pen(QColor(48, 70, 88));
    pen.setWidthF(1.0f);
    painter.setPen(pen);
    painter.drawRoundedRect(rect, radius, radius);
    painter.restore();
}

void Paint::panel(QPainter &painter, const QRectF &rect, const QColor &border,
                 int alpha, int radius, const QColor &fill)
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(fill.red(), fill.green(), fill.blue(), alpha));
    painter.drawRoundedRect(rect, radius, radius);
    painter.setBrush(Qt::NoBrush);
    QPen pen(withAlpha(border, 190));
    pen.setWidthF(1.0f);
    painter.setPen(pen);
    painter.drawRoundedRect(rect, radius, radius);
    painter.restore();
}
} // namespace base
