#pragma once

#include <map>
#include <utility>
#include <SFML/System/Vector2.hpp>
#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Resolves unit-vs-unit collisions on opposing lanes.
 */
class CollisionSystem {
public:
    /**
     * @brief Resolves collisions between opposing units.
     *
     * Special units can survive multiple normal enemy collisions by taking
     * damage until their own health reaches zero.
     */
    void update(GameState& state,
                const std::map<int, sf::Vector2f>& towerPositions,
                float towerRadius,
                float unitRadius) const;

private:
    /** @brief Returns true when units travel between the same towers in opposite directions. */
    bool sameLaneOppositeDirection(const Unit& a, const Unit& b) const;
};

} // namespace tw
