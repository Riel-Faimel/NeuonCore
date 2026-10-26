#include "Menu/Match/Match.h"

#include "Inventory.h"

namespace menu {

void Match::start(const Games::Level &level, Base &base, Score &score, Inventory &inventory)
{
    games_ = std::make_unique<Games>(level, base, score, inventory);
}

void Match::stop()
{
    games_.reset();
}

void Match::update(float dt)
{
    if (games_) {
        games_->update(dt);
    }
}

void Match::render()
{
    if (games_) {
        games_->render();
    }
}

const QImage &Match::frame() const
{
    return games_->frame();
}

Games::State Match::state() const
{
    return games_->state();
}

bool Match::finished() const
{
    if (!games_) {
        return false;
    }
    return games_->state() == Games::State::Victory
           || games_->state() == Games::State::Defeat;
}

int Match::runScore() const
{
    return games_ ? games_->runScore() : 0;
}

int Match::kills() const
{
    return games_ ? games_->kills() : 0;
}

int Match::wave() const
{
    return games_ ? games_->currentWave() : 0;
}

int Match::totalWaves() const
{
    return games_ ? games_->totalWaves() : 0;
}

float Match::runTime() const
{
    return games_ ? games_->runTime() : 0.0f;
}

void Match::keyPress(int key, bool autoRepeat)
{
    if (games_) {
        games_->keyPress(key, autoRepeat);
    }
}

void Match::keyRelease(int key)
{
    if (games_) {
        games_->keyRelease(key);
    }
}

void Match::mouseMove(const Math::Vec2 &position)
{
    if (games_) {
        games_->mouseMove(position);
    }
}

void Match::mousePressLeft(const Math::Vec2 &position)
{
    if (games_) {
        games_->mousePressLeft(position);
    }
}

void Match::mousePressRight(const Math::Vec2 &position)
{
    if (games_) {
        games_->mousePressRight(position);
    }
}

void Match::mouseReleaseLeft()
{
    if (games_) {
        games_->mouseReleaseLeft();
    }
}

} // namespace menu
