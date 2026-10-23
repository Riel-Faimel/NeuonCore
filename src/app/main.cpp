#include "Menu/Menu.h"
#include "Base/Base.h"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFocusEvent>
#include <QGuiApplication>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QScreen>
#include <QShowEvent>
#include <QTextStream>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <tuple>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef DEBUG
#include "../tests/SelfTest.h"
#endif

class GameWindow : public QWidget {
public:
    explicit GameWindow(QWidget *parent = nullptr) : QWidget(parent), menu_(true)
    {
        setWindowTitle(QStringLiteral("星核协议 · NEON CORE"));
        qreal pixelRatio = 1.0;
        if (QScreen *primary = QGuiApplication::primaryScreen()) {
            pixelRatio = primary->devicePixelRatio();
        }
        setFixedSize(qRound(GameConfig::ScreenWidth / pixelRatio),
                     qRound(GameConfig::ScreenHeight / pixelRatio));
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setCursor(Qt::ArrowCursor);

        QPixmap icon(32, 32);
        icon.fill(Qt::transparent);
        {
            QPainter painter(&icon);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(Base::Cyan, 2));
            painter.setBrush(Qt::NoBrush);
            const QPolygonF ship({QPointF(16, 3), QPointF(28, 27),
                                  QPointF(16, 21), QPointF(4, 27)});
            painter.drawPolygon(ship);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(220, 253, 255));
            painter.drawEllipse(QPointF(16, 15), 3, 3);
        }
        setWindowIcon(QIcon(icon));

        menu_.render();
        clock_.start();
        timer_.setTimerType(Qt::PreciseTimer);
        timer_.setInterval(16);
        connect(&timer_, &QTimer::timeout, this, [this] { tick(); });
        timer_.start();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(rect(), menu_.frame());
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        menu_.keyPress(event->key(), event->isAutoRepeat());
        event->accept();
    }

    void keyReleaseEvent(QKeyEvent *event) override
    {
        if (!event->isAutoRepeat()) {
            menu_.keyRelease(event->key());
        }
        event->accept();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        menu_.mouseMove(toGamePos(event->position()));
        event->accept();
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            menu_.mousePressLeft(toGamePos(event->position()));
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            menu_.mouseReleaseLeft();
            event->accept();
        }
    }

    void focusOutEvent(QFocusEvent *event) override
    {
        menu_.clearInput();
        QWidget::focusOutEvent(event);
    }

    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        if (!centered_) {
            centered_ = true;
            if (QScreen *currentScreen = screen()) {
                const QRect available = currentScreen->availableGeometry();
                move(available.center() - rect().center());
            }
        }
        setFocus(Qt::ActiveWindowFocusReason);
    }

private:
    Math::Vec2 toGamePos(const QPointF &position) const
    {
        const float sx = width() > 0
                             ? float(GameConfig::ScreenWidth) / float(width())
                             : 1.0f;
        const float sy = height() > 0
                             ? float(GameConfig::ScreenHeight) / float(height())
                             : 1.0f;
        return {float(position.x()) * sx, float(position.y()) * sy};
    }

    void tick()
    {
        const qint64 elapsed = clock_.nsecsElapsed();
        clock_.restart();
        float dt = float(double(elapsed) / 1000000000.0);
        if (dt <= 0.00001f) {
            dt = 1.0f / 60.0f;
        }
        dt = Math::clamp(dt, 0.0f, 1.0f / 30.0f);

        menu_.update(dt);
        menu_.render();
        updateCursorShape();
        update();
    }

    void updateCursorShape()
    {
        const bool needBlank = menu_.inMatch();
        if (needBlank != cursorHidden_) {
            cursorHidden_ = needBlank;
            setCursor(needBlank ? Qt::BlankCursor : Qt::ArrowCursor);
        }
    }

    Menu menu_;
    QTimer timer_;
    QElapsedTimer clock_;
    bool cursorHidden_ = false;
    bool centered_ = false;
};

int main(int argc, char *argv[])
{
#ifdef DEBUG
    bool headless = false;
    bool smoke = false;
    bool capture = false;
    bool windowTest = false;
    QString captureDirectory = QStringLiteral("screenshots");
    QString windowShot = QStringLiteral("screenshots/window.png");
    for (int i = 1; i < argc; ++i) {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        if (argument == QStringLiteral("--smoke-test")) {
            smoke = true;
            headless = true;
        } else if (argument == QStringLiteral("--capture")) {
            capture = true;
            headless = true;
            if (i + 1 < argc) {
                captureDirectory = QString::fromLocal8Bit(argv[++i]);
            }
        } else if (argument == QStringLiteral("--window-test")) {
            windowTest = true;
            if (i + 1 < argc) {
                windowShot = QString::fromLocal8Bit(argv[++i]);
            }
        }
    }

    if (headless) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
#endif
    if (!qEnvironmentVariableIsSet("QT_SCALE_FACTOR")) {
        qputenv("QT_SCALE_FACTOR", "1");
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("NeonCore"));
    QApplication::setOrganizationName(QStringLiteral("NeonCore"));

#ifdef DEBUG
    if (smoke) {
        return SelfTest::run();
    }
    if (capture) {
        return SelfTest::capture(captureDirectory);
    }

    if (windowTest) {
        if (!QDir().mkpath(QFileInfo(windowShot).absolutePath())) {
            return 1;
        }
        GameWindow window;
        window.show();
        QTimer::singleShot(250, &window, [&window] {
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QApplication::sendEvent(&window, &press);
            QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
            QApplication::sendEvent(&window, &release);
        });
        QTimer::singleShot(1600, &window, [&window, windowShot] {
            const QPixmap shot = window.grab();
            QApplication::exit(shot.save(windowShot) ? 0 : 2);
        });
        return app.exec();
    }
#endif

    GameWindow window;
    window.show();
    return app.exec();
}

#ifdef Q_OS_WIN
extern int main(int argc, char *argv[]);

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main(__argc, __argv);
}
#endif
