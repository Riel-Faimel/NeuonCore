#include "base/Base.h"
#include "GameConfig.h"

#include <QAudioDevice>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QHash>
#include <QMediaDevices>
#include <QPainterPath>
#include <QPen>
#include <QSet>
#include <QSoundEffect>
#include <QStandardPaths>
#include <QUrl>
#include <QVector>
#include <Qt>
#include <QtAlgorithms>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

struct Base::Impl {
    static constexpr float Tau = 6.28318530717958647692f;
    static constexpr int SampleRate = 44100;
    static constexpr int Channels = 2;
    static constexpr int PoolSize = 4;

    QSet<int> keys;
    Math::Vec2 mousePosition{GameConfig::ScreenWidth / 2.0f,
                             GameConfig::ScreenHeight / 2.0f};
    bool mouseActive = false;
    bool mouseDown = false;

    bool audioEnabled = false;
    bool muted = false;
    bool audioInitialized = false;
    QHash<QString, QVector<QSoundEffect *>> pools;
    QHash<QString, int> next;

    ~Impl()
    {
        for (const auto &pool : pools) {
            qDeleteAll(pool);
        }
    }

    QByteArray makeWav(float duration, float freqStart, float freqEnd,
                       const QString &wave, float volume, float noiseMix)
    {
        const int count = std::max(1, int(SampleRate * duration));
        QByteArray pcm;
        pcm.reserve(count * Channels * 2);
        QDataStream pcmStream(&pcm, QIODevice::WriteOnly);
        pcmStream.setByteOrder(QDataStream::LittleEndian);

        float phase = 0.0f;
        std::mt19937 rng(7);
        std::uniform_real_distribution<float> noise(-1.0f, 1.0f);

        for (int i = 0; i < count; ++i) {
            const float t = float(i) / float(count);
            const float freq = freqStart + (freqEnd - freqStart) * t;
            phase += Tau * freq / float(SampleRate);

            float value = 0.0f;
            if (wave == QStringLiteral("square")) {
                value = std::sin(phase) >= 0.0f ? 1.0f : -1.0f;
            } else if (wave == QStringLiteral("saw")) {
                value = 2.0f * std::fmod(phase / Tau, 1.0f) - 1.0f;
            } else if (wave == QStringLiteral("noise")) {
                value = noise(rng);
            } else {
                value = std::sin(phase);
            }
            if (wave != QStringLiteral("noise") && noiseMix > 0.0f) {
                value = value * (1.0f - noiseMix) + noise(rng) * noiseMix;
            }

            const float attack = std::min(1.0f, t / 0.015f);
            const float release = std::min(1.0f, (1.0f - t) / 0.22f);
            const float mixed = std::clamp(value * attack * release * volume,
                                           -1.0f, 1.0f);
            const qint16 sample = qint16(mixed * 32767.0f);
            for (int channel = 0; channel < Channels; ++channel) {
                pcmStream << sample;
            }
        }

        QByteArray wav;
        QDataStream out(&wav, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out.writeRawData("RIFF", 4);
        out << quint32(36 + pcm.size());
        out.writeRawData("WAVE", 4);
        out.writeRawData("fmt ", 4);
        out << quint32(16);
        out << quint16(1);
        out << quint16(Channels);
        out << quint32(SampleRate);
        out << quint32(SampleRate * Channels * 2);
        out << quint16(Channels * 2);
        out << quint16(16);
        out.writeRawData("data", 4);
        out << quint32(pcm.size());
        out.writeRawData(pcm.constData(), pcm.size());
        return wav;
    }
};

bool Base::sFontsReady = false;
QFont Base::sTiny;
QFont Base::sSmall;
QFont Base::sBody;
QFont Base::sMedium;
QFont Base::sLarge;
QFont Base::sBig;
QFont Base::sTitle;
QFont Base::sHuge;
QFont Base::sTechTiny;
QFont Base::sTechSmall;
QFont Base::sTech;
QFont Base::sTechLarge;

const QColor Base::Ink{233, 246, 255};
const QColor Base::Text{207, 226, 242};
const QColor Base::Muted{119, 146, 171};
const QColor Base::Dim{72, 96, 120};
const QColor Base::Cyan{83, 232, 255};
const QColor Base::CyanSoft{39, 152, 181};
const QColor Base::Magenta{255, 91, 173};
const QColor Base::Coral{255, 96, 112};
const QColor Base::Amber{255, 171, 78};
const QColor Base::Violet{181, 128, 255};
const QColor Base::Green{105, 255, 184};
const QColor Base::White{244, 251, 255};
const QColor Base::Red{255, 72, 88};
const QColor Base::Panel{7, 14, 27};
const QColor Base::Black{1, 3, 8};

Base::Base() : impl_(std::make_unique<Impl>()) {}

Base::~Base() = default;

void Base::initialize()
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

void Base::keyPress(int key)
{
    impl_->keys.insert(key);
}

void Base::keyRelease(int key)
{
    impl_->keys.remove(key);
}

void Base::mouseMove(const Math::Vec2 &position)
{
    impl_->mousePosition = position;
    impl_->mouseActive = true;
}

void Base::mousePressLeft(const Math::Vec2 &position)
{
    impl_->mousePosition = position;
    impl_->mouseActive = true;
    impl_->mouseDown = true;
}

void Base::mouseReleaseLeft()
{
    impl_->mouseDown = false;
}

void Base::clear()
{
    impl_->keys.clear();
    impl_->mouseDown = false;
}

Math::Vec2 Base::movementAxis() const
{
    Math::Vec2 axis;
    if (impl_->keys.contains(Qt::Key_D) || impl_->keys.contains(Qt::Key_Right)) {
        axis.x += 1.0f;
    }
    if (impl_->keys.contains(Qt::Key_A) || impl_->keys.contains(Qt::Key_Left)) {
        axis.x -= 1.0f;
    }
    if (impl_->keys.contains(Qt::Key_S) || impl_->keys.contains(Qt::Key_Down)) {
        axis.y += 1.0f;
    }
    if (impl_->keys.contains(Qt::Key_W) || impl_->keys.contains(Qt::Key_Up)) {
        axis.y -= 1.0f;
    }
    if (axis.lengthSquared() > 0.0f) {
        axis = axis.normalized();
    }
    return axis;
}

bool Base::shootingHeld() const
{
    return impl_->mouseDown || impl_->keys.contains(Qt::Key_J);
}

bool Base::mouseDown() const
{
    return impl_->mouseDown;
}

bool Base::mouseActive() const
{
    return impl_->mouseActive;
}

Math::Vec2 Base::mousePosition() const
{
    return impl_->mousePosition;
}

void Base::setMouseActive(bool active)
{
    impl_->mouseActive = active;
}

void Base::initAudio()
{
    Impl &io = *impl_;
    if (io.audioInitialized) {
        return;
    }
    io.audioInitialized = true;

    if (QMediaDevices::defaultAudioOutput().isNull()) {
        io.audioEnabled = false;
        return;
    }

    const QString dirPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + QStringLiteral("/neon_core_cpp_audio");
    QDir dir;
    if (!dir.mkpath(dirPath)) {
        io.audioEnabled = false;
        return;
    }

    struct SoundDef {
        QString name;
        float duration;
        float freqStart;
        float freqEnd;
        QString wave;
        float volume;
        float noiseMix;
    };

    const QVector<SoundDef> defs = {
        {QStringLiteral("shoot"), 0.065f, 980.0f, 560.0f, QStringLiteral("square"), 0.20f, 0.10f},
        {QStringLiteral("hit"), 0.045f, 320.0f, 150.0f, QStringLiteral("noise"), 0.16f, 0.65f},
        {QStringLiteral("kill"), 0.20f, 220.0f, 55.0f, QStringLiteral("noise"), 0.30f, 0.72f},
        {QStringLiteral("hurt"), 0.30f, 160.0f, 55.0f, QStringLiteral("saw"), 0.32f, 0.35f},
        {QStringLiteral("dash"), 0.16f, 260.0f, 900.0f, QStringLiteral("sine"), 0.22f, 0.15f},
        {QStringLiteral("shoot_enemy"), 0.09f, 480.0f, 220.0f, QStringLiteral("saw"), 0.10f, 0.18f},
        {QStringLiteral("pickup"), 0.25f, 620.0f, 1240.0f, QStringLiteral("sine"), 0.24f, 0.03f},
        {QStringLiteral("ui"), 0.08f, 700.0f, 980.0f, QStringLiteral("sine"), 0.20f, 0.00f},
        {QStringLiteral("upgrade"), 0.55f, 360.0f, 1120.0f, QStringLiteral("sine"), 0.26f, 0.03f},
        {QStringLiteral("boss"), 0.85f, 90.0f, 42.0f, QStringLiteral("saw"), 0.28f, 0.42f},
        {QStringLiteral("victory"), 1.50f, 260.0f, 980.0f, QStringLiteral("sine"), 0.34f, 0.02f},
        {QStringLiteral("defeat"), 1.20f, 210.0f, 48.0f, QStringLiteral("saw"), 0.34f, 0.24f},
    };

    for (const SoundDef &def : defs) {
        const QString filename = dirPath + QStringLiteral("/") + def.name + QStringLiteral(".wav");
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            continue;
        }
        file.write(io.makeWav(def.duration, def.freqStart, def.freqEnd,
                              def.wave, def.volume, def.noiseMix));
        file.close();

        const QUrl url = QUrl::fromLocalFile(filename);
        QVector<QSoundEffect *> pool;
        for (int i = 0; i < Impl::PoolSize; ++i) {
            auto *effect = new QSoundEffect();
            effect->setSource(url);
            effect->setLoopCount(1);
            effect->setVolume(0.68);
            pool.append(effect);
        }
        io.pools.insert(def.name, pool);
        io.next.insert(def.name, 0);
    }
    io.audioEnabled = !io.pools.isEmpty();
}

