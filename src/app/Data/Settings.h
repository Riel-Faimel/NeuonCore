#pragma once

#include <QCoreApplication>
#include <QSettings>
#include <QString>

namespace app {

// 配置存储：
// - Windows：QSettings 默认格式写注册表（HKCU\Software\<org>\<app>），
//   MSIX 安装目录只读时也不会因为写 exe 同级文件而失败；
// - 其他平台：继续用 exe 同级的 settings.ini。
inline QSettings openSettings()
{
#ifdef Q_OS_WIN
    return QSettings();
#else
    return QSettings(
        QCoreApplication::applicationDirPath() + QStringLiteral("/settings.ini"),
        QSettings::IniFormat);
#endif
}

} // namespace app
