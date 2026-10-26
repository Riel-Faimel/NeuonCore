#include "Base.h"

#include "Base/Audio/Audio.h"
#include "Base/Audio/Music.h"
#include "Base/Input/Input.h"
#include "Base/Paint/Paint.h"

#include <memory>

struct Base::Impl {
    base::Input input;
    base::Audio audio;
    base::Music music;
};

Base::Base() : impl_(std::make_unique<Impl>()) {}

Base::~Base() = default;

void Base::initialize()
{
    base::Paint::initialize();
}

void Base::keyPress(int key)
{
    impl_->input.keyPress(key);
}

void Base::keyRelease(int key)
{
    impl_->input.keyRelease(key);
}

void Base::mouseMove(const Math::Vec2 &position)
{
    impl_->input.mouseMove(position);
}

void Base::mousePressLeft(const Math::Vec2 &position)
{
    impl_->input.mousePressLeft(position);
}

void Base::mouseReleaseLeft()
{
    impl_->input.mouseReleaseLeft();
}

void Base::clear()
{
    impl_->input.clear();
}

Math::Vec2 Base::movementAxis() const
{
    return impl_->input.movementAxis();
}

bool Base::shootingHeld() const
{
    return impl_->input.shootingHeld();
}

bool Base::mouseDown() const
{
    return impl_->input.mouseDown();
}

bool Base::mouseActive() const
{
    return impl_->input.mouseActive();
}

Math::Vec2 Base::mousePosition() const
{
    return impl_->input.mousePosition();
}

void Base::setMouseActive(bool active)
{
    impl_->input.setMouseActive(active);
}

void Base::initAudio()
{
    impl_->audio.init();
    impl_->music.initialize();
}

void Base::playSound(const QString &name)
{
    impl_->audio.play(name);
}

void Base::setSoundVolume(float volume)
{
    impl_->audio.setVolume(volume);
}

float Base::soundVolume() const
{
    return impl_->audio.volume();
}

void Base::setMusicVolume(float volume)
{
    impl_->music.setVolume(volume);
}

float Base::musicVolume() const
{
    return impl_->music.volume();
}

QPixmap Base::appIcon(int size)
{
    return base::Paint::appIcon(size);
}

bool Base::toggleMute()
{
    const bool muted = impl_->audio.toggleMute();
    impl_->music.setMuted(muted);
    return muted;
}

bool Base::muted() const
{
    return impl_->audio.muted();
}

bool Base::audioEnabled() const
{
    return impl_->audio.enabled();
}

const QColor &Base::Ink = base::Paint::Ink;
const QColor &Base::Text = base::Paint::Text;
const QColor &Base::Muted = base::Paint::Muted;
const QColor &Base::Dim = base::Paint::Dim;
const QColor &Base::Cyan = base::Paint::Cyan;
const QColor &Base::CyanSoft = base::Paint::CyanSoft;
const QColor &Base::Magenta = base::Paint::Magenta;
const QColor &Base::Coral = base::Paint::Coral;
const QColor &Base::Amber = base::Paint::Amber;
const QColor &Base::Violet = base::Paint::Violet;
const QColor &Base::Green = base::Paint::Green;
const QColor &Base::White = base::Paint::White;
const QColor &Base::Red = base::Paint::Red;
const QColor &Base::Panel = base::Paint::Panel;
const QColor &Base::Black = base::Paint::Black;

const QFont &Base::tinyFont() { return base::Paint::tinyFont(); }
const QFont &Base::smallFont() { return base::Paint::smallFont(); }
const QFont &Base::bodyFont() { return base::Paint::bodyFont(); }
const QFont &Base::mediumFont() { return base::Paint::mediumFont(); }
const QFont &Base::largeFont() { return base::Paint::largeFont(); }
const QFont &Base::bigFont() { return base::Paint::bigFont(); }
const QFont &Base::titleFont() { return base::Paint::titleFont(); }
const QFont &Base::hugeFont() { return base::Paint::hugeFont(); }
const QFont &Base::techTinyFont() { return base::Paint::techTinyFont(); }
const QFont &Base::techSmallFont() { return base::Paint::techSmallFont(); }
const QFont &Base::techFont() { return base::Paint::techFont(); }
const QFont &Base::techLargeFont() { return base::Paint::techLargeFont(); }

