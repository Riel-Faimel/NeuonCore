#include "Score.h"

void Score::reset()
{
    total_ = 0;
    kills_ = 0;
}

void Score::addKill(int points)
{
    total_ += points;
    ++kills_;
}