void Base::playSound(const QString &name)
{
    Impl &io = *impl_;
    if (!io.audioEnabled || io.muted) {
        return;
    }
    const auto it = io.pools.find(name);
    if (it == io.pools.end() || it.value().isEmpty()) {
        return;
    }
    const QVector<QSoundEffect *> &pool = it.value();
    QSoundEffect *selected = nullptr;
    for (QSoundEffect *effect : pool) {
        if (!effect->isPlaying()) {
            selected = effect;
            break;
        }
    }
    if (!selected) {
        int &index = io.next[name];
        selected = pool.at(index % pool.size());
        index = (index + 1) % pool.size();
        selected->stop();
    }
    selected->play();
}

bool Base::toggleMute()
{
    Impl &io = *impl_;
    io.muted = !io.muted;
    if (io.muted) {
        for (const auto &pool : io.pools) {
            for (QSoundEffect *effect : pool) {
                effect->stop();
            }
        }
    }
    return io.muted;
}

bool Base::muted() const
{
    return impl_->muted;
}

bool Base::audioEnabled() const
{
    return impl_->audioEnabled;
}

const QFont &Base::tinyFont() { return sTiny; }
const QFont &Base::smallFont() { return sSmall; }
const QFont &Base::bodyFont() { return sBody; }
const QFont &Base::mediumFont() { return sMedium; }
const QFont &Base::largeFont() { return sLarge; }
const QFont &Base::bigFont() { return sBig; }
const QFont &Base::titleFont() { return sTitle; }
const QFont &Base::hugeFont() { return sHuge; }
const QFont &Base::techTinyFont() { return sTechTiny; }
const QFont &Base::techSmallFont() { return sTechSmall; }
const QFont &Base::techFont() { return sTech; }
const QFont &Base::techLargeFont() { return sTechLarge; }

