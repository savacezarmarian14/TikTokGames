#include "utils/Math.hpp"
#include <cmath>

namespace tw {
namespace math {

float length(const sf::Vector2f& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

float distance(const sf::Vector2f& a, const sf::Vector2f& b) {
    return length(b - a);
}

sf::Vector2f normalize(const sf::Vector2f& v) {
    const float len = length(v);
    if (len <= 0.0001f) {
        return {0.0f, 0.0f};
    }
    return {v.x / len, v.y / len};
}

sf::Vector2f lerp(const sf::Vector2f& a, const sf::Vector2f& b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

float clamp(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

} // namespace math
} // namespace tw
