#include "Menu/Menu.h"
#include "Base/Base.h"
#include "Pointer.h"

#include <QApplication>
#include <QDir>
#include <QCursor>
#include <QElapsedTimer>
#include <QEnterEvent>
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
        setCursor(Qt::BlankCursor);

        // 应用图标由代码绘制：一次生成多档尺寸，窗口/任务栏都用它。
        QIcon icon;
        for (int size : {16, 24, 32, 48, 64, 128, 256}) {
            icon.addPixmap(Base::appIcon(size));
        }
        setWindowIcon(icon);

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

        // 指针是全局元素：每一帧都叠在画面最上层，不参与任何画面自己的绘制。
        if (width() > 0 && height() > 0) {
            painter.save();
            painter.scale(float(width()) / float(GameConfig::ScreenWidth),
                          float(height()) / float(GameConfig::ScreenHeight));
            pointer_.draw(painter);
            painter.restore();
        }
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
        const Math::Vec2 position = toGamePos(event->position());
        menu_.mouseMove(position);
        pointer_.moveTo(position);
        event->accept();
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton
            || event->button() == Qt::RightButton) {
            const Math::Vec2 position = toGamePos(event->position());
            if (event->button() == Qt::LeftButton) {
                menu_.mousePressLeft(position);
            } else {
                menu_.mousePressRight(position);
            }
            pointer_.moveTo(position);
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

    // 指针在窗口内：隐藏系统光标，由全局准星代替。
    void enterEvent(QEnterEvent *event) override
    {
        setCursor(Qt::BlankCursor);
        const Math::Vec2 position = toGamePos(event->position());
        menu_.mouseMove(position);
        pointer_.moveTo(position);
        QWidget::enterEvent(event);
    }

    // 指针离开窗口：恢复系统箭头，窗口里也不再画准星。
    void leaveEvent(QEvent *event) override
    {
        setCursor(Qt::ArrowCursor);
        pointer_.setVisible(false);
        QWidget::leaveEvent(event);
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

        const QPoint local = mapFromGlobal(QCursor::pos());
        if (rect().contains(local)) {
            const Math::Vec2 position = toGamePos(QPointF(local));
            menu_.mouseMove(position);
            pointer_.moveTo(position);
        }
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
        pointer_.update(dt);
        menu_.render();
        update();
    }

    Menu menu_;
    Pointer pointer_;
    QTimer timer_;
    QElapsedTimer clock_;
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
