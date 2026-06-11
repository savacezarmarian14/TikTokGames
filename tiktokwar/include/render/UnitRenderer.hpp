#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <map>
#include <unordered_map>
#include <vector>

#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Draws moving units, trails, and spawn/destruction particles.
 */
class UnitRenderer {
public:
    /**
     * @brief Renders all live units and updates transient unit particles.
     *
     * Normal, shielded, and spiked units share one rendering path. Special
     * unit visuals are selected via UnitKind.
     */
    void render(sf::RenderTarget& target,
                const GameState& state,
                const std::map<int, sf::Vector2f>& towerPositions,
                float towerRadius,
                float unitRadius) const;

private:
    struct Particle {
        sf::Vector2f position;
        sf::Vector2f velocity;
        sf::Color color;
        float radius = 2.0f;
        float lifetime = 0.0f;
        float maxLifetime = 0.0f;
    };

private:
    /** @brief Returns the display color for a unit team. */
    sf::Color teamColor(Team team) const;
    /** @brief Emits short-lived particles at a position. */
    void spawnBurst(const sf::Vector2f& pos, const sf::Color& color, bool strong) const;

private:
    mutable sf::Clock frameClock_;
    mutable sf::Clock animationClock_;
    mutable std::unordered_map<int, sf::Vector2f> previousPositions_;
    mutable std::vector<Particle> particles_;
};

} // namespace tw
