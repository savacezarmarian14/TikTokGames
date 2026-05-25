#pragma once

#include <SFML/System/Vector2.hpp>

namespace tw {
namespace math {

/** @brief Returns the Euclidean length of a vector. */
float length(const sf::Vector2f& v);
/** @brief Returns the Euclidean distance between two points. */
float distance(const sf::Vector2f& a, const sf::Vector2f& b);
/** @brief Returns a unit-length vector, or zero when input length is zero. */
sf::Vector2f normalize(const sf::Vector2f& v);
/** @brief Linearly interpolates between two vectors. */
sf::Vector2f lerp(const sf::Vector2f& a, const sf::Vector2f& b, float t);
/** @brief Clamps a scalar value into the inclusive range [minValue, maxValue]. */
float clamp(float value, float minValue, float maxValue);

} // namespace math
} // namespace tw
