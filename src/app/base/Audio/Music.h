#pragma once

#include <QString>

#include <memory>

namespace base {

// Music 组件：背景音乐管理器。
// 播放器只在第一次初始化时创建一次，之后常驻主线程，由内部定时器推进曲目；
// 它的生命周期和任何界面 / 对局的创建销毁无关，切换界面不会重新起播。
// QMediaPlayer 自带解码线程，这里不需要再开 QThread。
class Music {
public:
    Music();
    ~Music();

    Music(const Music &) = delete;
    Music &operator=(const Music &) = delete;

    void initialize();
    void setMuted(bool muted);
    bool playing() const;
    int trackCount() const;
    QString currentTrack() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace base
