#pragma once

#include <cmath>

// 纯数学工具。模块拿到的是无状态计算能力，不持有游戏对象。
class Math {
public:
    static constexpr float Pi = 3.14159265358979323846f;
    static constexpr float Tau = 6.28318530717958647692f;

    struct Vec2 {
        float x = 0.0f;
        float y = 0.0f;

        Vec2() = default;
        Vec2(float xValue, float yValue) : x(xValue), y(yValue) {}

        Vec2 operator+(const Vec2 &other) const { return {x + other.x, y + other.y}; }
        Vec2 operator-(const Vec2 &other) const { return {x - other.x, y - other.y}; }
        Vec2 operator-() const { return {-x, -y}; }
        Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
        Vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }
        Vec2 &operator+=(const Vec2 &other) { x += other.x; y += other.y; return *this; }
        Vec2 &operator-=(const Vec2 &other) { x -= other.x; y -= other.y; return *this; }
        Vec2 &operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
        Vec2 &operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

        float lengthSquared() const { return x * x + y * y; }
        float length() const { return std::sqrt(lengthSquared()); }
        float dot(const Vec2 &other) const { return x * other.x + y * other.y; }
        float distanceSquaredTo(const Vec2 &other) const { return (*this - other).lengthSquared(); }
        float distanceTo(const Vec2 &other) const { return (*this - other).length(); }

        Vec2 normalized() const
        {
            const float value = length();
            if (value <= 0.000001f) {
                return {0.0f, 0.0f};
            }
            return {x / value, y / value};
        }

        Vec2 rotated(float radians) const
        {
            const float cosine = std::cos(radians);
            const float sine = std::sin(radians);
            return {x * cosine - y * sine, x * sine + y * cosine};
        }
    };

    static float clamp(float value, float low, float high);
    static float easeOutCubic(float t);
    static bool segmentCircle(const Vec2 &start, const Vec2 &end,
                              const Vec2 &center, float radius);

private:
    Math() = delete;
};
