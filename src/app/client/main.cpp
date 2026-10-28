#include "Menu/Menu.h"
#include "Base/Base.h"
#include "Data/GameData.h"
#include "Loadout/Loadout.h"
#include "Net/Protocol.h"
#include "Shop/Shop.h"

#include <QApplication>
#include <QCoreApplication>
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
#include <QQmlEngine>
#include <QQuickWidget>
#include <QQuickItem>
#include <QUrl>
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
    explicit GameWindow(bool offline = false, const QString &host = QString(),
                        quint16 port = 0, QWidget *parent = nullptr)
        : QWidget(parent), menu_(true, offline, host, port)
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
        setupResultOverlay();
        setupFailureOverlay();
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
            BaseService.pointerDraw(painter);
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
        BaseService.pointerMoveTo(position);
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
            BaseService.pointerMoveTo(position);
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
        BaseService.pointerMoveTo(position);
        QWidget::enterEvent(event);
    }

    // 指针离开窗口：恢复系统箭头，窗口里也不再画准星。
    void leaveEvent(QEvent *event) override
    {
        setCursor(Qt::ArrowCursor);
        BaseService.pointerSetVisible(false);
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
            BaseService.pointerMoveTo(position);
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

    void setupResultOverlay()
    {
        resultOverlay_ = new QQuickWidget(this);
        resultOverlay_->setResizeMode(QQuickWidget::SizeRootObjectToView);
        resultOverlay_->setClearColor(Qt::transparent);
        resultOverlay_->setFocusPolicy(Qt::NoFocus);

        const QString appQml = QCoreApplication::applicationDirPath()
                               + QStringLiteral("/qml/ResultOverlay.qml");
        const QString sourceQml = QDir::currentPath()
                                  + QStringLiteral("/src/qml/ResultOverlay.qml");
        const QString path = QFileInfo::exists(appQml) ? appQml : sourceQml;
        resultOverlay_->engine()->addImportPath(
            QCoreApplication::applicationDirPath() + QStringLiteral("/qml"));
        resultOverlay_->setSource(QUrl::fromLocalFile(path));
        if (resultOverlay_->status() != QQuickWidget::Ready) {
            delete resultOverlay_;
            resultOverlay_ = nullptr;
            return;
        }
        resultQmlRoot_ = resultOverlay_->rootObject();
        resultOverlay_->setGeometry(rect());
        resultOverlay_->hide();
    }

    void syncResultOverlay()
    {
        if (resultOverlay_ == nullptr || resultQmlRoot_ == nullptr) {
            return;
        }
        if (menu_.screen() != Menu::Screen::Result) {
            resultOverlay_->hide();
            return;
        }
        const menu::ResultSummary summary = menu_.resultSummary();
        resultQmlRoot_->setProperty("victory", summary.victory);
        resultQmlRoot_->setProperty("levelName", summary.levelName);
        resultQmlRoot_->setProperty("score", summary.score);
        resultQmlRoot_->setProperty("kills", summary.kills);
        resultQmlRoot_->setProperty("wave", summary.wave);
        resultQmlRoot_->setProperty("totalWaves", summary.totalWaves);
        resultQmlRoot_->setProperty("runTime", double(summary.runTime));
        if (resultQmlRoot_->property("retryRequested").toBool()) {
            resultQmlRoot_->setProperty("retryRequested", false);
            menu_.keyPress(Qt::Key_Return);
            return;
        }
        if (resultQmlRoot_->property("backRequested").toBool()) {
            resultQmlRoot_->setProperty("backRequested", false);
            menu_.keyPress(Qt::Key_Escape);
            return;
        }
        resultOverlay_->setGeometry(rect());
        resultOverlay_->show();
        resultOverlay_->raise();
    }

    void setupFailureOverlay()
    {
        failureOverlay_ = new QQuickWidget(this);
        failureOverlay_->setResizeMode(QQuickWidget::SizeRootObjectToView);
        failureOverlay_->setClearColor(Qt::transparent);
        failureOverlay_->setFocusPolicy(Qt::NoFocus);

        const QString appQml = QCoreApplication::applicationDirPath()
                               + QStringLiteral("/qml/FailureOverlay.qml");
        const QString sourceQml = QDir::currentPath()
                                  + QStringLiteral("/src/qml/FailureOverlay.qml");
        const QString path = QFileInfo::exists(appQml) ? appQml : sourceQml;
        failureOverlay_->engine()->addImportPath(
            QCoreApplication::applicationDirPath() + QStringLiteral("/qml"));
        failureOverlay_->setSource(QUrl::fromLocalFile(path));
        if (failureOverlay_->status() != QQuickWidget::Ready) {
            delete failureOverlay_;
            failureOverlay_ = nullptr;
            return;
        }
        failureQmlRoot_ = failureOverlay_->rootObject();
        failureOverlay_->setGeometry(rect());
        failureOverlay_->hide();
    }

    void syncFailureOverlay()
    {
        if (failureOverlay_ == nullptr || failureQmlRoot_ == nullptr) {
            return;
        }
        if (menu_.screen() != Menu::Screen::Failure) {
            failureOverlay_->hide();
            return;
        }
        const menu::FailureView view = menu_.failureSummary();
        failureQmlRoot_->setProperty("title", view.title);
        failureQmlRoot_->setProperty("detail", view.detail);
        failureQmlRoot_->setProperty("endpoint", view.endpoint);
        failureQmlRoot_->setProperty("retryable", view.retryable);
        if (failureQmlRoot_->property("retryRequested").toBool()) {
            failureQmlRoot_->setProperty("retryRequested", false);
            menu_.keyPress(Qt::Key_Return);
            return;
        }
        if (failureQmlRoot_->property("backRequested").toBool()) {
            failureQmlRoot_->setProperty("backRequested", false);
            menu_.keyPress(Qt::Key_Escape);
            return;
        }
        failureOverlay_->setGeometry(rect());
        failureOverlay_->show();
        failureOverlay_->raise();
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
        BaseService.pointerUpdate(dt);
        menu_.render();
        syncResultOverlay();
        syncFailureOverlay();
        update();
    }

    Menu menu_;
    QQuickWidget *resultOverlay_ = nullptr;
    QQuickItem *resultQmlRoot_ = nullptr;
    QQuickWidget *failureOverlay_ = nullptr;
    QQuickItem *failureQmlRoot_ = nullptr;
    QTimer timer_;
    QElapsedTimer clock_;
    bool centered_ = false;
};