void Base::reticle(QPainter &painter, const Math::Vec2 &center, float time)
{
    base::Paint::reticle(painter, center, time);
}

void Base::gear(QPainter &painter, const Math::Vec2 &center, float radius,
                const QColor &color)
{
    base::Paint::gear(painter, center, radius, color);
}

void Base::crate(QPainter &painter, const Math::Vec2 &center, float radius,
                 const QColor &color)
{
    base::Paint::crate(painter, center, radius, color);
}

QColor Base::shade(const QColor &color, float factor)
{
    return base::Paint::shade(color, factor);
}

QColor Base::withAlpha(const QColor &color, int alpha)
{
    return base::Paint::withAlpha(color, alpha);
}

void Base::fillCircle(QPainter &painter, const Math::Vec2 &center,
                      float radius, const QColor &color)
{
    base::Paint::fillCircle(painter, center, radius, color);
}

void Base::strokeCircle(QPainter &painter, const Math::Vec2 &center,
                        float radius, const QColor &color, float width)
{
    base::Paint::strokeCircle(painter, center, radius, color, width);
}

void Base::strokeLine(QPainter &painter, const Math::Vec2 &start,
                      const Math::Vec2 &end, const QColor &color, float width)
{
    base::Paint::strokeLine(painter, start, end, color, width);
}

void Base::arcRadians(QPainter &painter, const Math::Vec2 &center, float radius,
                      float startRadians, float spanRadians,
                      const QColor &color, float width)
{
    base::Paint::arcRadians(painter, center, radius, startRadians, spanRadians,
                            color, width);
}

void Base::polygon(QPainter &painter, const QPolygonF &points,
                   const QColor &fill, const QColor &outline, float width)
{
    base::Paint::polygon(painter, points, fill, outline, width);
}

void Base::glow(QPainter &glowPainter, const Math::Vec2 &pos, float radius,
                const QColor &color, float intensity)
{
    base::Paint::glow(glowPainter, pos, radius, color, intensity);
}

QRectF Base::textBounds(const QFont &font, const QString &value,
                        const Math::Vec2 &pos, const QString &anchor)
{
    return base::Paint::textBounds(font, value, pos, anchor);
}

void Base::text(QPainter &painter, const QFont &font, const QString &value,
                const QColor &color, const Math::Vec2 &pos,
                const QString &anchor, int alpha)
{
    base::Paint::text(painter, font, value, color, pos, anchor, alpha);
}

void Base::shadowText(QPainter &painter, const QFont &font, const QString &value,
                      const QColor &color, const Math::Vec2 &pos,
                      const QString &anchor, int alpha, const QColor &shadow)
{
    base::Paint::shadowText(painter, font, value, color, pos, anchor, alpha, shadow);
}

void Base::glowText(QPainter &painter, const QFont &font, const QString &value,
                    const QColor &color, const Math::Vec2 &pos,
                    const QString &anchor, int alpha)
{
    base::Paint::glowText(painter, font, value, color, pos, anchor, alpha);
}

QStringList Base::wrapText(const QString &value, const QFont &font, float maxWidth)
{
    return base::Paint::wrapText(value, font, maxWidth);
}

void Base::bar(QPainter &painter, const QRectF &rect, float ratio,
               const QColor &color, float ghost, const QColor &background)
{
    base::Paint::bar(painter, rect, ratio, color, ghost, background);
}

void Base::panel(QPainter &painter, const QRectF &rect, const QColor &border,
                 int alpha, int radius, const QColor &fill)
{
    base::Paint::panel(painter, rect, border, alpha, radius, fill);
}
