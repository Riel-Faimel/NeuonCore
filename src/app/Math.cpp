#include "Math.h"

#include <algorithm>

float Math::clamp(float value, float low, float high)
{
    return std::max(low, std::min(high, value));
}

float Math::easeOutCubic(float t)
{
    t = clamp(t, 0.0f, 1.0f);
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

bool Math::segmentCircle(const Vec2 &start, const Vec2 &end,
                         const Vec2 &center, float radius)
{
    const Vec2 delta = end - start;
    const float lengthSquared = delta.lengthSquared();
    if (lengthSquared <= 0.0001f) {
        return start.distanceSquaredTo(center) <= radius * radius;
    }
    const float t = clamp(((center - start).x * delta.x + (center - start).y * delta.y)
                              / lengthSquared,
                          0.0f, 1.0f);
    const Vec2 closest = start + delta * t;
    return closest.distanceSquaredTo(center) <= radius * radius;
}
