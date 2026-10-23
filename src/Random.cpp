#include "Random.h"

Random::Random(unsigned int seed) : engine_(seed), seed_(seed) {}

float Random::real(float low, float high)
{
    std::uniform_real_distribution<float> distribution(low, high);
    return distribution(engine_);
}

int Random::integer(int low, int high)
{
    std::uniform_int_distribution<int> distribution(low, high);
    return distribution(engine_);
}