QColor Base::shade(const QColor &color, float factor)
{
    return QColor(int(Math::clamp(color.red() * factor, 0.0f, 255.0f)),
                  int(Math::clamp(color.green() * factor, 0.0f, 255.0f)),
                  int(Math::clamp(color.blue() * factor, 0.0f, 255.0f)),
                  color.alpha());
}

QColor Base::withAlpha(const QColor &color, int alpha)
{
    return QColor(color.red(), color.green(), color.blue(), std::clamp(alpha, 0, 255));
}

void Base::fillCircle(QPainter &painter, const Math::Vec2 &center,
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

void Base::strokeCircle(QPainter &painter, const Math::Vec2 &center,
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

void Base::strokeLine(QPainter &painter, const Math::Vec2 &start,
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

void Base::arcRadians(QPainter &painter, const Math::Vec2 &center, float radius,
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

void Base::polygon(QPainter &painter, const QPolygonF &points,
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

void Base::glow(QPainter &glowPainter, const Math::Vec2 &pos, float radius,
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

QRectF Base::textBounds(const QFont &font, const QString &value,
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

void Base::text(QPainter &painter, const QFont &font, const QString &value,
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

void Base::shadowText(QPainter &painter, const QFont &font, const QString &value,
                      const QColor &color, const Math::Vec2 &pos,
                      const QString &anchor, int alpha, const QColor &shadow)
{
    text(painter, font, value, shadow, {pos.x + 2.0f, pos.y + 3.0f},
         anchor, int(alpha * 0.75f));
    text(painter, font, value, color, pos, anchor, alpha);
}

void Base::glowText(QPainter &painter, const QFont &font, const QString &value,
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

QStringList Base::wrapText(const QString &value, const QFont &font, float maxWidth)
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

void Base::bar(QPainter &painter, const QRectF &rect, float ratio,
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

void Base::panel(QPainter &painter, const QRectF &rect, const QColor &border,
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
