#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <map>

#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Draws towers, health rings, and tower labels.
 */
class TowerRenderer {
public:
    /**
     * @brief Renders every tower in the state.
     *
     * A cartoon cannon is drawn for each active opposing lane so towers appear
     * to target nearby enemy towers dynamically.
     */
    void render(sf::RenderTarget& target,
                const GameState& state,
                const std::map<int, sf::Vector2f>& towerPositions,
                float towerRadius,
                const sf::Font& font) const;

private:
    /** @brief Returns the display color for a team and alive/dead state. */
    sf::Color teamColor(Team team, bool alive) const;
    /** @brief Returns health-ring color for an HP ratio. */
    sf::Color hpColor(float ratio) const;

private:
    mutable sf::Clock clock_;
};

} // namespace tw
