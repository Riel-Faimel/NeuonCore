#pragma once

#include <QColor>
#include <QHash>
#include <QString>

#include <vector>

class Arena;
class Base;
class Random;
class SelfTest;

namespace games {

// Upgrades 组件：封装强化池、可选性判断、抽取、叠加与应用。
// 工具：Random（洗牌）、Arena（读取/修改玩家状态与播放反馈）、Base（音效）。
// 封装：强化定义、上限、等级计数、效果分支。
// 暴露：开启三选一、应用某张卡、当前候选卡、已叠加等级。
class Upgrades {
public:
    struct Offer {
        QString key;
        QString name;
        QString tag;
        QString desc;
        QColor color;
        int level = 0;
    };

    Upgrades(Random &random, Arena &arena, Base &base);

    void reset();
    bool open();
    bool apply(int index);

    const std::vector<Offer> &offers() const { return offers_; }
    int levelOf(const QString &key) const { return counts_.value(key); }
    bool isOpen() const { return open_; }

private:
    friend class ::SelfTest;

    struct Definition {
        QString key;
        QString name;
        QString tag;
        QString desc;
        QColor color;
    };

    static const std::vector<Definition> &catalog();
    bool eligible(const Definition &definition) const;
    void applyEffect(const QString &key);

    Random &random_;
    Arena &arena_;
    Base &base_;
    QHash<QString, int> counts_;
    std::vector<Offer> offers_;
    bool open_ = false;
};

} // namespace games
