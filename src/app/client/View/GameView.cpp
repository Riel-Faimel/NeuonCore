#include "View/GameView.h"

namespace client {

GameView::GameView() = default;
GameView::~GameView() = default;

void GameView::apply(const net::Snapshot &snapshot)
{
    world_.applySnapshot(snapshot);
    // 0 表示服务端没有给这个会话分配飞机（观战）；视图仍把第一架当作
    // 绘制/HUD 的参考机，Arena::isLocalPlayer() 会对 0 做这个回退。
    localPlayerId_ = snapshot.localPlayerId;

    boss_.active = snapshot.boss.active;
    boss_.hp = snapshot.boss.hp;
    boss_.maxHp = snapshot.boss.maxHp;
    boss_.hpDisplay = snapshot.boss.hpDisplay;

    waves_ = snapshot.waves;

    upgrades_.clear();
    upgrades_.reserve(snapshot.upgrades.size());
    for (const net::UpgradeOfferSnapshot &data : snapshot.upgrades) {
        games::Upgrades::Offer offer;
        offer.key = data.key;
        offer.name = data.name;
        offer.tag = data.tag;
        offer.desc = data.desc;
        offer.valueText = data.valueText;
        offer.color = data.color;
        offer.level = data.level;
        offer.value = data.value;
        upgrades_.push_back(std::move(offer));
    }

    state_ = snapshot.state;
    runTime_ = snapshot.runTime;
    score_ = snapshot.score;
    kills_ = snapshot.kills;
}

} // namespace client
