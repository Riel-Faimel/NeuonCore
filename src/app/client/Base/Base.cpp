#include "Base.h"

#include "Base/Audio/Audio.h"
#include "Base/Audio/Music.h"
#include "Base/Input/Input.h"
#include "Base/Paint/Paint.h"
#include "Base/Pointer/Pointer.h"
#include "Base/ThreadPool/ThreadPool.h"

#include <memory>

// 进程内唯一的 Base 实例（声明见 Base.h）。构造只分配 Impl，
// 不创建 QSoundEffect / QMediaPlayer，也不依赖 QApplication 是否已存在。
Base BaseService;

struct Base::Impl {
    base::Input input;
    base::Audio audio;
    base::Music music;
    base::Pointer pointer;
    base::ThreadPool threads;
};

Base::Base() : impl_(std::make_unique<Impl>()) {}

Base::~Base() = default;

void Base::initialize()
{
    base::Paint::initialize();
    // 工作线程在 QApplication 就绪后启动，避免静态初始化期就创建线程。
    BaseService.impl_->threads.start();
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

void Base::pointerMoveTo(const Math::Vec2 &position)
{
    impl_->pointer.moveTo(position);
}

void Base::pointerSetVisible(bool visible)
{
    impl_->pointer.setVisible(visible);
}

void Base::pointerUpdate(float dt)
{
    impl_->pointer.update(dt);
}

void Base::pointerDraw(QPainter &painter) const
{
    impl_->pointer.draw(painter);
}

const Math::Vec2 &Base::pointerPosition() const
{
    return impl_->pointer.position();
}

float Base::pointerTime() const
{
    return impl_->pointer.time();
}

bool Base::pointerVisible() const
{
    return impl_->pointer.visible();
}

bool Base::parallelEnabled() const
{
    return impl_->threads.enabled();
}

int Base::parallelWorkers() const
{
    return impl_->threads.workerCount();
}

void Base::parallelFor(int taskCount, const std::function<void(int)> &task)
{
    impl_->threads.parallelFor(taskCount, task);
}

void Base::beginWorldPass()
{
    base::Paint::setPass(base::Paint::Pass::World);
}

void Base::beginGlowPass()
{
    base::Paint::setPass(base::Paint::Pass::Glow);
}

void Base::endRenderPass()
{
    base::Paint::setPass(base::Paint::Pass::All);
}

void Base::initAudio()
{
    impl_->audio.init();
    impl_->music.initialize();
}

void Base::shutdown()
{
    impl_->threads.shutdown();
    impl_->audio.shutdown();
    impl_->music.shutdown();
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
const QFont &Base::techTinyFont() { return base::Paint::techTinyFont(); }
const QFont &Base::techSmallFont() { return base::Paint::techSmallFont(); }
const QFont &Base::techFont() { return base::Paint::techFont(); }

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

void Base::text(QPainter &painter, const QFont &font, const QString &value,
                const QColor &color, const Math::Vec2 &pos,
                const QString &anchor, int alpha)
{
    base::Paint::text(painter, font, value, color, pos, anchor, alpha);
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
