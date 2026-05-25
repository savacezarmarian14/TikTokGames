#pragma once

#include <map>
#include <SFML/System/Vector2.hpp>
#include "core/Config.hpp"
#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Applies unit impact effects to destination towers.
 */
class CombatSystem {
public:
    /** @brief Stores a shared configuration reference. */
    explicit CombatSystem(const Config& config);
    /** @brief Damages or heals target towers when units reach impact range. */
    void update(GameState& state, const std::map<int, sf::Vector2f>& towerPositions, float towerRadius) const;

private:
    const Config& config_;
};

} // namespace tw
