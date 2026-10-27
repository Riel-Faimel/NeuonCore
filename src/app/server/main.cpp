#include "Net/Server.h"

#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include <cstdio>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("NeonCore"));
    QCoreApplication::setOrganizationName(QStringLiteral("NeonCore"));

    quint16 port = 7777;
    const QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QStringLiteral("--port") && i + 1 < args.size()) {
            bool ok = false;
            const uint parsed = args[++i].toUInt(&ok);
            if (!ok || parsed == 0 || parsed > 65535) {
                std::fprintf(stderr, "SERVER ERROR: invalid port\n");
                return 2;
            }
            port = quint16(parsed);
        } else if (args[i] == QStringLiteral("--help")
                   || args[i] == QStringLiteral("-h")) {
            std::printf("NeonCore server\n"
                        "  --port <1-65535>   listen port (default 7777)\n");
            return 0;
        } else {
            std::fprintf(stderr, "SERVER ERROR: unknown argument %s\n",
                         args[i].toUtf8().constData());
            return 2;
        }
    }

    server::GameServer gameServer;
    QString error;
    if (!gameServer.listen(port, false, &error)) {
        std::fprintf(stderr, "SERVER ERROR: %s\n", error.toUtf8().constData());
        return 1;
    }
    std::printf("NEON CORE server listening on port %u (tick %d Hz)\n",
                unsigned(gameServer.port()), gameServer.tickRate());
    std::fflush(stdout);
    return app.exec();
}
