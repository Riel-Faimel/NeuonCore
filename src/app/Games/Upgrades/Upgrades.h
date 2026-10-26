#pragma once

#include <QColor>
#include <QHash>
#include <QString>

#include <string_view>
#include <vector>

class Arena;
class SelfTest;

namespace games {

// Upgrades 组件：封装强化池、可选性判断、抽取、叠加与应用。
// 工具：Random（洗牌 + 每轮数值）、Arena（读取/修改玩家状态与播放反馈）、Base（音效）。
// 每轮三选一都会为“数值型”强化重新掷一个区间内的随机值，卡片上显示的就是实际生效值。
class Upgrades {
public:
    struct Offer {
        QString key;
        QString name;
        QString tag;
        QString desc;
        QString valueText;   // “+27%” / “+34 HP” / “+1 穿透” 之类
        QColor color;
        int level = 0;
        float value = 0.0f;  // 本轮掷出的实际数值
    };

    Upgrades(Arena &arena);

    void reset();
    bool open();
    bool apply(int index);

    const std::vector<Offer> &offers() const { return offers_; }
    int levelOf(const QString &key) const { return counts_.value(key); }
    bool isOpen() const { return open_; }

private:
    friend class ::SelfTest;

    struct Definition {
        std::string_view key;
        QString name;
        QString tag;
        QColor color;
        float minValue = 0.0f;
        float maxValue = 0.0f;
    };

    static const std::vector<Definition> &catalog();
    bool eligible(const Definition &definition) const;
    float rollValue(const Definition &definition) const;
    static QString describe(std::string_view key, float value);
    static QString valueLabel(std::string_view key, float value);
    void applyEffect(const Offer &offer);

    Arena &arena_;
    QHash<QString, int> counts_;
    std::vector<Offer> offers_;
    bool open_ = false;
};

} // namespace games
