#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <unordered_map>

#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Placeholder for future HUD rendering.
 */
class HudRenderer {
public:
    /** @brief Renders HUD elements for the current frame. */
    void render(sf::RenderTarget& target,
                const GameState& state,
                const sf::Font& font,
                const sf::Vector2u& windowSize) const;

private:
    mutable sf::Clock clock_;
    mutable std::unordered_map<int, float> displayedRatios_;
};

} // namespace tw