int main(int argc, char *argv[])
{
    // 编译期常量来自 src/json；解析异常会退化为默认值/空表，入口显式拒绝启动。
    if (const char *dataError = data::validationError(); dataError != nullptr) {
        std::fprintf(stderr, "DATA ERROR: %s\n", dataError);
        return 1;
    }

    bool offline = false;
    bool showHelp = false;
    QString connectHost;
    quint16 connectPort = net::kDefaultPort;
    QString argumentError;

    DebugInfo(
    bool headless = false;
    bool smoke = false;
    bool capture = false;
    bool windowTest = false;
    bool bench = false;
    QString captureDirectory = QStringLiteral("screenshots");
    QString windowShot = QStringLiteral("screenshots/window.png");
    )

    for (int i = 1; i < argc; ++i) {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        if (argument == QStringLiteral("--local")) {
            offline = true;
        } else if (argument == QStringLiteral("--help") || argument == QStringLiteral("-h")) {
            showHelp = true;
        } else if (argument == QStringLiteral("--connect")) {
            if (i + 1 >= argc) {
                argumentError = QStringLiteral("--connect requires host:port");
                break;
            }
            const QString endpoint = QString::fromLocal8Bit(argv[++i]);
            const int colon = endpoint.lastIndexOf(QLatin1Char(':'));
            if (colon <= 0 || colon >= endpoint.size() - 1) {
                argumentError = QStringLiteral("--connect expects host:port");
                break;
            }
            bool ok = false;
            const uint value = endpoint.mid(colon + 1).toUInt(&ok);
            if (!ok || value == 0 || value > 65535) {
                argumentError = QStringLiteral("--connect port must be 1..65535");
                break;
            }
            connectHost = endpoint.left(colon);
            connectPort = quint16(value);
#ifdef DEBUG
        } else if (argument == QStringLiteral("--smoke-test")) {
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
        } else if (argument == QStringLiteral("--bench")) {
            bench = true;
            headless = true;
#endif
        } else {
            argumentError = QStringLiteral("unknown argument: %1").arg(argument);
            break;
        }
    }

    auto usage = [] {
        std::fprintf(stderr,
                     "usage: NeonCore [--local] [--connect host:port]\n"
                     "       NeonCoreServer --port 7777   (stdio control)\n"
                     "       NeonCoreConsole --smoke-test | --capture DIR | --bench\n");
    };
    if (showHelp) {
        usage();
        return 0;
    }
    if (!argumentError.isEmpty()) {
        std::fprintf(stderr, "ARGUMENT ERROR: %s\n", argumentError.toUtf8().constData());
        usage();
        return 2;
    }
    if (offline && !connectHost.isEmpty()) {
        std::fprintf(stderr, "ARGUMENT ERROR: --local cannot be combined with --connect\n");
        return 2;
    }

#ifdef DEBUG
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

    // 全局服务：字体/颜色等绘制资源必须在 QGuiApplication 之后初始化。
    // 音频（QSoundEffect / QMediaPlayer）由 Menu 按需调用 BaseService.initAudio()。
    Base::initialize();
    // Shop 的存档由 main 负责：启动读一次，退出前写一次。
    ShopService.load();
    LoadoutService.load();

#ifdef DEBUG
    if (smoke) {
        const int code = SelfTest::run();
        ShopService.save();
        LoadoutService.save();
        BaseService.shutdown();
        return code;
    }
    if (capture) {
        const int code = SelfTest::capture(captureDirectory);
        ShopService.save();
        LoadoutService.save();
        BaseService.shutdown();
        return code;
    }
    if (bench) {
        const int code = SelfTest::bench();
        ShopService.save();
        LoadoutService.save();
        BaseService.shutdown();
        return code;
    }

    if (windowTest) {
        if (!QDir().mkpath(QFileInfo(windowShot).absolutePath())) {
            return 1;
        }
        GameWindow window(offline, connectHost, connectPort);
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
        const int code = app.exec();
        ShopService.save();
        LoadoutService.save();
        BaseService.shutdown();
        return code;
    }
#endif

    GameWindow window(offline, connectHost, connectPort);
    window.show();
    const int code = app.exec();
    // BaseService 是全局对象，析构发生在 QApplication 之后；
    // 先把音频对象在应用还活着时释放掉。
    ShopService.save();
    LoadoutService.save();
    BaseService.shutdown();
    return code;
}

#ifdef Q_OS_WIN
extern int main(int argc, char *argv[]);

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main(__argc, __argv);
}
#endif
