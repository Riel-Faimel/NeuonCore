#pragma once

#include <QString>

#include <memory>

namespace base {

// Audio 组件：程序化 WAV 与音效池。QSoundEffect 细节封装在 Impl。
class Audio {
public:
    Audio();
    ~Audio();

    Audio(const Audio &) = delete;
    Audio &operator=(const Audio &) = delete;

    void init();
    void play(const QString &name);
    bool toggleMute();
    bool muted() const;
    bool enabled() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace base
